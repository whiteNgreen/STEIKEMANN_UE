// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SteikemannCharMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API USteikemannCharMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variables")
	float CharacterFriction{ 15.f };

	class ASteikemannCharacter* CharacterOwner_Steikemann{ nullptr };

	bool DoJump(bool bReplayingMoves) override;

public: // Slipping
	UPROPERTY(BlueprintReadWrite)
	float Traced_GroundFriction;
};
