// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/EnemyAIController.h"
#include "../DebugMacros.h"
#include "../GameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "SmallEnemy.h"
#include "../Steikemann/SteikemannCharacter.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"

AEnemyAIController::AEnemyAIController()
{
	BTComponent = CreateDefaultSubobject<UBehaviorTreeComponent>("BehaviorTree Component");
	BBComponent = CreateDefaultSubobject<UBlackboardComponent>("Blackboard Component");
	PSComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	PSComponent->OnSeePawn.AddDynamic(this, &AEnemyAIController::OnSeePawn);
	PSComponent->OnHearNoise.AddDynamic(this, &AEnemyAIController::HearNoise);

	Async(EAsyncExecution::TaskGraphMainThread, [this]() {
		SetState(ESmallEnemyAIState::RecentlySpawned);
		FTimerHandle h;
		GetWorldTimerManager().SetTimer(h, [this]() { SetState(ESmallEnemyAIState::Idle); }, TimeSpentRecentlySpawned, false);
		});
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PRINTPARLONG("AI Possessed %s", *InPawn->GetName());
	InitializeBlackboard(*BBComponent, *BB);
	BTComponent->StartTree(*BT);

	SetState(ESmallEnemyAIState::Idle);
	
	BBComponent->SetValueAsObject("Player", UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass()));
}

void AEnemyAIController::OnSeePawn(APawn* SeenPawn)
{
}

void AEnemyAIController::HearNoise(APawn* InstigatorPawn, const FVector& Location, float Volume)
{
}

void AEnemyAIController::ResetTree()
{
	BTComponent->RestartTree();
}

void AEnemyAIController::SetState(const ESmallEnemyAIState& state)
{
	m_AIState = state;
	BBComponent->SetValueAsEnum("Enum_State", (int8)state);
	ResetTree();
}

void AEnemyAIController::IncapacitateAI(const EAIIncapacitatedType& IncapacitateType, float Time, const ESmallEnemyAIState& NextState)
{
	GetWorldTimerManager().ClearTimer(TH_IncapacitateTimer);
	SetState(ESmallEnemyAIState::Incapacitated);
	m_AIIncapacitatedType = IncapacitateType;

	// AI Incapacitated for an undetermined amount of time
	if (Time < 0.f)
	{
		return;
	}
	GetWorldTimerManager().SetTimer(TH_IncapacitateTimer, [this, NextState]() { PostIncapacitated_DetermineState(NextState); }, Time, false);
}

void AEnemyAIController::PostIncapacitated_DetermineState(const ESmallEnemyAIState& StateOverride)
{
	if (StateOverride != ESmallEnemyAIState::None) 
	{
		SetState(StateOverride);
		return;
	}

	// Make a determine next state algorithm based on pawn senses and potential player position
		//	Temp method 
	SetState(ESmallEnemyAIState::Idle);
}

void AEnemyAIController::CapacitateAI(float Time, const ESmallEnemyAIState& NextState)
{
	// AI Incapacitated for an undetermined amount of time
	if (Time < 0.f)
	{
		PostIncapacitated_DetermineState(NextState);
		return;
	}
	GetWorldTimerManager().SetTimer(TH_IncapacitateTimer, [this, NextState]() { PostIncapacitated_DetermineState(NextState); }, Time, false);
}

void AEnemyAIController::SetNewTargetPoints()
{
	ASmallEnemy* SEPawn = Cast<ASmallEnemy>(GetPawn());
	BBComponent->SetValueAsVector("VTarget_A", SEPawn->GetActorLocation());

	// Get location near spawner 
	FVector NearSpawnLocation = SEPawn->GetRandomLocationNearSpawn();
	BBComponent->SetValueAsVector("VTarget_B", NearSpawnLocation);
	UpdateTargetPosition();

	DrawDebugPoint(GetWorld(), NearSpawnLocation, 30.f, FColor::Black, false, 3.f, -1);
}

void AEnemyAIController::UpdateTargetPosition()
{
	FVector targetlocation = BBComponent->GetValueAsVector("VTarget_B");

	BBComponent->SetValueAsVector("VTargetLocation", targetlocation);
}


