// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/EnemyAIController.h"
#include "../DebugMacros.h"
#include "../GameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "SmallEnemy.h"
#include "../Spawner/EnemySpawner.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "../StaticVariables.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/PawnSensingComponent.h"

AEnemyAIController::AEnemyAIController()
{
	//BTComponent = CreateDefaultSubobject<UBehaviorTreeComponent>("BehaviorTree Component");
	//BBComponent = CreateDefaultSubobject<UBlackboardComponent>("Blackboard Component");
	PSComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	PSComponent->OnSeePawn.AddDynamic(this, &AEnemyAIController::AIOnSeePawn);
	PSComponent->OnHearNoise.AddDynamic(this, &AEnemyAIController::AIHearNoise);

	/* When spawned or respawned. Activate enemy after a timer */
	//Async(EAsyncExecution::TaskGraphMainThread, [this]() {
	//	SetState(ESmallEnemyAIState::RecentlySpawned);
	//	FTimerHandle h;
	//	GetWorldTimerManager().SetTimer(h, [this]() { SetState(ESmallEnemyAIState::Idle); }, TimeSpentRecentlySpawned, false);
	//	});

	//AActor* player = UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass());
	//InitiateChase(player);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TM_AI.Tick(DeltaTime);
	
	switch (m_AIState)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		break;
	case ESmallEnemyAIState::Idle:
		PRINT("IDLE");
		IdleUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::Alerted:
		break;
	case ESmallEnemyAIState::ChasingTarget:
		PRINT("CHASE");
		ChaseUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::GuardSpawn:
		PRINT("GUARD");
		GuardSpawnUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::Attack:
		break;
	case ESmallEnemyAIState::Incapacitated:
		break;
	case ESmallEnemyAIState::None:
		break;
	default:
		break;
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	m_PawnOwner = Cast<ASmallEnemy>(InPawn);
	m_DogPack = m_PawnOwner->m_DogPack.Get();
	m_DogType = m_PawnOwner->m_DogType;
	m_PawnOwner->m_AI = this;

	AActor* player = UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass());
	m_Player = player;

	SetState(ESmallEnemyAIState::RecentlySpawned);
	//ChaseTimedUpdate();
	//m_AIState = ESmallEnemyAIState::ChasingTarget;	// Tmp method of testing ChasingTarget
}

void AEnemyAIController::IdleBegin()
{
	if (!m_SpawnPointData.IsValid()) {
		FTimerHandle h;
		TM_AI.SetTimer(h, this, &AEnemyAIController::IdleBegin, 0.5f);
		return;
	}

	// Red is Guard dog
	if (m_DogType == EDogType::Red)
	{
		m_EIdleState = EIdleState::MoveTo_GuardLocation;
		IdleLocation = m_SpawnPointData->IdleLocation;
		return;
	}

	// Others are simply sleeping next to the guard dog
	m_EIdleState = EIdleState::MovingTo_SleepLocation;
	IdleLocation = GetRandomLocationAroundPoint2D(m_SpawnPointData->IdleLocation, Idle_SleepingLocationRandRadius, Idle_SleepingLocationRandRadius / 3.f);

	DrawDebugLine(GetWorld(), m_SpawnPointData->IdleLocation, IdleLocation, FColor::Red, true, 2.f, -2, 5.f);

}

void AEnemyAIController::IdleUpdate(float DeltaTime)
{
	switch (m_DogType)
	{
	case EDogType::Red:
	{
		IdleUpdate_Red(DeltaTime);
		break;
	}
	case EDogType::Pink:
		IdleUpdate_PinkTeal(DeltaTime);
		break;
	case EDogType::Teal:
		IdleUpdate_PinkTeal(DeltaTime);
		break;
	default:
		break;
	}
}

void AEnemyAIController::IdleUpdate_Red(float DeltaTime)
{
	if (m_EIdleState == EIdleState::MoveTo_GuardLocation)
	{
		EPathFollowingRequestResult::Type result = MoveToLocation(IdleLocation);
		if (result == EPathFollowingRequestResult::AlreadyAtGoal) {
			m_EIdleState = EIdleState::Guard;
		}
	}
	else if (m_EIdleState == EIdleState::Guard)
	{
		PRINT("GUARDING!");
	}
	DrawDebugPoint(GetWorld(), IdleLocation, 40.f, FColor::Black);
}

void AEnemyAIController::IdleUpdate_PinkTeal(float DeltaTime)
{
	if (m_EIdleState == EIdleState::MovingTo_SleepLocation)
	{
		EPathFollowingRequestResult::Type result = MoveToLocation(IdleLocation, Idle_SleepingLocationAcceptanceRadius);
		if (result == EPathFollowingRequestResult::AlreadyAtGoal) 
		{
			m_EIdleState = EIdleState::Sleeping;
			m_PawnOwner->SleepingBegin();
		}
	}
	else if (m_EIdleState == EIdleState::Sleeping)
	{
		PRINT("sleeping...");
	}
	DrawDebugPoint(GetWorld(), IdleLocation, 30.f, FColor::Blue, false, 0, -1);
}

