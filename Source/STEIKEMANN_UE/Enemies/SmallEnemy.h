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
#include "Components/TimelineComponent.h"
//#include "../GameplayTags.h"
#include "EnemyAIController.h"

#include "SmallEnemy.generated.h"

DECLARE_DELEGATE(FIncapacitatedLandDelegation)
DECLARE_DELEGATE(FIncapacitatedCollision)

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
		STATE_Launched,

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


struct SpawnPointData
{
	FVector Location;
	float Radius_Min;
	float Radius_Max;
};

UCLASS()
class STEIKEMANN_UE_API ASmallEnemy : public ABaseCharacter,
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

public:	// Components
	// Collision
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		USphereComponent* PlayerPogoDetection{ nullptr };

	// Timelines
	UPROPERTY(BlueprintReadOnly)
		UTimelineComponent* TlComp_Scooped { nullptr };
	UPROPERTY(BlueprintReadOnly)
		UTimelineComponent* TlComp_Smacked { nullptr };

	// Particles
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Particles")
		UNiagaraSystem* NS_Trail{ nullptr };
	UNiagaraComponent* NComp_AirTrailing;

#pragma endregion //Base

#pragma region Animation
public:	// Variables
	class UEnemyAnimInstance* AnimInstance{ nullptr };

public: // Functions
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Attacked();
	void Anim_Attacked_Pure(FVector direction);
#pragma endregion // Animation

#pragma region SpawnRespawn
	SpawnPointData m_SpawnPointData;
	FVector GetRandomLocationNearSpawn();
#pragma endregion // SpawnRespawn

	void DisableCollisions();
	void EnableCollisions();
#pragma region GameplayTags
	UPROPERTY(BlueprintReadOnly, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; return; }
#pragma endregion //GameplayTags

#pragma region AI
public: // Variables
	UPROPERTY(BlueprintReadOnly)
		APawn* m_SensedPawn{ nullptr };
	UPROPERTY(BlueprintReadOnly)
		bool bSensingPawn{};
/// <summary>
/// DELEGATE:
/// 	Funksjoner kalt at AI legger til i delegate.
/// 	Delegate kalles p� BTService
/// </summary>
public: // Functions
	/* Returns true if the player is spotted, false if it spots something else */
	FGameplayTag SensingPawn(APawn* pawn);

#pragma endregion // AI

#pragma region States
public:	// STATES
	EEnemyState m_State = EEnemyState::STATE_None;
	EGravityState m_Gravity = EGravityState::Default;
	void SetDefaultState();

	virtual void Landed(const FHitResult& Hit) override;

	void EnableGravity();
	void DisableGravity();

public: // Variables for calling AI
	FIncapacitatedCollision IncapacitatedCollisionDelegate;
	FIncapacitatedLandDelegation IncapacitatedLandDelegation;

public: // Functinos calling AI controller
	void Incapacitate(const EAIIncapacitatedType& IncapacitateType, float Time = -1.f, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None);
	void IncapacitateUndeterminedTime(const EAIIncapacitatedType& IncapacitateType, void (ASmallEnemy::* landingfunction)() = nullptr);
	void Capacitate(const EAIIncapacitatedType& IncapacitateType, float Time = -1.f, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None);

private: // Functions Capacitate - Used for IncapacitatedLandingDelegate
	void IncapacitatedLand();
	void CollisionDelegate();
	void Capacitate_Grappled();

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Scoop|Curve")
		UCurveFloat* Curve_ScoopedZForceFloat{ nullptr };
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Scoop|Curve")
		float ScoopedCurveMultiplier{ 1.f };
	/* When launched target height is set by the player, but this actors Z height will be adjusted by this value */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Scoop")
		float ScoopedZHeightAdjustment{ -30.f };
	FVector ScoopedLocation{};
	float ScoopedLength2D{};

	UFUNCTION()
		void Tl_Scooped(float value);
	UFUNCTION()
		void Tl_ScoopedEnd();

	void Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting Scooped
	void Receive_ScoopAttack_Pure(const FVector& TargetLocation, const FVector& InstigatorLocation, const float& time) override;

	void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override {}
	void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) override;

public: // Particles for getting smacked
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Particles|Trail")
		float NS_Trail_SpawnRate{ 50.f };
	float NS_Trail_SpawnRate_Internal{};
	/* Multiplied by velocity */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Particles|Trail")
		float NS_Trail_SpeedMin{ 0.4f };
	/* Multiplied by velocity */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Particles|Trail")
		float NS_Trail_SpeedMax{ 1.0f };
	bool bTrailingParticles{};
	void NS_Start_Trail(FVector direction);
	void NS_Update_Trail(float DeltaTime);
	void NS_Stop_Trail();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Particles|Trail")
		UCurveFloat* Curve_NSTrail{ nullptr };
	UFUNCTION()
		void Tl_Smacked(float value);

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
