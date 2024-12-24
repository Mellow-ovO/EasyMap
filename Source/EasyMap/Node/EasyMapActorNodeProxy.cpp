// Fill out your copyright notice in the Description page of Project Settings.


#include "EasyMap/Node/EasyMapActorNodeProxy.h"

void UEasyMapActorNodeProxy::OnTick(float DeltaSeconds)
{
	Super::OnTick(DeltaSeconds);
	if (bAutoUpdateMapData)
	{
		UpdateActorMapData();
	}
}

void UEasyMapActorNodeProxy::SetActor(AActor* InActor)
{
	Actor = InActor;
	K2_OnActorSet(InActor);
	UpdateActorMapData();
}

void UEasyMapActorNodeProxy::SetIsUpdatePosition(bool InIsUpdatePosition)
{
	bIsUpdateRotation = InIsUpdatePosition;
	UpdateActorMapData();
}

void UEasyMapActorNodeProxy::SetIsUpdateRotation(bool InIsUpdateRotation)
{
	bIsUpdateRotation = InIsUpdateRotation;
	UpdateActorMapData();
}

void UEasyMapActorNodeProxy::UpdateActorMapData()
{
	if (!Actor.IsValid()) return;

	if (bIsUpdatePosition)
	{
		const FVector& ActorLocation = Actor->GetActorLocation();
		SetWorldPosition(FVector2D(ActorLocation));
	}

	if (bIsUpdateRotation)
	{
		const float Yaw = Actor->GetActorRotation().Yaw;
		SetWorldRotationYaw(Yaw);
	}
}
