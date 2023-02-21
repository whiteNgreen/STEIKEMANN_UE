// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "../GameplayTags.h"
#include "../StaticVariables.h"
#include "EnemyAIController.generated.h"

DECLARE_MULTICAST_DELEGATE(FSensedPawnsDelegate)

UENUM(BlueprintType)
enum class EDogType : uint8
{
	Red,
	Pink,
	Teal
};

UENUM(BlueprintType)
enum class ESmallEnemyAIState : uint8
{
	RecentlySpawned,
	Idle,
	Alerted,
	ChasingTarget,
	GuardSpawn,
	Attack,

	Incapacitated,
	
	None
};
UENUM(BlueprintType)
enum class EIdleState : uint8
{
	MoveTo_GuardLocation,
	Guard,

	Sleeping,
	MovingTo_SleepLocation
};

UENUM(BlueprintType)
enum class EAIIncapacitatedType : uint8
{
	None,
	Stunned,
	Grappled,
	StuckToWall
};


/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:	// Components
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UPawnSensingComponent* PSComponent{ nullptr };

public: // Assets

public:	// Functions
	AEnemyAIController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;


	// Idle
	EIdleState m_EIdleState;
	FVector IdleLocation{};
	TSharedPtr<struct SpawnPointData> m_SpawnPointData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float Idle_SleepingLocationRandRadius{ 200.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float Idle_SleepingLocationAcceptanceRadius{ 100.f };

	void IdleBegin();
	void IdleUpdate(float DeltaTime);
	
	void IdleUpdate_Red(float DeltaTime);
	void IdleUpdate_PinkTeal(float DeltaTime);

	// Pawn sensing
	UFUNCTION()
		void AIOnSeePawn(APawn* pawn);
	UFUNCTION()
		void AIHearNoise(APawn* InstigatorPawn, const FVector& Location, float Volume);
	//void SensePawn(APawn* pawn, FGameplayTag& tag);
	//void SensePawn_Player();
	void SpotPlayer();

	void AlertedTimeCheck();
	void StopSensingPlayer();

	// Attacking 
	void Attack();

	// Setting State
	void SetState(const ESmallEnemyAIState& state);
	void LeaveState(const ESmallEnemyAIState& state);
	
	// Incapacitating Owner
	void IncapacitateAI(const EAIIncapacitatedType& IncapacitateType, float Time, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None);
	void ReDetermineState(const ESmallEnemyAIState& StateOverride = ESmallEnemyAIState::None);

	void CapacitateAI(float Time, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None);

	
	// Chase Target
	void ChaseBegin();
	void ChaseStop();
	void ChaseTimedUpdate();
	void ChaseUpdate(float DeltaTime);
	void LerpPinkTeal_ChaseLocation(float DeltaTime);
	AActor* m_Player{ nullptr };
	FTimerHandle TH_ChaseUpdate;

	struct EDogPack* m_DogPack{ nullptr };

	void ChasePlayer_Red();
	void ChasePlayer_Red_Update();

	void ChasePlayer_Pink();

	void ChasePlayer_Teal();

	void ChasePlayer_CircleAround_Update(const FVector& forward);

	/* How far forward of the player, will Pink pursue player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PinkTeal_ForwardChaseLength{ 400.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PinkTeal_AttackActivationRange{ 300.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PinkTeal_SideLength{ 500.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PinkTeal_SideLength_LerpSpeed{ 2.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PinkTeal_UpdateTime{ 0.2f };
	FVector PinkTeal_ChaseLocation;
	FVector PinkTeal_ChaseLocation_Target;
	bool bPinkTealCloseToPlayer{};

	// Guard spawn
	void GuardSpawnUpdate(float DeltaTime);
	FVector m_GuardLocation{};

public: // Variables
	UPROPERTY(BlueprintReadOnly)
		class ASmallEnemy* m_PawnOwner{ nullptr };

	ESmallEnemyAIState m_AIState = ESmallEnemyAIState::None;
	EAIIncapacitatedType m_AIIncapacitatedType = EAIIncapacitatedType::None;

	FTimerManager TM_AI;	// ha en egen timer manager istedenfor å bruke World Timer Manager?
	FTimerHandle TH_IncapacitateTimer;

	FTimerHandle TH_StopSensingPlayer;
	FTimerHandle TH_SpotPlayer;


	bool bIsSensingPawn{};
	FSensedPawnsDelegate SensedPawnsDelegate;
	//FDelegateHandle DH_SensedPlayer;

	/* Time taken to spot the player */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float TimeToSpotPlayer{ 2.f };

	//UPROPERTY(BlueprintReadWrite)
		//bool bFollowPlayer{};
	/* Time AI is spent as recently spawned, where it does nothing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float TimeSpentRecentlySpawned{ 1.f };
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	EDogType m_DogType;
};
