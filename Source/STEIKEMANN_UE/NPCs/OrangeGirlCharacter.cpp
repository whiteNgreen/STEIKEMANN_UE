// Fill out your copyright notice in the Description page of Project Settings.


#include "OrangeGirlCharacter.h"
#include "../DebugMacros.h"

AOrangeGirlCharacter::AOrangeGirlCharacter()
{
}

void AOrangeGirlCharacter::BeginPlay()
{
	Super::BeginPlay();
	GameplayTags.AddTag(Tag::OrangeGirl());
}

void AOrangeGirlCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AOrangeGirlCharacter::Gen_ReceiveAttack(const FVector Direction, const float Strength, const EAttackType AType, const float Delaytime)
{
	if (!CanBeAttacked()) 
		return;
	bAICanBeDamaged = false;

	PRINTLONG(2.f, "Orange girl ATTACKED");

	TimerManager.SetTimer(FTHCanBeDamaged, this, &IAttackInterface::ResetCanbeDamaged, 0.5f);
}
