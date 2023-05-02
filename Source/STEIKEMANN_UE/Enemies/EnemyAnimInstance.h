// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyClasses_Enums.h"
#include "EnemyAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;


	UPROPERTY(BlueprintReadOnly)
		class ASmallEnemy* Owner;

public:	// Variables
	UPROPERTY(BlueprintReadWrite)
		EEnemyAnimState m_AnimState;

	UPROPERTY(BlueprintReadOnly)
		float Speed{};
	UPROPERTY(BlueprintReadOnly)
		FVector Velocity{};

	void SetLaunchedInAir(FVector direction);
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Launched_SpinSpeed{ 1000.f };
	UPROPERTY(BlueprintReadOnly)
		FRotator Launched_SpinAngle{};
	UPROPERTY(BlueprintReadOnly)
		bool bIsLaunchedInAir{};
	UPROPERTY(BlueprintReadOnly)
		bool bIsSleeping{};
	UPROPERTY(BlueprintReadOnly)
		bool bSpottingPlayer{};
};
