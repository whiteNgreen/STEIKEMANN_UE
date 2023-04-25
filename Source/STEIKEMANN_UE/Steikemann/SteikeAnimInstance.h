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
		class ASteikemannCharacter* SteikeOwner{ nullptr };
		//TWeakObjectPtr<class ASteikemannCharacter> SteikeOwner{ nullptr };

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

	/* Deciding when the lower body should blend to the default state machine _ IN AIR ONLY */
	UPROPERTY(BlueprintReadOnly)
		bool bBlendLowerBody_Air{};

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
		float WallInputAngle{};

#pragma endregion //WallSticking

#pragma region LedgeGrab
	UPROPERTY(BlueprintReadOnly)
		bool bLedgeGrab{};
#pragma endregion //LedgeGrab

#pragma region Grappling
	UPROPERTY(BlueprintReadWrite)
		bool bGrappling{};
	UPROPERTY(BlueprintReadOnly)
		FVector GrappledTarget{};
	UPROPERTY(BlueprintReadOnly)
		bool bControlRigLerp{};
	UPROPERTY(BlueprintReadOnly)
		float ControlRig_LeafAlpha{};
#pragma endregion //Grappling

#pragma region GroundPound
	UPROPERTY(BlueprintReadOnly)
		bool bGroundPound{};
#pragma endregion //GroundPound
#pragma region FootIK_ControlRig
	UPROPERTY(BlueprintReadOnly)
		FTransform Hip_BaseTransform = FTransform(FRotator(0, 90, 0), FVector(0, 0, 90), FVector(4.5f));
	UPROPERTY(BlueprintReadOnly)
		FTransform IKFoot_BaseTransform_L;
	UPROPERTY(BlueprintReadOnly)
		FTransform IKFoot_BaseTransform_R;


	//UPROPERTY(BlueprintReadOnly)
	//	bool bIK_Foot_L_Active{};
	////UPROPERTY(BlueprintReadOnly)
	////	FVector IK_Foot_L_SurfaceLocation;
	////UPROPERTY(BlueprintReadOnly)
	////	FVector IK_Foot_L_SurfaceDirection;
	////UPROPERTY(BlueprintReadOnly)
	////	FQuat IK_Foot_L_NewRotation;

	//UPROPERTY(BlueprintReadOnly)
	//	bool bIK_Foot_R_Active{};

	//UPROPERTY(BlueprintReadOnly)
	//	FVector IK_Foot_R_SurfaceLocation;
	//UPROPERTY(BlueprintReadOnly)
	//	FVector IK_Foot_R_SurfaceDirection;
	//UPROPERTY(BlueprintReadOnly)
	//	FQuat IK_Foot_R_NewRotation;

	//FQuat GetNewFootIKDirection(const FVector CurrentDirection, const FVector SurfaceDirection);
#pragma endregion //FootIK_ControlRig
};
