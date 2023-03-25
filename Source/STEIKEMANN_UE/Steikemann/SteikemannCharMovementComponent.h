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
		float m_GravityScaleOverride /*UMETA(DisplayName = "Gravity Scale Override")*/ { 2.f };
		/* Gravity scale during freefall */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		//float m_GravityScaleOverride_Freefall /*UMETA(DisplayName = "Freefall Gravity")*/ { 2.f };
		/* Interpolation speed between gravityscale override and freefall gravity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|GravityOverride")
		float m_GravityScaleOverride_InterpSpeed{ 2.f };

public:	// Functions
	void EnableGravity();
	void DisableGravity();
private:	// Private Functions
	void SetGravityScale(float deltatime);

#pragma endregion			//Gravity

#pragma region Jump
public:
	bool DoJump(bool bReplayingMoves) override;

	/* --- New Jump --- */
	bool bIsJumping{};
	bool bIsDoubleJumping{};
	
	float InitialJumpVelocity;
	void Jump(const float& JumpStrength);
	void Jump(const FVector& direction, const float& JumpStrength);
	void DoubleJump(const FVector& Direction, const float& JumpStrength);

	void JumpHeight(const float Height, const float time);

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

#pragma endregion			//Jump
#pragma region InAir
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|FreeFall")
		float AirFriction2D_NoInputStrength{ 0.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MyVariables|FreeFall")
		float AirFriction2D_Strength{ 1.f };

	void AirFriction2D(FVector input);
	float AirFriction2D_Multiplier{ 1.f };
	UFUNCTION()
		void AirFrictionMultiplier(float value);
#pragma endregion			//InAir
#pragma region Pogo
public:
	void PB_Launch_Active(FVector direction, float strength);
#pragma endregion			//Pogo
#pragma region GRAPPLE HOOK
	bool bGrappleHook_InitialState{};

#pragma endregion	//GRAPPLE HOOK
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
	Wall::WallData m_WallJumpData;


	//void InitialOnWall(const Wall::WallData& wall, float time);
	void Initial_OnWall_Hang(const Wall::WallData& wall, float time);

	void WallJump(FVector input, float JumpStrength);
	void LedgeJump(const FVector input, float JumpStrength);

	void ExitWall();
	void CancelOnWall();
private:
	FTimerHandle TH_WallHang;

	void ExitWall_Air();
	//void InitialOnWall_IMPL(float time);
	void OnWallHang_IMPL();
	void OnWallDrag_IMPL(float deltatime);

	FVector GetInputDirectionToNormal(FVector& input, const FVector& normal);
	FVector GetInputDirectionToNormal(FVector& input, const FVector& normal, FVector& right, FVector& up);
	FVector ClampDirectionToAngleFromVector(const FVector& direction, const FVector& clampVector, const float angle, const FVector& right, const FVector& up);

#pragma endregion			//On Wall
#pragma region GroundPound
public:
	bool bGP_PreLaunch{};
	void GP_PreLaunch();
	void GP_Launch(float strength);
#pragma endregion		//GroundPound

public: // Slipping
	UPROPERTY(BlueprintReadWrite)
	float Traced_GroundFriction;
};
