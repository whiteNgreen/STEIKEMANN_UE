// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../AbstractClasses/AbstractCharacter.h"
#include "../Interfaces/AttackInterface.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../DebugMacros.h"
#include "GameplayTagAssetInterface.h"
#include "../WallDetectionComponent.h"
//#include "../GameplayTags.h"

#include "SmallEnemy.generated.h"

/************************ ENUMS *****************************/
UENUM()
enum class EGravityState : int8
{
	Default,
	LerpToDefault,

	None,
	LerpToNone,
	ForcedNone
};
// State
UENUM()
enum class EEnemyState : int8
{
	STATE_None,

	STATE_OnGround,
	STATE_InAir,

	STATE_OnWall
};
// Wall Mechanic
UENUM()
enum class EWall : int8
{
	WALL_None,

	WALL_Stuck,

	WALL_Leaving
};

UCLASS()
class STEIKEMANN_UE_API ASmallEnemy : public AAbstractCharacter,
	public IAttackInterface,
	public IGameplayTagAssetInterface,
	public IGrappleTargetInterface
{
	GENERATED_BODY()
#pragma region Base
public:
	// Sets default values for this character's properties
	ASmallEnemy();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* PlayerPogoDetection{ nullptr };
#pragma endregion //Base

	void DisableCollisions();
	void EnableCollisions();
#pragma region GameplayTags
	UPROPERTY(BlueprintReadOnly, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; return; }
#pragma endregion //GameplayTags

#pragma region States
public:	// STATES
	EEnemyState m_State = EEnemyState::STATE_None;
	EGravityState m_Gravity = EGravityState::Default;
	void SetDefaultState();

private: // Gravity
	float GravityScale;
	float GravityZ;

#pragma endregion //States
	void RotateActorYawToVector(FVector AimVector, float DeltaTime = 0);

#pragma region Wall Mechanics
public:
	UWallDetectionComponent* WallDetector{ nullptr };
	EWall m_WallState = EWall::WALL_None;
	FTimerHandle TH_LeavingWall;

public:
	void StickToWall();
	virtual bool IsStuck_Pure() override { return m_State == EEnemyState::STATE_OnWall; }
	void LeaveWall();
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_LeavingWallTimer{ 0.5f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		bool bWDC_Debug{};


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_Capsule_Radius{ 40.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_Capsule_Halfheight{ 90.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_MinHeight{ 20.f };
private:
	Wall::WallData m_WallData;
	//Wall::WallData m_WallJumpData;

#pragma endregion //Wall Mechanics


#pragma region GrappleHooked
public: 
	bool bCanBeGrappleHooked{ true };
	/* The internal cooldown before enemy can be grapplehooked again */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappleHookedInternalCooldown{ 0.5f };

	FTimerHandle Handle_GrappledCooldown;
	void ResetCanBeGrappleHooked() { bCanBeGrappleHooked = true; }


	/* Time it should take to reach the Grappled Instigator : 2nd method */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappledLaunchTime{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappledLaunchTime_CollisionActivation{ 0.1f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappledInstigatorOffset{ 50.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook|PulledFree", meta = (UIMin = "0", UIMax = "3"))
		float Grappled_PulledFreeStrengthMultiplier{ 1.5f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook|PulledFree")
		float Grappled_PulledFreeNoCollisionTimer{ 0.5f };

	/* ----- Grapple Interface ------ */
	virtual void TargetedPure() override;

	virtual void UnTargetedPure() override;

	virtual void InReach_Pure() override;

	virtual void OutofReach_Pure() override;

	virtual void HookedPure() override;
	virtual void HookedPure(const FVector InstigatorLocation, bool OnGround,bool PreAction = false) override;

	virtual void UnHookedPure() override;

	virtual void PullFree_Pure(const FVector InstigatorLocation);

	//virtual FGameplayTag GetGrappledGameplayTag_Pure() const override { return Enemy; }
#pragma endregion //GrappleHooked

#pragma region GettingSmacked
public:
	bool bCanBeSmackAttacked{ true };

	FTimerHandle THandle_GotSmackAttacked{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|SmackAttack")
		float SmackAttack_OnGroundMultiplication{ 0.1f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|SmackAttack")
		float SmackAttack_InternalTimer{ 0.5f };

	/**
	*	Sets a timer before character can be damaged again 
	*	With respect to the specific handle it should use */
	void WaitBeforeNewDamage(FTimerHandle TimerHandle, float Time);	
	
	bool CanBeAttacked() override;

	void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting SmackAttacked
	void Receive_SmackAttack_Pure(const FVector& Direction, const float& Strength) override;
	bool GetCanBeSmackAttacked() const override { return bCanBeSmackAttacked; }
	void ResetCanBeSmackAttacked() override { bCanBeSmackAttacked = true; }


	void Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting Scooped
	void Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength) override;

	void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override {}
	void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) override;


#pragma endregion //GettingSmacked
#pragma region Pogo
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Pogo|Collision")
		float PB_SphereRadius{ 90.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Pogo|Collision")
		float PB_SphereRadius_Stuck{ 150.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Pogo")
		float PB_Groundpound_LaunchWallNormal{ 0.2f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Pogo")
		float PB_Groundpound_LaunchStrength{ 1200.f };
public:
	void Receive_Pogo_GroundPound_Pure() override;
#pragma endregion //Pogo
};
