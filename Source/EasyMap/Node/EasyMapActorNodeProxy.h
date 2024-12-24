// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EasyMapNodeProxy.h"
#include "EasyMapActorNodeProxy.generated.h"

UCLASS()
class UEasyMapActorNodeProxy : public UEasyMapNodeProxy
{
	GENERATED_BODY()

	virtual void OnTick(float DeltaSeconds) override;;

public:
	UFUNCTION(BlueprintCallable)
	virtual void SetActor(AActor* InActor);

	UFUNCTION(BlueprintCallable)
	void SetIsUpdatePosition(bool InIsUpdatePosition);

	UFUNCTION(BlueprintCallable)
	void SetIsUpdateRotation(bool InIsUpdateRotation);

	UFUNCTION(BlueprintImplementableEvent)
	void K2_OnActorSet(AActor* InActor);

protected:
	UPROPERTY(BlueprintReadonly, EditAnywhere)
	bool bIsUpdatePosition = true;
	
	UPROPERTY(BlueprintReadonly, EditAnywhere)
	bool bIsUpdateRotation = false;
	
	UPROPERTY(BlueprintReadonly, EditAnywhere)
	bool bAutoUpdateMapData = true;

protected:
	virtual void UpdateActorMapData();

protected:
	TWeakObjectPtr<AActor> Actor;
};