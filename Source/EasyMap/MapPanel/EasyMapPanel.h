// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EasyMap/Node/EasyMapNodeProxy.h"
#include "Components/CanvasPanel.h"
#include "EasyMapPanel.generated.h"

USTRUCT(BlueprintType)
struct EASYMAP_API FMapLayer
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FSlateBrush Brush;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D Size = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector2D Position = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 ZOrder = -1;
};

USTRUCT(BlueprintType)
struct EASYMAP_API FMapDetail
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D MapSize = FVector2D::ZeroVector;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MapAngleOffset = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D WorldSize = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D WorldOffset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FMapLayer> Layers;
};

UCLASS()
class EASYMAP_API UEasyMapPanel : public UCanvasPanel
{
	GENERATED_UCLASS_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SetMapDetail(const FMapDetail& InMapDetail);

    UFUNCTION(BlueprintCallable)
	void Zoom(float InScale);

	UFUNCTION(BlueprintCallable)
	void FocusToWorldPosition(const FVector2D& InWorldPosition);

    UFUNCTION(BlueprintCallable)
    void FocusToMapNode(UEasyMapNodeProxy* InNodeScript, bool InIsLockNode);

    UFUNCTION(BlueprintCallable)
    void CancelFocusLock();

	UFUNCTION(BlueprintCallable)
	UEasyMapNodeProxy* AddSimpleNode(const FSlateBrush& InImageBrush, const FVector2D& InWorldPosition, int32 InZOrder = 1);

	UFUNCTION(BlueprintCallable)
	UEasyMapNodeProxy* AddCustomNode(UWidget* InNodeWidget, TSubclassOf<UEasyMapNodeProxy> InScriptClass, int32 InZOrder = 1);

	UFUNCTION(BlueprintCallable)
	void AddCustomNodeInstance(UWidget* InNodeWidget, UEasyMapNodeProxy* InScriptNode, int32 InZOrder = 1);

	UFUNCTION(BlueprintCallable)
	void RemoveAllNode();

	UFUNCTION(BlueprintPure)
	float GetMinZoomScale();

protected:
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Style")
    FMapDetail MapDetail;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(DisplayName="Pressed Tint Color"), Category = "Style|SimpleNode")
	FSlateColor SimpleNodePressedTintColor = FSlateColor(FLinearColor::Gray);
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, meta=(DisplayName="Pressed Scale"), Category = "Style|SimpleNode")
	FVector2D SimpleNodePressedScale = FVector2D(0.95f);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClickedMap, const FVector2D&, WorldPosition);
    UPROPERTY(BlueprintAssignable)
    FOnClickedMap OnClickedMap;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClickedMapNode, UEasyMapNodeProxy*, NodeScript);
    UPROPERTY(BlueprintAssignable)
	FOnClickedMapNode OnClickedMapNode;
	
public:
	FVector2D WorldPosition2MapPosition(const FVector2D& InWorldLocation) const;

	float GetMapAngleOffset() const { return MapDetail.MapAngleOffset; };

	FOnClickedMap GetOnClickedMap() const { return OnClickedMap; };
	
	FOnClickedMapNode GetOnClickedMapNode() const { return OnClickedMapNode; };

protected:
    // UVisual interface
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

    // UWidget interface
    virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

	// UPanelWidget interface
	virtual void OnSlotRemoved(class UPanelSlot* PanelSlot) override;

private:
    struct Implement;
    TSharedPtr<Implement> Impl;
};
