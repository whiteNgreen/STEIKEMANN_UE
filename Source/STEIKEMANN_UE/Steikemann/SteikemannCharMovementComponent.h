// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "../DebugMacros.h"
#include "../WallDetectionComponent.h"
#include "SteikemannCharMovementComponent.generated.h"

/**
 * 
 */

//UENUM(BlueprintType)
//enum ECustomMovementMode
//{
//	MOVECustom_None				UMETA(DisplayName = "None"),
//	MOVECustom_Slide			UMETA(DisplayName = "Slide"),
//	MOVECustom_WallSticking		UMETA(DisplayName = "Wallsticking"),
//	MOVECustom_Grappling		UMETA(DisplayName = "Grappling"),
//};

// TODO: GRAVITY OVERRIDE ENUM
UENUM(BlueprintType)
enum class EGravityMode : uint8
{
	Default,
	LerpToDefault,
	LerpToNone,
	None,
	ForcedNone
};

UCLASS()
class STEIKEMANN_UE_API USteikemannCharMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()
	
public:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	class ASteikemannCharacter* CharacterOwner_Steikemann{ nullptr };
	ASteikemannCharacter* GetCharOwner() { return CharacterOwner_Steikemann; }
	
	TEnumAsByte<enum ECustomMovementMode> CustomMovementMode;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Friction")
	float CharacterFriction{ 15.f };


#pragma region Gravity
public:
	EGravityMode m_GravityMode = EGravityMode::Default;

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
private:
	void SetGravityScale(float deltatime);

#pragma endregion //Gravity

#pragma region Crouch
public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Crouch|CrouchSlide")

	float CrouchSlideSpeed{};
	FVector CrouchSlideDirection{};

	void Initiate_CrouchSlide(const FVector& SlideDirection);
	void Do_CrouchSlide(float DeltaTime);

/* -- Crouch Jump -- */
	bool bCrouchJump{};
	void StartCrouchJump()	{ bCrouchJump = true;  }
	void EndCrouchJump()	{ bCrouchJump = false; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump|CrouchSlideJump")
		float CrouchJumpSpeed{ 1000.f };

/* -- CrouchSlide Jump -- */
	bool bCrouchSlideJump{};
	FVector CrouchSlideJump_Vector{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump|CrouchSlideJump")
		float CrouchSlideJumpAngle	UMETA(DisplayName = "Jump Angle") { 30.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump|CrouchSlideJump")
		float CSJ_MaxInputAngleAdjustment UMETA(DisplayName = "Max Input Angle Adjustment") { 30.f };


	/*	* Initiates the CrouchSlideJump. The SlideDirection vector is the current direction the character is crouchsliding in 
		* The player can use their input to slightly alter the direction of the CrouchSlideJump 
		* Function assumes the vectors are normalized */
	bool CrouchSlideJump(const FVector& SlideDirection, const FVector& Input);
	void EndCrouchSlideJump() { bCrouchSlideJump = false; }

#pragma endregion //Crouch

#pragma region Jump
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump")
		//bool bJumping{};
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|Jump")
		//float JumpInterpSpeed{ 2.f };

	bool DoJump(bool bReplayingMoves) override;

	/* --- New Jump --- */
	bool bIsJumping{};
	bool bIsDoubleJumping{};
	
	float InitialJumpVelocity;
	void Jump(const float& JumpStrength);
	void Jump(const FVector& direction, const float& JumpStrength);
	void DoubleJump(const FVector& Direction, const float& JumpStrength);

	/* How far through the jump is the player? Determined by the current velocity */
	float JumpPercentage{};
	void DetermineJump(float DeltaTime);
	
	float JumpPrematureSlowDownTime{ 0.2f };
	void SlowdownJumpSpeed(float DeltaTime);

	bool bJumpPrematureSlowdown{};
	void StopJump();
	
	bool bJumpHeightHold{};

	FTimerHandle TH_JumpHold;
	void StartJumpHeightHold();
	void StopJumpHeightHold();
	
	void DeactivateJumpMechanics();

#pragma endregion //Jump

#pragma region GRAPPLE HOOK
	bool bGrappleHook_InitialState{};

#pragma endregion //GRAPPLE HOOK

//#pragma region Bounce
	//void Bounce(FVector surfacenormal);
//#pragma endregion //Bounce

//#pragma region Dash
	//float fPreDashTimerLength{};
	//float fPreDashTimer{};
	//float fDashTimerLength{};
	//float fDashTimer{};
	//float fDashLength{};
	//FVector DashDirection;

	//void Start_Dash(float preDashTime, float dashTime, float dashLength, FVector dashdirection);
	//void Update_Dash(float deltaTime);

	/**	Dash During Grapplehook_Swing - Boost
	*/
	//void Grapplehook_Dash(float DashStrength, FVector DashDirection);
//#pragma endregion //Dash

#pragma region On Wall
public:	// WallJump
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Mechanics|Drag")
		float WJ_DragSpeed{ 200.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Mechanics|Jump", meta = (UIMin = "0", UIMax = "2"))
		float WallJump_StrenghtMulti{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Mechanics|Jump", meta = (UIMin = "0", UIMax = "1"))
		float WallJump_SidewaysAngleLimit{ 0.5f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Mechanics|Jump", meta = (UIMin = "0", UIMax = "90"))
		float WallJump_UpwardAngle{ 45.f };
public: // Ledge Jump
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wall Mechanics|LedgeJump", meta = (UIMin = "0", UIMax = "90"))
		float LedgeJump_AngleLimit{ 20.f };
	UPROPERTY(EditAnywhere, Category = "MyVariables|LedgeJump")
		float LedgeJumpBoost_Multiplier{ 0.2f };
private:


public:
	// NEW ON WALL
	EOnWallState m_WallState = EOnWallState::WALL_None;
	Wall::WallData m_Walldata;


	void InitialOnWall(const Wall::WallData& wall, float time);
	void Initial_OnWall_Hang(const Wall::WallData& wall, float time);

	void WallJump(FVector input, float JumpStrength);
	void LedgeJump(const FVector input, float JumpStrength);

	void ExitWall();
private:
	FTimerHandle TH_WallHang;

	void ExitWall_Air();
	void InitialOnWall_IMPL(float time);
	void OnWallHang_IMPL();
	void OnWallDrag_IMPL(float deltatime);

	FVector GetInputDirectionToNormal(FVector& input, const FVector& normal);
	FVector GetInputDirectionToNormal(FVector& input, const FVector& normal, FVector& right, FVector& up);
	FVector ClampDirectionToAngleFromVector(const FVector& direction, const FVector& clampVector, const float angle, const FVector& right, const FVector& up);

#pragma endregion //On Wall

#pragma region GroundPound
public:
	bool bGP_PreLaunch{};
	void GP_PreLaunch();
	void GP_Launch(float strength);
#pragma endregion //GroundPound

public: // Slipping
	UPROPERTY(BlueprintReadWrite)
	float Traced_GroundFriction;
};
