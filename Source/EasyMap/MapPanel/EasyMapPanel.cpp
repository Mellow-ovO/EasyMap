// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyMap/MapPanel/EasyMapPanel.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Button.h"

// ContainerCanvas 以自身左上角对齐 FrameCanvas 中心点
static const FAnchors DEFAULT_CONTAINER_ANCHORS = FAnchors(0.5f);
static const FVector2D DEFAULT_CONTAINER_ALIGNMENT = FVector2D::ZeroVector;
static const FMargin DEFAULT_CONTAINER_OFFSET = FMargin(0.f);

// Map 内 Node 默认中心点对齐
static const FVector2D DEFAULT_MAP_NODE_ALIGNMENT = FVector2D(0.5f);

// OnClicked 事件触发的最大移动距离
static constexpr int MAX_CLICK_POINTER_DELTA = 400;

// OnClicked 事件触发的手指抬起最大响应事件
static constexpr float MAX_CLICK_INTERVAL = 0.2;

struct UEasyMapPanel::Implement : public FGCObject, public FTickableGameObject
{
	Implement(UEasyMapPanel* InOwner)
		: Owner(InOwner)
	{
	}

	void SetMapDetail(const FMapDetail& InMapDetail)
	{
		if (!FrameCanvas.IsValid()) return;

		// 清理之前的 Layer
		for (UImage* ImageWidget : LayerImageWidgets)
		{
			ImageWidget->RemoveFromParent();
		}
		LayerImageWidgets.Empty();

		for (const FMapLayer& Layer : InMapDetail.Layers)
		{
			AddMapLayer(Layer);
		}
		Owner->MapDetail = InMapDetail;
		ZoomScale = 1.0f;
		MinZoomScale = 1.0f;
		MinContainerPosition = FVector2D::ZeroVector;
		MaxContainerPosition = FVector2D::ZeroVector;

		CalMinZoomScale();
		// 重新计算边界
		RecalculateLayoutLimit();

		// 刷新所有 Node 节点，反向旋转
		for (const auto Pair: NodeProxys)
		{
			Pair.Value->SetWorldRotationYaw(Pair.Value->GetWorldRotationYaw());
		}
	}

	void Zoom(float InZoomScale)
	{
		if (InZoomScale <= 0.f) return;

		if (Owner->GetCachedGeometry().Size == FVector2D::ZeroVector)
		{
			CachedScale = InZoomScale;
			return;
		}

		// 重新设置 ContainerCanvas 在 FrameCanvas 中的位置
		const FGeometry& Geometry = FrameCanvas->GetCachedGeometry();
		const FVector2D& FrameLeftTopScreenPosition = USlateBlueprintLibrary::LocalToAbsolute(Geometry, FVector2D::ZeroVector);
		const FVector2D& FrameAbsoluteSize = USlateBlueprintLibrary::GetAbsoluteSize(Geometry);
		const FVector2D& CenterWorldPosition = ScreenPosition2WorldPosition(FrameLeftTopScreenPosition + (FrameAbsoluteSize / 2));

		ZoomScale = FMath::Max(InZoomScale, MinZoomScale);

		RecalculateLayoutLimit();

		// 设置 ContainerCanvas 的Scale
		FWidgetTransform WidgetRenderTransform;
		WidgetRenderTransform.Scale = FVector2D(ZoomScale);
		ContainerCanvas->SetRenderTransform(WidgetRenderTransform.ToSlateRenderTransform());

		// 对 ContainerCanvas 中的 Node 进行反向缩放
		const FVector2D NodeScale = FVector2D::UnitVector / ZoomScale;
		for (const auto Pair : NodeProxys)
		{
			Pair.Key->SetRenderScale(NodeScale);
		}

		FocusToWorldPosition(CenterWorldPosition);
	}

	void FocusToWorldPosition(const FVector2D& InWorldPosition)
	{
		// 需要反向设置 Container 坐标，所以 * -1
		const FVector2D& ContainerPosition = WorldPosition2MapPosition(InWorldPosition) * ZoomScale * -1;
		SetContainerPosition(ContainerPosition);
	}

