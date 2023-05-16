// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemySpawnerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API UEnemySpawnerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	virtual void NativeUpdateAnimation(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
		FVector HitDirection;
	UPROPERTY(BlueprintReadOnly)
		float Anim_HitDirection;
	UFUNCTION()
		void TL_Anim_HitDirection(float value);
};
