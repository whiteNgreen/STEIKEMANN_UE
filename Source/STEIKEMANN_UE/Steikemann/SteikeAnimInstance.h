// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "../DebugMacros.h"

#include "SteikeAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class USteikeAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeBeginPlay() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly)
		TWeakObjectPtr<class ASteikemannCharacter> SteikeOwner{ nullptr };
		//class ASteikemannCharacter* SteikeOwner{ nullptr };

	/* Walking Speed */
	UPROPERTY(BlueprintReadOnly)
		float Speed{};
	/* Falling Speed */
	UPROPERTY(BlueprintReadOnly)
		float VerticalSpeed{};

	UPROPERTY(BlueprintReadOnly)
		float HorizontalSpeed{};

	/* Check if player is freefalling */
	UPROPERTY(BlueprintReadOnly)
		bool bFalling{};
	/* Check if the player is on the ground. 
		Used to check when the character lands after free falling */
	UPROPERTY(BlueprintReadOnly)
		bool bOnGround{};

#pragma region Jump
	
	/* Checks the Character script if Jump button is pressed */
	UPROPERTY(BlueprintReadOnly)
		bool bJumping{};
	/* The Jump animations anim notify activates this bool to start the jump sequence */
	UPROPERTY(BlueprintReadWrite)
		bool bActivateJump{};



#pragma endregion //Jump

#pragma region WallSticking

	UPROPERTY(BlueprintReadOnly)
		bool bOnWall{};

	UPROPERTY(BlueprintReadOnly)
		bool bStickingToWall{};

	UPROPERTY(BlueprintReadOnly)
		float OnWallRotation{};

#pragma endregion //WallSticking

#pragma region LedgeGrab

	UPROPERTY(BlueprintReadOnly)
		bool bLedgeGrab{};



#pragma endregion //LedgeGrab

	UPROPERTY(BlueprintReadWrite)
		bool bGrappling{};

	/* Checks if character is dashing */
	UPROPERTY(BlueprintReadOnly)
		bool bDashing{};

};