	FVector2D RotatePoint(const FVector2D& Point,float angle) const
	{
		FVector2D Center = Owner->MapDetail.MapSize/2;
		FVector2D Delta = Point - Center;
		float Radians = FMath::DegreesToRadians(angle);
		float X = Delta.X * FMath::Cos(Radians) - Delta.Y * FMath::Sin(Radians) + Center.X;
		float Y = Delta.X * FMath::Sin(Radians) + Delta.Y * FMath::Cos(Radians) + Center.Y;
		return FVector2D(X,Y);
	}

	void FocusToMapNode(UEasyMapNodeProxy* InNodeProxy, bool InIsLockNode)
	{
		const FVector2D& WorldPosition = InNodeProxy->GetWorldPosition();
		FocusToWorldPosition(WorldPosition);
		if (InIsLockNode)
		{
			LockedNode = InNodeProxy;
		}
	}

	void CancelFocusLock()
	{
		LockedNode = nullptr;
	}

	UEasyMapNodeProxy* AddSimpleNode(const FSlateBrush& ImageBrush, const FVector2D& InWorldPosition, int32 InZOrder = 1)
	{
		UButton* NodeWidget = NewObject<UButton>(Owner);
		FButtonStyle Style;
		Style.Normal = ImageBrush;
		Style.Hovered = ImageBrush;
		Style.Pressed = ImageBrush;
		Style.Pressed.ImageSize = Style.Pressed.ImageSize * Owner->SimpleNodePressedScale;
		Style.Pressed.TintColor = Owner->SimpleNodePressedTintColor;
		NodeWidget->SetStyle(Style);

		UEasyMapNodeProxy* NodeProxy = AddCustomNode(NodeWidget, UEasyMapNodeProxy::StaticClass(), InZOrder);
		NodeProxy->SetWorldPosition(InWorldPosition);

		NodeWidget->OnClicked.AddDynamic(NodeProxy, &UEasyMapNodeProxy::HandleWidgetClicked);
		return NodeProxy;
	}

	UEasyMapNodeProxy* AddCustomNode(UWidget* InNodeWidget, const TSubclassOf<UEasyMapNodeProxy> InProxyClass = UEasyMapNodeProxy::StaticClass(), int32 InZOrder = 1)
	{
		if(!IsValid(InNodeWidget) || !IsValid(InProxyClass))
		{
			return nullptr;
		}
		UCanvasPanelSlot* CanvasPanelSlot = Owner->AddChildToCanvas(InNodeWidget);
		CanvasPanelSlot->SetAlignment(DEFAULT_MAP_NODE_ALIGNMENT);
		CanvasPanelSlot->SetAutoSize(true);
		CanvasPanelSlot->SetZOrder(InZOrder);
		InNodeWidget->SetRenderScale(FVector2D::UnitVector / ZoomScale);

		UEasyMapNodeProxy* NodeProxy = NewObject<UEasyMapNodeProxy>(Owner, InProxyClass);
		NodeProxy->Init(Owner, InNodeWidget);
		NodeProxys.Add(InNodeWidget, NodeProxy);
		return NodeProxy;
	}

	void AddCustomNodeInstance(UWidget* InNodeWidget, UEasyMapNodeProxy* InProxyNode, int32 InZOrder = 1)
	{
		if(!IsValid(InNodeWidget) || !IsValid(InProxyNode))
		{
			return;
		}
		UCanvasPanelSlot* CanvasPanelSlot = Owner->AddChildToCanvas(InNodeWidget);
		CanvasPanelSlot->SetAlignment(DEFAULT_MAP_NODE_ALIGNMENT);
		CanvasPanelSlot->SetAutoSize(true);
		CanvasPanelSlot->SetZOrder(InZOrder);
		InNodeWidget->SetRenderScale(FVector2D::UnitVector / ZoomScale);

		InProxyNode->Init(Owner, InNodeWidget);
		NodeProxys.Add(InNodeWidget, InProxyNode);
	}

	void RemoveAllNode()
	{
		TArray NodeProxyList = NodeProxys.Array();
		for(const auto Pair:NodeProxyList)
		{
			Pair.Key->RemoveFromParent();
		}
		NodeProxys.Empty();
	}

	float GetMinZoomScale() const
	{
		return MinZoomScale;
	}

