// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyClasses_Enums.h"
#include "EnemyAIController.generated.h"

DECLARE_MULTICAST_DELEGATE(FSensedPawnsDelegate)



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

	FTimerManager TimerManager;

	// Recently Spawned
	void RecentlySpawnedBegin();

	// Idle
	EIdleState m_EIdleState;
	FVector IdleLocation{};
	TSharedPtr<struct SpawnPointData> m_SpawnPointData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float Idle_SleepingLocationRandRadius{ 200.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float Idle_SleepingLocationAcceptanceRadius{ 100.f };

	void IdleBegin();
	void IdleEnd();
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

	void AlertedInit(const APawn& instigator);

	void AlertedBegin();
	void AlertedEnd();
	void AlertedUpdate(float DeltaTime);
	void AlertedTimeCheck();
	void StopSensingPlayer();

	FVector SuspiciousLocation{};
	bool bIsSensingPawn{};

	bool AlertedByPack();

	// Attacking 
	/* The distance to the player the AI will initiate Attack */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float AttackDistance{ 300.f };
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float AttackStateTime{ 1.5f };
	FTimerHandle TH_AttackState;
	/* Timer from the enemy jumps to the Chomp they do in air */
	FTimerHandle TH_PreAttack;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float JumpToChompTime{ 1.f };
	bool CanAttack_AI() const;
	void AttackBegin();
	void AttackEnd();
	void Attack();
	void AttackJump();
	void CancelAttackJump();

	void ReceiveAttack();

	// Setting State
	void SetState(const ESmallEnemyAIState& state);
	void AlwaysBeginStates(const ESmallEnemyAIState& incommingState);
	void LeaveState(const ESmallEnemyAIState& currentState, const ESmallEnemyAIState& incommingState);
	
	// Incapacitating Owner
	void IncapacitateBegin();
	void IncapacitateEnd();
	void IncapacitateAI(const EAIIncapacitatedType& IncapacitateType, float Time/*, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None*/);
	void ReDetermineState(const ESmallEnemyAIState& StateOverride = ESmallEnemyAIState::None);

	/* After getting incapacitated by the player, if the player is within this radius
	 * then the dog will instantly go to chase state */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float PostGettingSmackedActivationRange{ 500.f };
	void Incapacitate_GettingSmacked();
	void Post_Incapacitate_GettingSmacked();
	void RedetermineIncapacitateState();
	bool IsIncapacitated() const;

#pragma region Chase

#pragma region Chase_General
	// Chase Target
	void ChaseBegin();
	void ChaseEnd();
	void ChaseTimedUpdate();
	void ChaseUpdate(float DeltaTime);
	void LerpPinkTeal_ChaseLocation(float DeltaTime);
	void GetPlayerPtr();
	AActor* m_Player{ nullptr };
	FTimerHandle TH_ChaseUpdate;
#pragma endregion //Chase_General

#pragma region Chase_State
public:
	FTimerHandle TH_DisabledChaseMovement;
	EAI_ChaseState m_EAI_ChaseState;
	void SetChaseState_Start();
	void SetChaseState(const EAI_ChaseState NewChaseState);
	void Evaluate_ChaseState();
	FTimerHandle TH_Chase_InvalidPath_Bark;
	void ChaseState_InvalidPath_Begin();
	void ChaseState_Bark();
	FTimerHandle TH_Chase_InvalidPath_JumpAttempt;
	void ChaseState_CorgiJump();

public:
	UPROPERTY(EditAnywhere, Category = "State|Chase|Bark")
		float ChaseState_Bark_DisableMovementTimer{ 1.f };
	/* Barking interval is set between the minimum value and 
	 * min value + Addition value */
	UPROPERTY(EditAnywhere, Category = "State|Chase|Bark")
		float ChaseState_Bark_Interval_Min{ 1.f };
	UPROPERTY(EditAnywhere, Category = "State|Chase|Bark")
		float ChaseState_Bark_Interval_Addition{ 1.f };
	UPROPERTY(EditAnywhere, Category = "State|Chase|InvalidPath Jump")
		float ChaseState_CorgiJumpTime{ 2.f };


#pragma endregion //Chase_State

#pragma region Chase_DogPack
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
#pragma endregion //Chase_DogPack
#pragma endregion //Chase
	// Guard spawn
	void GuardSpawnUpdate(float DeltaTime);
	void GuardSpawnBegin();
	void GuardSpawnEnd();
	FVector m_GuardLocation{};
	FTimerHandle TH_GuardSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float GuardSpawn_Time{ 2.f };

public: // Variables
	UPROPERTY(BlueprintReadOnly)
		class ASmallEnemy* m_PawnOwner{ nullptr };

	ESmallEnemyAIState m_AIState = ESmallEnemyAIState::None;
	EAIIncapacitatedType m_AIIncapacitatedType = EAIIncapacitatedType::None;
	EAIPost_IncapacitatedType m_EAIPost_IncapacitatedType;

	FTimerManager TM_AI;	// ha en egen timer manager istedenfor å bruke World Timer Manager?
	FTimerHandle TH_IncapacitateTimer;

	FTimerHandle TH_StopSensingPlayer;
	FTimerHandle TH_SpotPlayer;


	//FSensedPawnsDelegate SensedPawnsDelegate;
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


#ifdef UE_BUILD_DEBUG
public:
	void PrintState();
	void Print_SetState(ESmallEnemyAIState state);

	void PRINT_ChaseState();

	#define PRINT_STATE()			PrintState()
	#define PRINT_SETSTATE(state)	Print_SetState(state)
	#define PRINT_CHASESTATE()		PRINT_ChaseState()
#else
	#define PRINT_STATE()
	#define PRINT_SETSTATE(state)
	#define PRINT_CHASESTATE()
#endif
};
