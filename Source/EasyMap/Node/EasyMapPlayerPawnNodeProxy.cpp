// Fill out your copyright notice in the Description page of Project Settings.


#include "EasyMap/Node/EasyMapPlayerPawnNodeProxy.h"

#include "Kismet/GameplayStatics.h"

void UEasyMapPlayerPawnNodeProxy::SetPlayerIndex(int32 InPlayerIndex)
{
	PlayerIndex = InPlayerIndex;
	SetActor(UGameplayStatics::GetPlayerPawn(this, PlayerIndex));
}

void UEasyMapPlayerPawnNodeProxy::Init(UEasyMapPanel* InMapPanel, UWidget* InWidget)
{
	Super::Init(InMapPanel, InWidget);
	SetActor(UGameplayStatics::GetPlayerPawn(this, PlayerIndex));
}
