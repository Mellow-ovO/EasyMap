// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Widget.h"
#include "EasyMapNodeProxy.generated.h"

class UEasyMapPanel;
class UWidget;

UCLASS(Blueprintable)
class EASYMAP_API UEasyMapNodeProxy : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	UWidget* GetWidget();

	UFUNCTION(BlueprintPure)
	UCanvasPanelSlot* GetWidgetSlot();

	UFUNCTION(BlueprintCallable)
	void SetWorldPosition(const FVector2D& InWorldPosition);
	
	UFUNCTION(BlueprintPure)
	FVector2D GetWorldPosition();

	UFUNCTION(BlueprintCallable)
	void SetWorldRotationYaw(float InYaw);
	
	UFUNCTION(BlueprintPure)
	float GetWorldRotationYaw();
	
	UFUNCTION(BlueprintCallable)
	void SetNodeAngleOffset(float InAngleOffset);
	
	UFUNCTION(BlueprintPure)
	float GetNodeAngleOffset();

	UFUNCTION(BlueprintCallable)
	void RemoveFromParent();

public:
	virtual void Init(UEasyMapPanel* InMapPanel, UWidget* InWidget);
	
	virtual void OnTick(float DeltaSeconds);;

	UFUNCTION(BlueprintCallable)
	void HandleWidgetClicked();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnTick(float DeltaSeconds);

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnNodeClicked();

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnInit(UEasyMapPanel* InMapPanel, UWidget* InWidget);

protected:
	TWeakObjectPtr<UEasyMapPanel> MapPanel;
	TWeakObjectPtr<UWidget> Widget;
	FVector2D WorldPosition = FVector2D::ZeroVector;
	float WorldRotationYaw = 0.f;
	float NodeAngleOffset = 0.f;
};
