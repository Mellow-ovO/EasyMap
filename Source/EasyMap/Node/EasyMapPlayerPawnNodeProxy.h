// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EasyMapActorNodeProxy.h"
#include "EasyMapPlayerPawnNodeProxy.generated.h"

UCLASS()
class UEasyMapPlayerPawnNodeProxy : public UEasyMapActorNodeProxy
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, BlueprintSetter="SetPlayerIndex")
	int32 PlayerIndex = 0;

public:
	UFUNCTION(BlueprintCallable)
	void SetPlayerIndex(int32 InPlayerIndex);
	
protected:
	virtual void Init(UEasyMapPanel* InMapPanel, UWidget* InWidget) override;
};
