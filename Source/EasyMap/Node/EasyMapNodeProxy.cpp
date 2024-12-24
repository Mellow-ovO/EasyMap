// Fill out your copyright notice in the Description page of Project Settings.

#include "EasyMap/Node/EasyMapNodeProxy.h"
#include "EasyMap/MapPanel/EasyMapPanel.h"

void UEasyMapNodeProxy::Init(UEasyMapPanel* InMapPanel, UWidget* InWidget)
{
	MapPanel = InMapPanel;
	Widget = InWidget;
	K2_OnInit(InMapPanel,InWidget);
}

void UEasyMapNodeProxy::OnTick(float DeltaSeconds)
{
	K2_OnTick(DeltaSeconds);
}

void UEasyMapNodeProxy::HandleWidgetClicked()
{
	if (MapPanel.IsValid())
	{
		MapPanel->GetOnClickedMapNode().Broadcast(this);
	}
	K2_OnNodeClicked();
}

UWidget* UEasyMapNodeProxy::GetWidget()
{
	return Widget.Get();
}

UCanvasPanelSlot* UEasyMapNodeProxy::GetWidgetSlot()
{
	if (Widget.IsValid())
	{
		return Cast<UCanvasPanelSlot>(Widget->Slot);
	}
	return nullptr;
}

void UEasyMapNodeProxy::SetWorldPosition(const FVector2D& InWorldPosition)
{
	if (!MapPanel.IsValid()) return;
	WorldPosition = InWorldPosition;
	const FVector2D& MapPosition = MapPanel->WorldPosition2MapPosition(WorldPosition);
	GetWidgetSlot()->SetPosition(MapPosition);
}

FVector2D UEasyMapNodeProxy::GetWorldPosition()
{
	return WorldPosition;
}

void UEasyMapNodeProxy::SetWorldRotationYaw(float InYaw)
{
	WorldRotationYaw = InYaw;
	if (Widget.IsValid())
	{
		Widget->SetRenderTransformAngle(InYaw + NodeAngleOffset);
	}
}

float UEasyMapNodeProxy::GetWorldRotationYaw()
{
	if (Widget.IsValid())
	{
		return Widget->GetRenderTransformAngle();
	}
	return WorldRotationYaw;
}

void UEasyMapNodeProxy::SetNodeAngleOffset(float InAngleOffset)
{
	NodeAngleOffset = InAngleOffset;
	SetWorldRotationYaw(GetWorldRotationYaw());
}

float UEasyMapNodeProxy::GetNodeAngleOffset()
{
	return NodeAngleOffset;
}

void UEasyMapNodeProxy::RemoveFromParent()
{
	if (Widget.IsValid())
	{
		Widget->RemoveFromParent();
	}
	MapPanel.Reset();
	Widget.Reset();
	MarkAsGarbage();
}
