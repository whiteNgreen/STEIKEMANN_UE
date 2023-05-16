// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawnerAnimInstance.h"

void UEnemySpawnerAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);
	
}

void UEnemySpawnerAnimInstance::TL_Anim_HitDirection(float value)
{
	Anim_HitDirection = value;
}
