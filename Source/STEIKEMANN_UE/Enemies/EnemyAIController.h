// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "../GameplayTags.h"
#include "EnemyAIController.generated.h"

DECLARE_MULTICAST_DELEGATE(FSensedPawnsDelegate)

UENUM(BlueprintType)
enum class ESmallEnemyAIState : uint8
{
	RecentlySpawned,
	Idle,
	Alerted,
	ChasingTarget,
	Attack,

	Incapacitated,
	
	None
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
		class UBehaviorTreeComponent* BTComponent{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UBlackboardComponent* BBComponent{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UPawnSensingComponent* PSComponent{ nullptr };

public: // Assets
	UPROPERTY(EditAnywhere)
		class UBehaviorTree* BT{ nullptr };

	UPROPERTY(EditAnywhere)
		class UBlackboardData* BB{ nullptr };

public:	// Functions
	AEnemyAIController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	// Pawn sensing
	UFUNCTION()
		void AIOnSeePawn(APawn* pawn);
	UFUNCTION()
		void AIHearNoise(APawn* InstigatorPawn, const FVector& Location, float Volume);
	void SensePawn(APawn* pawn, FGameplayTag& tag);
	void SensePawn_Player();
	void SpotPlayer();

	// Attacking 
	void Attack();

	// Setting State
	void ResetTree();
	void SetState(const ESmallEnemyAIState& state);
	
	
	// Incapacitating Owner
	void IncapacitateAI(const EAIIncapacitatedType& IncapacitateType, float Time, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None);
	void ReDetermineState(const ESmallEnemyAIState& StateOverride = ESmallEnemyAIState::None);

	void CapacitateAI(float Time, const ESmallEnemyAIState& NextState = ESmallEnemyAIState::None);

	
public: // Functions called by BTTasks and BTServices
	void SetNewTargetPoints();

	void UpdateTargetPosition();

public: // Variables
	UPROPERTY(BlueprintReadOnly)
		class ASmallEnemy* m_PawnOwner{ nullptr };

	ESmallEnemyAIState m_AIState = ESmallEnemyAIState::RecentlySpawned;
	EAIIncapacitatedType m_AIIncapacitatedType = EAIIncapacitatedType::None;

	FTimerManager TM_AI;	// ha en egen timer manager istedenfor å bruke World Timer Manager?
	FTimerHandle TH_IncapacitateTimer;
	FTimerHandle TH_SensedPlayer;
	FTimerHandle TH_SpotPlayer;


	bool bIsSensingPawn{};
	FSensedPawnsDelegate SensedPawnsDelegate;
	FDelegateHandle DH_SensedPlayer;

	UPROPERTY(BlueprintReadWrite)
		bool bFollowPlayer{};
	/* Time AI is spent as recently spawned, where it does nothing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		float TimeSpentRecentlySpawned{ 1.f };
};