	FVector2D WorldPosition2MapPosition(const FVector2D& WorldPosition) const
	{
		FVector2D MapPosition = (WorldPosition - Owner->MapDetail.WorldOffset) / Owner->MapDetail.WorldSize * Owner->MapDetail.MapSize;
		return RotatePoint(MapPosition , Owner->MapDetail.MapAngleOffset);
	}

	void ReleaseSlateResources(bool bReleaseChildren)
	{
		bIsTickable = false;

		// 释放 Timer
		if (DelayConstructHandle.IsValid())
		{
			Owner->GetWorld()->GetTimerManager().ClearTimer(DelayConstructHandle);
		}

		// 释放 FrameCanvas & ContainerCanvas
		FrameCanvas.Reset();
		ContainerCanvas.Reset();

		NodeProxys.Empty();
		LayerImageWidgets.Empty();

		Owner->Super::ReleaseSlateResources(bReleaseChildren);
	}

	TSharedRef<SWidget> RebuildWidget()
	{
		Owner->Super::RebuildWidget();

		ContainerCanvas = Owner->MyCanvas;

		// 构造一个框体 Canvas, 用于让 ContainerCanvas(MyCanvas) 在内部移动
		FrameCanvas = SNew(SConstraintCanvas)
			+ SConstraintCanvas::Slot()
			  .Expose(ContainerSlot)
			  .Anchors(DEFAULT_CONTAINER_ANCHORS)
			  .Alignment(DEFAULT_CONTAINER_ALIGNMENT)
			  .Offset(DEFAULT_CONTAINER_OFFSET)
			  .AutoSize(true)
			[
				ContainerCanvas.ToSharedRef()
			];

		// 绑定鼠标移动事件，用于地图的拖拽与点击判定
		FrameCanvas->SetOnMouseButtonDown(FPointerEventHandler::CreateRaw(this, &Implement::HandleMouseButtonDown));
		FrameCanvas->SetOnMouseButtonUp(FPointerEventHandler::CreateRaw(this, &Implement::HandleMouseButtonUp));
		FrameCanvas->SetOnMouseMove(FPointerEventHandler::CreateRaw(this, &Implement::HandleMouseMove));

		return FrameCanvas.ToSharedRef();
	}

	void SynchronizeProperties()
	{
		Owner->Super::SynchronizeProperties();

		DelayConstructMapPanel();
	}

	void OnSlotRemoved(UPanelSlot* PanelSlot)
	{
		NodeProxys.Remove(PanelSlot->Content);
		Owner->Super::OnSlotRemoved(PanelSlot);
	}

private:
	void DelayConstructMapPanel()
	{
		// 临时写法，理论上延迟两帧会有数据
		UWorld* World = Owner->GetWorld();
		if(IsValid(World))
		{
			FTimerManager& TimerManager = Owner->GetWorld()->GetTimerManager();
			TimerManager.ClearTimer(DelayConstructHandle);
			if(Owner->GetCachedGeometry().Size == FVector2D::ZeroVector)
			{
				DelayConstructHandle = TimerManager.SetTimerForNextTick(FTimerDelegate::CreateRaw(this, &Implement::DelayConstructMapPanel));
				return;
			}
		}
		bIsTickable = true;

		// 属性变更时重新设置 MapDetail 相关逻辑
		SetMapDetail(Owner->MapDetail);

		// 刷新 ContainerCanvas 位置
		const FMargin Offset = ContainerSlot->GetOffset();
		SetContainerPosition(FVector2D(Offset.Left, Offset.Top));

		if (CachedScale > 0.f)
		{
			Zoom(CachedScale);
		}
	}

	void CalMinZoomScale()
	{
		const FVector2D& FrameSize = FrameCanvas->GetCachedGeometry().GetLocalSize();
		const FVector2D& ZoomScaleXY = FrameSize / Owner->MapDetail.MapSize;
		MinZoomScale = ZoomScaleXY.GetMax();
		if (ZoomScale < MinZoomScale)
		{
			Zoom(MinZoomScale);
		}
	}