void AEnemyAIController::AIOnSeePawn(APawn* pawn)
{
	return;	// TMP -- no pawn sensing


	// If player was previously heard behind the AI, instantly become hostile

	// Else do a spot check
	FGameplayTag PawnTag;
	SensePawn(pawn, PawnTag);
	if (PawnTag == Tag::Player())
	{
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
	SetState(ESmallEnemyAIState::ChasingTarget);
}

void AEnemyAIController::Attack()
{
	auto player = Cast<ASteikemannCharacter>(m_PawnOwner->m_SensedPawn);
	if (!player) return;
	player->PTakeDamage(1, m_PawnOwner);
}

void AEnemyAIController::SetState(const ESmallEnemyAIState& state)
{
	if (m_AIState == state) return;
	m_AIState = state;

	FTimerHandle h;
	switch (state)
	{
	case ESmallEnemyAIState::RecentlySpawned:
	{
		m_AIState = ESmallEnemyAIState::None;
		GetWorldTimerManager().SetTimer(h, [this]() { SetState(ESmallEnemyAIState::Idle); }, 0.5f, false);
		
		break;
	}
	case ESmallEnemyAIState::Idle:
	{
		IdleBegin();
		break;
	}
	case ESmallEnemyAIState::Alerted:
		break;
	case ESmallEnemyAIState::ChasingTarget:
	{
		ChaseBegin();
		break;
	}
	case ESmallEnemyAIState::GuardSpawn:
		break;
	case ESmallEnemyAIState::Attack:
		break;
	case ESmallEnemyAIState::Incapacitated:
		break;
	case ESmallEnemyAIState::None:
		break;
	default:
		break;
	}
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
	GetWorldTimerManager().SetTimer(TH_IncapacitateTimer, [this, NextState]() { 
		ReDetermineState();
	}, Time, false);
}

void AEnemyAIController::ReDetermineState(const ESmallEnemyAIState& StateOverride)
{
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

void AEnemyAIController::ChaseBegin()
{
}

void AEnemyAIController::ChaseTimedUpdate()
{
	TM_AI.SetTimer(TH_ChaseUpdate, this, &AEnemyAIController::ChaseTimedUpdate, PinkTeal_UpdateTime, false);

	if (!m_Player || !m_DogPack) {
		//if (!m_Player)
		//	PRINTLONG("!PLAYER");
		//if (!m_DogPack)
		//	PRINTLONG("!DOGPACK");
		return;
	}
	FVector Forward{};

	switch (m_DogType)
	{
	case EDogType::Red:
		ChasePlayer_Red_Update();
		break;
	case EDogType::Pink:
		Forward = m_Player->GetActorForwardVector();
		ChasePlayer_CircleAround_Update(Forward);
		break;
	case EDogType::Teal:
	{
		if (!m_DogPack->Red->IsIncapacitated()) {
			Forward = FVector(m_Player->GetActorLocation() - m_DogPack->Red->GetActorLocation()).GetSafeNormal2D();
			ChasePlayer_CircleAround_Update(Forward);
		}
		else if (!m_DogPack->Pink->IsIncapacitated()) {
			Forward = FVector(m_Player->GetActorLocation() - m_DogPack->Pink->GetActorLocation()).GetSafeNormal2D();
			ChasePlayer_CircleAround_Update(Forward);
		}
		else {

			ChasePlayer_Red_Update();
		}
		break;
	}	
	default:
		break;
	}

	//	--- GUARD --- 
	m_GuardLocation = (FVector(PinkTeal_ChaseLocation - m_PawnOwner->m_SpawnPointData->Location).GetSafeNormal() * m_PawnOwner->m_SpawnPointData->Radius_Max) + m_PawnOwner->m_SpawnPointData->Location;

	if (!m_PawnOwner->IsTargetWithinSpawn(m_Player->GetActorLocation())) 
	{
		SetState(ESmallEnemyAIState::GuardSpawn);
	}
}

void AEnemyAIController::ChaseUpdate(float DeltaTime)
{
	switch (m_DogType)
	{
	case EDogType::Red:
	{
		ChasePlayer_Red();

		DrawDebugPoint(GetWorld(), PinkTeal_ChaseLocation, 30.f, FColor::Red, false, 0.f, -1);
		break;
	}
	case EDogType::Pink:
	{
		ChasePlayer_Pink();
		LerpPinkTeal_ChaseLocation(DeltaTime);

		DrawDebugPoint(GetWorld(), PinkTeal_ChaseLocation, 30.f, FColor::Purple, false, 0.f, -1);
		break;
	}
	case EDogType::Teal:
	{
		ChasePlayer_Teal();
		LerpPinkTeal_ChaseLocation(DeltaTime);

		DrawDebugPoint(GetWorld(), PinkTeal_ChaseLocation, 30.f, FColor::Cyan, false, 0.f, -1);
		break;
	}
	default:
		break;
	}

}

void AEnemyAIController::LerpPinkTeal_ChaseLocation(float DeltaTime)
{
	// Updating pink chaselocation
	PinkTeal_ChaseLocation.X = FMath::FInterpTo(PinkTeal_ChaseLocation.X, PinkTeal_ChaseLocation_Target.X, DeltaTime, PinkTeal_SideLength_LerpSpeed);
	PinkTeal_ChaseLocation.Y = FMath::FInterpTo(PinkTeal_ChaseLocation.Y, PinkTeal_ChaseLocation_Target.Y, DeltaTime, PinkTeal_SideLength_LerpSpeed);
	PinkTeal_ChaseLocation.Z = FMath::FInterpTo(PinkTeal_ChaseLocation.Z, PinkTeal_ChaseLocation_Target.Z, DeltaTime, PinkTeal_SideLength_LerpSpeed);
}

void AEnemyAIController::ChasePlayer_Red()
{
	if (!m_Player) return;

	PRINT("Chasing Player :: RED");
	MoveToLocation(PinkTeal_ChaseLocation);
}

void AEnemyAIController::ChasePlayer_Red_Update()
{
	PinkTeal_ChaseLocation = m_Player->GetActorLocation();
}

void AEnemyAIController::ChasePlayer_Pink()
{
	if (!m_Player) return;

	MoveToLocation(PinkTeal_ChaseLocation);
	if (!GetPathFollowingComponent()->HasValidPath())
		MoveToLocation(m_Player->GetActorLocation());

	PRINT("Chasing Player :: PINK");
}

void AEnemyAIController::ChasePlayer_Teal()
{
	if (!m_Player || !m_DogPack) return;

	MoveToLocation(PinkTeal_ChaseLocation);
	if (!GetPathFollowingComponent()->HasValidPath())
		MoveToLocation(m_Player->GetActorLocation());

	PRINT("Chasing Player :: TEAL");
}

void AEnemyAIController::ChasePlayer_CircleAround_Update(const FVector& forward)
{
	FVector playerLoc = m_Player->GetActorLocation();
	bPinkTealCloseToPlayer = FVector::Dist(playerLoc, GetPawn()->GetActorLocation()) <= PinkTeal_AttackActivationRange ||
		FVector::Dist(PinkTeal_ChaseLocation, GetPawn()->GetActorLocation()) <= PinkTeal_AttackActivationRange;
	//if (bPinkCloseToPlayer)	// If close to the player, and PackAttack is available, CHOMP!
	//return;

	FVector forwardLength = forward * PinkTeal_ForwardChaseLength;
	FHitResult Hit;
	FCollisionQueryParams Params("", false, m_Player);
	Params.AddIgnoredActor(GetPawn());
	if (GetWorld()->LineTraceSingleByChannel(Hit, playerLoc, playerLoc + forwardLength, ECC_WorldStatic/* CHANGE TO ECC_WALLS OR SOMETHING */, Params))
	{
		forwardLength = forward * FVector::Dist(playerLoc, Hit.ImpactPoint);
	}

	// Make the dog run a slight cirle around the player by adding to the players right vector and the dogs target location
	FVector right = FVector::CrossProduct(FVector::UpVector, forward);
	FVector FromPlayerToPawn = GetPawn()->GetActorLocation() - playerLoc;
	float Dot = FVector::DotProduct(FromPlayerToPawn.GetSafeNormal2D(), forward);
	FVector rightProj = FromPlayerToPawn.ProjectOnTo(right).GetSafeNormal();

	PinkTeal_ChaseLocation_Target = playerLoc + (forwardLength * DotGuassian(Dot, 0.5f, -0.5f));
	PinkTeal_ChaseLocation_Target += (rightProj * DotGuassian(Dot, 0.5f, -1.f) * PinkTeal_SideLength);

	DrawDebugPoint(GetWorld(), PinkTeal_ChaseLocation_Target, 20.f, FColor::White, false, PinkTeal_UpdateTime, -2);

}

void AEnemyAIController::GuardSpawnUpdate(float DeltaTime)
{
	MoveToLocation(m_GuardLocation);

	if (m_PawnOwner->IsTargetWithinSpawn(m_Player->GetActorLocation()))
	{
		SetState(ESmallEnemyAIState::ChasingTarget);
	}

	DrawDebugPoint(GetWorld(), m_GuardLocation, 30.f, FColor::Blue, false, 0.f, -2);
}



