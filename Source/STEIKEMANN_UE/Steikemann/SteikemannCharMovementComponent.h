// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../DebugMacros.h"

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Friction")
	float CharacterFriction{ 15.f };

	class ASteikemannCharacter* CharacterOwner_Steikemann{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump")
		bool bJumping{};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump")
		float JumpInterpSpeed{ 2.f };

	bool DoJump(bool bReplayingMoves) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		float GravityScaleOverride{ 2.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		float GravityScaleOverride_InterpSpeed{ 2.f };

public: // Slipping
	UPROPERTY(BlueprintReadWrite)
	float Traced_GroundFriction;
};