	void AddMapLayer(const FMapLayer& Layer)
	{
		UImage* Image = NewObject<UImage>(Owner);
		Image->SetBrush(Layer.Brush);
		LayerImageWidgets.Add(Image);

		UCanvasPanelSlot* CanvasPanelSlot = Owner->AddChildToCanvas(Image);
		CanvasPanelSlot->SetPosition(Layer.Position);
		CanvasPanelSlot->SetSize(Layer.Size);
		CanvasPanelSlot->SetZOrder(Layer.ZOrder);
	}

private:
	/////////////////////////////////////////////////////////////
	// Map Interaction

	FReply HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
	{
		bIsMouseDown = true;
		MouseDownPosition = MouseEvent.GetScreenSpacePosition();
		MouseDownTime = FDateTime::Now().ToUnixTimestamp();
		return FReply::Handled().CaptureMouse(FrameCanvas.ToSharedRef());
	}

	FReply HandleMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
	{
		if (bIsMouseDown == false || LockedNode != nullptr) return FReply::Unhandled();
		const FMargin& SlotOffset = ContainerSlot->GetOffset();
		const FVector2D& ScreenDelta = MouseEvent.GetCursorDelta();
		const FVector2D& FrameAbsoluteSize = USlateBlueprintLibrary::GetAbsoluteSize(FrameCanvas->GetCachedGeometry());
		const FVector2D& FrameLocalSize = FrameCanvas->GetCachedGeometry().GetLocalSize();
		const FVector2D& MapOffset = FrameLocalSize / FrameAbsoluteSize * ScreenDelta;
		const FVector2D& FinalOffset = FVector2D(SlotOffset.Left + MapOffset.X, SlotOffset.Top + MapOffset.Y);

		SetContainerPosition(FinalOffset);
		return FReply::Handled();
	}

	FReply HandleMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
	{
		do
		{
			if (!bIsMouseDown) break;
			if (!Owner->OnClickedMap.IsBound()) break;
			if (FDateTime::Now().ToUnixTimestamp() - MouseDownTime > MAX_CLICK_INTERVAL) break;

			const FVector2D& MousePosition = MouseEvent.GetScreenSpacePosition();
			if (FVector2D::DistSquared(MousePosition, MouseDownPosition) > MAX_CLICK_POINTER_DELTA) break;

			Owner->OnClickedMap.Broadcast(ScreenPosition2WorldPosition(MousePosition));
		}
		while (false);

		bIsMouseDown = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	FVector2D ScreenPosition2WorldPosition(const FVector2D& ScreenPosition) const
	{
		const FGeometry& ContainerGeometry = ContainerCanvas->GetCachedGeometry();
		const FVector2D& RotatedMapPosition = USlateBlueprintLibrary::AbsoluteToLocal(ContainerGeometry, ScreenPosition);
		const FVector2D& MapPosition = RotatePoint(RotatedMapPosition,Owner->MapDetail.MapAngleOffset * -1);
		return MapPosition2WorldPosition(MapPosition);
	}

	FVector2D MapPosition2WorldPosition(const FVector2D& MapPosition) const
	{
		return MapPosition / Owner->MapDetail.MapSize * Owner->MapDetail.WorldSize + Owner->MapDetail.WorldOffset;
	}

	void RecalculateLayoutLimit()
	{
		const FVector2D& FrameSize = FrameCanvas->GetCachedGeometry().GetLocalSize();
		const FVector2D& ScaledMapSize = Owner->MapDetail.MapSize * ZoomScale;

		MaxContainerPosition = FrameSize / 2 * (-1);
		MinContainerPosition = (ScaledMapSize - FrameSize / 2) * (-1);
	}

	// 设置 ContainerCanvas 在 FrameCanvas 中的坐标
	void SetContainerPosition(const FVector2D& ContainerPosition)
	{
		FMargin RawOffset = ContainerSlot->GetOffset();
		RawOffset.Left = FMath::Clamp(ContainerPosition.X, MinContainerPosition.X, MaxContainerPosition.X);
		RawOffset.Top = FMath::Clamp(ContainerPosition.Y, MinContainerPosition.Y, MaxContainerPosition.Y);
		ContainerSlot->SetOffset(RawOffset);
	}

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObjects(LayerImageWidgets);
		Collector.AddReferencedObjects(NodeProxys);
	}

	virtual FString GetReferencerName() const override
	{
		return TEXT("UMapPanel");
	}

