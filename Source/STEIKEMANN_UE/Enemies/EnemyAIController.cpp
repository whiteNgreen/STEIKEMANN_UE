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

	PSComponent->OnSeePawn.AddDynamic(this, &AEnemyAIController::AIOnSeePawn);
	PSComponent->OnHearNoise.AddDynamic(this, &AEnemyAIController::AIHearNoise);

	Async(EAsyncExecution::TaskGraphMainThread, [this]() {
		SetState(ESmallEnemyAIState::RecentlySpawned);
		FTimerHandle h;
		GetWorldTimerManager().SetTimer(h, [this]() { SetState(ESmallEnemyAIState::Idle); }, TimeSpentRecentlySpawned, false);
		});
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TM_AI.Tick(DeltaTime);
	bIsSensingPawn ? PRINT("bIsSensingPawn = True") : PRINT("bIsSensingPawn = False");
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	m_PawnOwner = Cast<ASmallEnemy>(InPawn);

	InitializeBlackboard(*BBComponent, *BB);
	BTComponent->StartTree(*BT);

	SetState(ESmallEnemyAIState::Idle);
	
	BBComponent->SetValueAsObject("Player", UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass()));
}

void AEnemyAIController::AIOnSeePawn(APawn* pawn)
{
	// If player was previously heard behind the AI, instantly become hostile


	// Else do a spot check
	FGameplayTag PawnTag;
	SensePawn(pawn, PawnTag);
	if (PawnTag == Tag::Player())
	{
		PRINTPARLONG("%s sees pawn %s", *GetName(), *pawn->GetName());
		// Spot player, wait 't' seconds before chasing them - BTServiceChecking if player is still spotted 
		bIsSensingPawn = true;

		DH_SensedPlayer = SensedPawnsDelegate.AddUObject(this, &AEnemyAIController::SensePawn_Player);
		SensePawn_Player();
	}
	// else if sensing other pawns (like fellow dogs)
		// be annoyed

	TM_AI.SetTimer(TH_SensedPlayer, [&]() {
		ReDetermineState();
		bIsSensingPawn = false; }, 0.6f/*Sensing timer + margin*/, false);
}

void AEnemyAIController::AIHearNoise(APawn* InstigatorPawn, const FVector& Location, float Volume)
{
	PRINTPARLONG("%s HEARING pawn %s", *GetName(), *InstigatorPawn->GetName());

	// SetState(Alerted)
	//
	// Hear player, if they are then spotted, instantly become hostile
		// Set SensedState on pawn to Previously Heard
}

void AEnemyAIController::SensePawn(APawn* pawn, FGameplayTag& tag)
{
	if (m_AIState == ESmallEnemyAIState::Idle)
		SetState(ESmallEnemyAIState::Alerted);
	tag = m_PawnOwner->SensingPawn(pawn);
}

void AEnemyAIController::SensePawn_Player()
{
	if (TM_AI.IsTimerActive(TH_SpotPlayer))
	{
		if (!bIsSensingPawn) {
			TM_AI.ClearTimer(TH_SpotPlayer);
			ReDetermineState();
			GEngine->AddOnScreenDebugMessage(0, 2.f, FColor::Purple, TEXT("No Longer sensing player"));
			SensedPawnsDelegate.Clear();
		}
	}
	else
	{
		TM_AI.SetTimer(TH_SpotPlayer, this, &AEnemyAIController::SpotPlayer, 2.f/*Time To Spot Player*/);
	}
}

void AEnemyAIController::SpotPlayer()
{
	GEngine->AddOnScreenDebugMessage(0, 2.f, FColor::Purple, TEXT("SPOTTING PLAYER"));
	SetState(ESmallEnemyAIState::ChasingTarget);
	BBComponent->SetValueAsObject("APlayer", m_PawnOwner->m_SensedPawn);
}

void AEnemyAIController::Attack()
{
	PRINTLONG("Attacking");
	auto player = Cast<ASteikemannCharacter>(m_PawnOwner->m_SensedPawn);
	if (!player) return;
	player->PTakeDamage(1, m_PawnOwner);
}

void AEnemyAIController::ResetTree()
{
	BTComponent->RestartTree();
}

void AEnemyAIController::SetState(const ESmallEnemyAIState& state)
{
	if (m_AIState == state) return;
	m_AIState = state;
	BBComponent->SetValueAsEnum("Enum_State", (int8)state);
	ResetTree();
}

void AEnemyAIController::IncapacitateAI(const EAIIncapacitatedType& IncapacitateType, float Time, const ESmallEnemyAIState& NextState)
{
	GetWorldTimerManager().ClearTimer(TH_IncapacitateTimer);
	SetState(ESmallEnemyAIState::Incapacitated);
	m_AIIncapacitatedType = IncapacitateType;

	//PSComponent->SetSensingUpdatesEnabled(false);
	// AI Incapacitated for an undetermined amount of time
	if (Time < 0.f)
	{
		return;
	}
	GetWorldTimerManager().SetTimer(TH_IncapacitateTimer, [this, NextState]() { 
		//PSComponent->SetSensingUpdatesEnabled(true);
		ReDetermineState();
	}, Time, false);
}

void AEnemyAIController::ReDetermineState(const ESmallEnemyAIState& StateOverride)
{
	PRINTLONG("Redetermining State");
	if (StateOverride != ESmallEnemyAIState::None) 
	{
		SetState(StateOverride);
		return;
	}

	// Make a determine next state based on pawn senses and potential player position
	if (bIsSensingPawn)
		SetState(ESmallEnemyAIState::ChasingTarget);
	else
		SetState(ESmallEnemyAIState::Idle);
}

void AEnemyAIController::CapacitateAI(float Time, const ESmallEnemyAIState& NextState)
{
	// AI Incapacitated for an undetermined amount of time
	if (Time < 0.f)
	{
		ReDetermineState(NextState);
		return;
	}
	GetWorldTimerManager().SetTimer(TH_IncapacitateTimer, [this, NextState]() { ReDetermineState(NextState); }, Time, false);
}

void AEnemyAIController::SetNewTargetPoints()
{
	BBComponent->SetValueAsVector("VTarget_A", m_PawnOwner->GetActorLocation());

	// Get location near spawner 
	FVector NearSpawnLocation = m_PawnOwner->GetRandomLocationNearSpawn();
	BBComponent->SetValueAsVector("VTarget_B", NearSpawnLocation);
	UpdateTargetPosition();

	DrawDebugPoint(GetWorld(), NearSpawnLocation, 30.f, FColor::Black, false, 3.f, -1);
}

void AEnemyAIController::UpdateTargetPosition()
{
	FVector targetlocation = BBComponent->GetValueAsVector("VTarget_B");

	BBComponent->SetValueAsVector("VTargetLocation", targetlocation);
}


