// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../DebugMacros.h"

#include "SteikemannCharMovementComponent.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum ECustomMovementMode
{
	MOVECustom_None				UMETA(DisplayName = "None"),
	MOVECustom_Dash				UMETA(DisplayName = "Dash"),
	MOVECustom_WallSticking		UMETA(DisplayName = "Wallsticking"),
	MOVECustom_Grappling		UMETA(DisplayName = "Grappling"),
};

UCLASS()
class STEIKEMANN_UE_API USteikemannCharMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class ASteikemannCharacter* CharacterOwner_Steikemann{ nullptr };
	
	TEnumAsByte<enum ECustomMovementMode> CustomMovementMode;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Friction")
	float CharacterFriction{ 15.f };


#pragma region Gravity

	/* Gravity over time while character is in the air */
		/* The Base gravity scale override */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		float GravityScaleOverride UMETA(DisplayName = "Gravity Scale Override") { 2.f };
		/* Gravity scale during freefall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		float GravityScaleOverride_Freefall UMETA(DisplayName = "Freefall Gravity") { 2.f };
		/* Interpolation speed between gravityscale override and freefall gravity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		float GravityScaleOverride_InterpSpeed{ 2.f };

#pragma endregion //Gravity

#pragma region Jump
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump")
		bool bJumping{};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump")
		float JumpInterpSpeed{ 2.f };

	bool DoJump(bool bReplayingMoves) override;

#pragma endregion //Jump

#pragma region Bounce
	void Bounce(FVector surfacenormal);

#pragma endregion //Bounce

#pragma region Dash
	
	float fPreDashTimerLength{};
	float fPreDashTimer{};
	float fDashTimerLength{};
	float fDashTimer{};
	float fDashLength{};
	FVector DashDirection;

	void Start_Dash(float preDashTime, float dashTime, float dashLength, FVector dashdirection);
	void Update_Dash(float deltaTime);
#pragma endregion //Dash

#pragma region Wall Jump

	/* If the characters velocity exceeds this value, they cannot stick to a wall */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Wall Jump")
		float WallJump_MaxStickingSpeed UMETA(DisplayName = "Max Stickable Speed") { 50.f };
	/* How much the velocity is lowered each tick when they touch a wall at high speeds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Wall Jump")
		float WallJump_WalltouchSlow UMETA(DisplayName = "Velocity Slowdown") { 100.f };
	
	//bool bTouchingWall{};
	bool bStickingToWall;
	bool bWallSlowDown{};
	FVector StickingSpot{};

	/* The angle from the walls normal that the character will jump from */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Wall Jump")
		float WallJump_JumpAngle UMETA(DisplayName = "Jump Angle") { 45.f };
	/* The angle the jump vector will be rotated when the character walljumps towards the left or right */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Wall Jump")
		float WallJump_SidewaysJumpAngle UMETA(DisplayName = "Jump Angle Sideways") { 45.f };
	

	bool bWallJump{};
	FVector WallJump_VelocityDirection{};
	bool WallJump(const FVector& ImpactNormal);
	bool StickToWall(float DeltaTime);
	bool ReleaseFromWall(const FVector& ImpactNormal);

#pragma endregion //Wall Jump

#pragma region LedgeGrab

	bool bLedgeGrab{};
	bool bLedgeJump{};


	UPROPERTY(EditAnywhere, Category = "MyVariables|LedgeJump")
		float LedgeJumpBoost_Multiplier{ 0.2f };
	float LedgeJumpBoost{};

	void Start_LedgeGrab();
	void Update_LedgeGrab();

	bool LedgeJump(const FVector& LedgeLocation);

#pragma endregion //LedgeGrab

public: // Slipping
	UPROPERTY(BlueprintReadWrite)
	float Traced_GroundFriction;
};