	// FTickableGameObject interface
	virtual void Tick(float DeltaTime) override
	{
		for (const auto Pair : NodeProxys)
		{
			Pair.Value->OnTick(DeltaTime);
		}

		if (LockedNode)
		{
			FocusToMapNode(LockedNode, true);
		}
	}

	virtual bool IsTickable() const override
	{
		return bIsTickable;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UMapPanel, STATGROUP_Tickables);
	}

private:
	UEasyMapPanel* Owner;
	AActor* FocusingActor = nullptr;
	UEasyMapNodeProxy* LockedNode = nullptr;

	SConstraintCanvas::FSlot* ContainerSlot = nullptr;
	TSharedPtr<SConstraintCanvas> FrameCanvas;
	TSharedPtr<SConstraintCanvas> ContainerCanvas;

	TArray<TObjectPtr<UImage>> LayerImageWidgets;
	TMap<TObjectPtr<UWidget>, TObjectPtr<UEasyMapNodeProxy>> NodeProxys;

	float ZoomScale = 1.f;
	float MinZoomScale = 1.f;
	float CachedScale = 0.f;

	bool bIsMouseDown = false;
	bool bIsTickable = false;

	int64 MouseDownTime = 0;

	FVector2D MouseDownPosition = FVector2D::ZeroVector;
	FVector2D MinContainerPosition = FVector2D::ZeroVector;
	FVector2D MaxContainerPosition = FVector2D::ZeroVector;

	FTimerHandle DelayConstructHandle;
};

///////////////////////////////////////////////////////////////
UEasyMapPanel::UEasyMapPanel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Impl(new Implement(this))
{
}

void UEasyMapPanel::SetMapDetail(const FMapDetail& InMapDetail)
{
	Impl->SetMapDetail(InMapDetail);
}

void UEasyMapPanel::Zoom(float InScale)
{
	Impl->Zoom(InScale);
}

void UEasyMapPanel::FocusToWorldPosition(const FVector2D& InWorldPosition)
{
	Impl->FocusToWorldPosition(InWorldPosition);
}

void UEasyMapPanel::FocusToMapNode(UEasyMapNodeProxy* InNodeProxy, bool InIsLockNode)
{
	Impl->FocusToMapNode(InNodeProxy, InIsLockNode);
}

void UEasyMapPanel::CancelFocusLock()
{
	Impl->CancelFocusLock();
}

UEasyMapNodeProxy* UEasyMapPanel::AddSimpleNode(const FSlateBrush& InImageBrush, const FVector2D& InWorldPosition, int32 InZOrder)
{
	return Impl->AddSimpleNode(InImageBrush, InWorldPosition, InZOrder);
}

UEasyMapNodeProxy* UEasyMapPanel::AddCustomNode(UWidget* InNodeWidget, TSubclassOf<UEasyMapNodeProxy> InProxyClass, int32 InZOrder)
{
	return Impl->AddCustomNode(InNodeWidget, InProxyClass, InZOrder);
}

void UEasyMapPanel::AddCustomNodeInstance(UWidget* InNodeWidget, UEasyMapNodeProxy* InProxyNode, int32 InZOrder)
{
	Impl->AddCustomNodeInstance(InNodeWidget, InProxyNode, InZOrder);
}

void UEasyMapPanel::RemoveAllNode()
{
	Impl->RemoveAllNode();
}

float UEasyMapPanel::GetMinZoomScale()
{
	return Impl->GetMinZoomScale();
}

FVector2D UEasyMapPanel::WorldPosition2MapPosition(const FVector2D& InWorldLocation) const
{
	return Impl->WorldPosition2MapPosition(InWorldLocation);
}

void UEasyMapPanel::ReleaseSlateResources(bool bReleaseChildren)
{
	Impl->ReleaseSlateResources(bReleaseChildren);
}

TSharedRef<SWidget> UEasyMapPanel::RebuildWidget()
{
	return Impl->RebuildWidget();
}

void UEasyMapPanel::SynchronizeProperties()
{
	Impl->SynchronizeProperties();
}

void UEasyMapPanel::OnSlotRemoved(UPanelSlot* PanelSlot)
{
	Impl->OnSlotRemoved(PanelSlot);
}
