// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/EnemyAIController.h"
#include "../DebugMacros.h"
#include "../GameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "SmallEnemy.h"
#include "../Spawner/EnemySpawner.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "BaseClasses/StaticVariables.h"

#include "Perception/PawnSensingComponent.h"

AEnemyAIController::AEnemyAIController()
{
	PSComponent = CreateDefaultSubobject<UPawnSensingComponent>("Pawn Sensing Component");
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	PSComponent->OnSeePawn.AddDynamic(this, &AEnemyAIController::AIOnSeePawn);
	PSComponent->OnHearNoise.AddDynamic(this, &AEnemyAIController::AIHearNoise);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!m_PawnOwner) return;

	TM_AI.Tick(DeltaTime);
	
	switch (m_AIState)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		break;
	case ESmallEnemyAIState::Idle:
		IdleUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::Alerted:
		AlertedUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::ChasingTarget:
		ChaseUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::GuardSpawn:
		GuardSpawnUpdate(DeltaTime);
		break;
	case ESmallEnemyAIState::Attack:
		break;
	case ESmallEnemyAIState::Incapacitated:
		StopMovement();
		break;
	case ESmallEnemyAIState::None:
		break;
	default:
		break;
	}
	//PRINT_STATE();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	m_PawnOwner = Cast<ASmallEnemy>(InPawn);
	m_DogPack = m_PawnOwner->m_DogPack.Get();
	m_DogType = m_PawnOwner->m_DogType;
	m_PawnOwner->m_AI = this;
	
	GetPlayerPtr();

	SetState(ESmallEnemyAIState::RecentlySpawned);
}

void AEnemyAIController::RecentlySpawnedBegin()
{
	FTimerHandle h;
	TM_AI.SetTimer(h, [this]() { SetState(ESmallEnemyAIState::Idle); }, 0.5f, false);
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
}

void AEnemyAIController::IdleEnd()
{
	StopMovement();
	m_EIdleState = EIdleState::None;
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
		//PRINT("GUARDING!");
	}
	//DrawDebugPoint(GetWorld(), IdleLocation, 40.f, FColor::Black);
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
		//PRINT("sleeping...");
	}
}

void AEnemyAIController::AIOnSeePawn(APawn* pawn)
{
	// If player was previously heard behind the AI, instantly become hostile
	// Else do a spot check
	FGameplayTag PawnTag = m_PawnOwner->SensingPawn(pawn);
	if (PawnTag == Tag::Player())	// If Seen pawn is player
	{	
		switch (m_AIState)
		{
		case ESmallEnemyAIState::RecentlySpawned:
			break;
		case ESmallEnemyAIState::Idle:
		{
			if (m_EIdleState == EIdleState::Sleeping) break;

			AlertedInit(*pawn);
			AlertedTimeCheck();
			TM_AI.SetTimer(TH_SpotPlayer, this, &AEnemyAIController::SpotPlayer, TimeToSpotPlayer);
			break;
		}
		case ESmallEnemyAIState::Alerted:
		{
			AlertedTimeCheck();
			AlertedInit(*pawn);
			
			break;
		}
		case ESmallEnemyAIState::ChasingTarget:
			break;
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
	// else if sensing other pawns (like fellow dogs)
		// be annoyed
}

void AEnemyAIController::AIHearNoise(APawn* InstigatorPawn, const FVector& Location, float Volume)
{
}


void AEnemyAIController::SpotPlayer()
{
	TM_AI.ClearTimer(TH_StopSensingPlayer);
	TM_AI.ClearTimer(TH_SpotPlayer);

	SetState(ESmallEnemyAIState::ChasingTarget);

	if (m_DogPack) m_DogPack->AlertPack(m_PawnOwner);

	//DrawDebugPoint(GetWorld(), GetPawn()->GetActorLocation() + FVector::UpVector * 100.f, 40.f, FColor::Red, false, 2.f, -2);
}

void AEnemyAIController::AlertedInit(const APawn& instigator)
{
	SetState(ESmallEnemyAIState::Alerted);
	SuspiciousLocation = instigator.GetActorLocation();
	
	//DrawDebugPoint(GetWorld(), SuspiciousLocation, 40.f, FColor::Orange, false, PSComponent->SensingInterval, -2);
}

void AEnemyAIController::AlertedBegin()
{
	TM_AI.SetTimer(TH_StopSensingPlayer, this, &AEnemyAIController::StopSensingPlayer, PSComponent->SensingInterval + 0.4f);
	m_EIdleState = EIdleState::None;
	if (m_DogType == EDogType::Red)
		m_PawnOwner->SpottingPlayer_Begin();
}

void AEnemyAIController::AlertedEnd()
{
	TM_AI.ClearTimer(TH_StopSensingPlayer);
	if (m_DogType == EDogType::Red)
		m_PawnOwner->SpottingPlayer_End();
}

void AEnemyAIController::AlertedUpdate(float DeltaTime)
{
	m_PawnOwner->RotateActorYawToVector(FVector(SuspiciousLocation - GetPawn()->GetActorLocation()), DeltaTime);
	//DrawDebugLine(GetWorld(), GetPawn()->GetActorLocation(), GetPawn()->GetActorLocation() + FVector(SuspiciousLocation - GetPawn()->GetActorLocation()), FColor::Orange);
}

void AEnemyAIController::AlertedTimeCheck()
{
	TM_AI.SetTimer(TH_StopSensingPlayer, this, &AEnemyAIController::StopSensingPlayer, PSComponent->SensingInterval + 0.4f);
	bIsSensingPawn = true;

	//DrawDebugPoint(GetWorld(), GetPawn()->GetActorLocation() + FVector::UpVector * 100.f, 40.f, FColor::Orange, false, PSComponent->SensingInterval);
}

void AEnemyAIController::StopSensingPlayer()
{
	//PRINTLONG("STOP SENSING PLAYER");
	//DrawDebugPoint(GetWorld(), GetPawn()->GetActorLocation() + FVector::UpVector * 100.f, 40.f, FColor::Blue, false, 1.f, -1);

	TM_AI.ClearTimer(TH_StopSensingPlayer);
	TM_AI.ClearTimer(TH_SpotPlayer);

	bIsSensingPawn = false;
	SetState(ESmallEnemyAIState::Idle);

	if (m_AIState != ESmallEnemyAIState::Alerted || m_AIState != ESmallEnemyAIState::Idle)
		return;
}

bool AEnemyAIController::AlertedByPack()
{
	switch (m_AIState)
	{
	case ESmallEnemyAIState::RecentlySpawned:			break;
	case ESmallEnemyAIState::Idle:						break;
	case ESmallEnemyAIState::Alerted:					break;
	case ESmallEnemyAIState::ChasingTarget:				return false;
	case ESmallEnemyAIState::GuardSpawn:				break;
	case ESmallEnemyAIState::Attack:					return false;
	case ESmallEnemyAIState::Incapacitated:				return false;
	case ESmallEnemyAIState::None:						break;
	default:
		break;
	}

	SetState(ESmallEnemyAIState::ChasingTarget);
	return true;
}

bool AEnemyAIController::CanAttack_AI() const
{
	if (m_AIState == ESmallEnemyAIState::Incapacitated)		return false;
	if (!m_PawnOwner->CanAttack_Pawn())						return false;
	return true;
}

void AEnemyAIController::AttackBegin()
{
	StopMovement();
	m_PawnOwner->RotateActorYawToVector(m_Player->GetActorLocation() - m_PawnOwner->GetActorLocation());
	AttackJump();// Should maybe be an animation event
	TM_AI.SetTimer(TH_AttackState, [this]() { SetState(ESmallEnemyAIState::ChasingTarget); } , AttackStateTime, false);
}

void AEnemyAIController::AttackEnd()
{
}

void AEnemyAIController::Attack()
{
	m_PawnOwner->CHOMP_Pure();
}

void AEnemyAIController::AttackJump()
{
	m_PawnOwner->AttackJump();
	TM_AI.SetTimer(TH_PreAttack, this, &AEnemyAIController::Attack, JumpToChompTime);
}

void AEnemyAIController::CancelAttackJump()
{
	TM_AI.ClearTimer(TH_PreAttack);
	TM_AI.ClearTimer(TH_AttackState);
}

void AEnemyAIController::ReceiveAttack()
{

}

void AEnemyAIController::SetState(const ESmallEnemyAIState& state)
{
	if (m_AIIncapacitatedType != EAIIncapacitatedType::None) return;
	// CHECK IF AI CAN LEAVE STATE
	// if (!CanLeaveState(ESmallEnemyAIState)) return;

	AlwaysBeginStates(state);
	if (m_AIState == state) return;
	LeaveState(m_AIState, state);
	m_AIState = state;

	switch (state)
	{
	case ESmallEnemyAIState::RecentlySpawned:	RecentlySpawnedBegin();		break;
	case ESmallEnemyAIState::Idle:				IdleBegin();				break;
	case ESmallEnemyAIState::Alerted:			AlertedBegin();				break;
	case ESmallEnemyAIState::ChasingTarget:		ChaseBegin();				break;
	case ESmallEnemyAIState::GuardSpawn:		GuardSpawnBegin();			break;
	case ESmallEnemyAIState::Attack:			AttackBegin();				break;
	case ESmallEnemyAIState::Incapacitated:									break;
	case ESmallEnemyAIState::None:											break;
	default:
		break;
	}

	//Print_SetState(state);
	//PRINT_SETSTATE(state);
}

void AEnemyAIController::AlwaysBeginStates(const ESmallEnemyAIState& incommingState)
{
	switch (incommingState)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		break;
	case ESmallEnemyAIState::Idle:
		break;
	case ESmallEnemyAIState::Alerted:
	{
		AlertedBegin();
		break;
	}
	case ESmallEnemyAIState::ChasingTarget:
		break;
	case ESmallEnemyAIState::GuardSpawn:
		break;
	case ESmallEnemyAIState::Attack:
		break;
	case ESmallEnemyAIState::Incapacitated:
	{
		IncapacitateBegin();
		break;
	}
	case ESmallEnemyAIState::None:
		break;
	default:
		break;
	}
}

void AEnemyAIController::LeaveState(const ESmallEnemyAIState& currentState, const ESmallEnemyAIState& incommingState)
{
	switch (currentState)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		break;
	case ESmallEnemyAIState::Idle:
	{
		IdleEnd();
		break;
	}
	case ESmallEnemyAIState::Alerted:
	{
		AlertedEnd();
		break;
	}
	case ESmallEnemyAIState::ChasingTarget:
	{
		ChaseEnd();
		break;
	}
	case ESmallEnemyAIState::GuardSpawn:
	{
		GuardSpawnEnd();
		break;
	}
	case ESmallEnemyAIState::Attack:
	{
		AttackEnd();
		break;
	}
	case ESmallEnemyAIState::Incapacitated:
		IncapacitateEnd();
		break;
	case ESmallEnemyAIState::None:
		break;
	default:
		break;
	}
}

void AEnemyAIController::IncapacitateBegin()
{
	AlertedEnd();
	ChaseEnd();
	GuardSpawnEnd();
	CancelAttackJump();
	StopMovement();
}

void AEnemyAIController::IncapacitateEnd()
{
	m_AIIncapacitatedType = EAIIncapacitatedType::None;
	m_PawnOwner->Post_IncapacitateDetermineState();
}

void AEnemyAIController::IncapacitateAI(const EAIIncapacitatedType& IncapacitateType, float Time/*, const ESmallEnemyAIState& NextState*/)
{
	TM_AI.ClearTimer(TH_IncapacitateTimer);
	SetState(ESmallEnemyAIState::Incapacitated);
	m_AIIncapacitatedType = IncapacitateType;
	// AI Incapacitated for an undetermined amount of time
	if (Time < 0.f)	return;

	TM_AI.SetTimer(TH_IncapacitateTimer, this, &AEnemyAIController::Post_Incapacitate_GettingSmacked, Time);
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

void AEnemyAIController::Incapacitate_GettingSmacked()
{
	m_EAIPost_IncapacitatedType = EAIPost_IncapacitatedType::ChasePlayer;
}

void AEnemyAIController::Post_Incapacitate_GettingSmacked()
{
	m_AIIncapacitatedType = EAIIncapacitatedType::None;

	switch (m_EAIPost_IncapacitatedType)
	{
	case EAIPost_IncapacitatedType::None:
		SetState(ESmallEnemyAIState::Idle);
		break;
	case EAIPost_IncapacitatedType::ChasePlayer:
		if (FVector(m_PawnOwner->GetActorLocation() - m_Player->GetActorLocation()).SquaredLength() < FMath::Square(PostGettingSmackedActivationRange))
			SetState(ESmallEnemyAIState::ChasingTarget);
		break;
	case EAIPost_IncapacitatedType::ConfusedScreaming:
		SetState(ESmallEnemyAIState::Idle);
		//SetState(ESmallEnemyAIState::);		// Need a confused screaming state. Where the dog looks around after whatever just hit it. 
		break;
	default:
		break;
	}
}

void AEnemyAIController::RedetermineIncapacitateState()
{
	if (FVector(m_PawnOwner->GetActorLocation() - m_Player->GetActorLocation()).SquaredLength() < FMath::Square(PostGettingSmackedActivationRange))
		m_EAIPost_IncapacitatedType = EAIPost_IncapacitatedType::ChasePlayer;
	else
		m_EAIPost_IncapacitatedType = EAIPost_IncapacitatedType::ConfusedScreaming;

	IncapacitateEnd();
	Post_Incapacitate_GettingSmacked();
}

bool AEnemyAIController::IsIncapacitated() const
{
	return m_AIState == ESmallEnemyAIState::Incapacitated;
}


void AEnemyAIController::ChaseBegin()
{
	//TM_AI.SetTimer(TH_ChaseUpdate, this, &AEnemyAIController::ChaseTimedUpdate, PinkTeal_UpdateTime, false);
	ChaseTimedUpdate();
	ChaseUpdate(GetWorld()->GetDeltaSeconds());
	SetChaseState_Start();
}

void AEnemyAIController::ChaseEnd()
{
	TM_AI.ClearTimer(TH_ChaseUpdate);

	TM_AI.ClearTimer(TH_Chase_InvalidPath_Bark);
	TM_AI.ClearTimer(TH_Chase_InvalidPath_JumpAttempt);
}

void AEnemyAIController::ChaseTimedUpdate()
{
	TM_AI.SetTimer(TH_ChaseUpdate, this, &AEnemyAIController::ChaseTimedUpdate, PinkTeal_UpdateTime, false);

	if (!m_Player || !m_DogPack) {
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
	if (TM_AI.IsTimerActive(TH_DisabledChaseMovement)) return;
	Evaluate_ChaseState();

	switch (m_DogType)
	{
	case EDogType::Red:
	{
		ChasePlayer_Red_Update();
		ChasePlayer_Red();
		break;
	}
	case EDogType::Pink:
	{
		ChasePlayer_Pink();
		LerpPinkTeal_ChaseLocation(DeltaTime);
		break;
	}
	case EDogType::Teal:
	{
		ChasePlayer_Teal();
		LerpPinkTeal_ChaseLocation(DeltaTime);
		break;
	}
	default:
		break;
	}

	if (m_Player)
		if (CanAttack_AI())
			if (FVector::Dist(GetPawn()->GetActorLocation(), m_Player->GetActorLocation()) < AttackDistance)
				SetState(ESmallEnemyAIState::Attack);
}

void AEnemyAIController::LerpPinkTeal_ChaseLocation(float DeltaTime)
{
	// Updating pink chaselocation
	PinkTeal_ChaseLocation.X = FMath::FInterpTo(PinkTeal_ChaseLocation.X, PinkTeal_ChaseLocation_Target.X, DeltaTime, PinkTeal_SideLength_LerpSpeed);
	PinkTeal_ChaseLocation.Y = FMath::FInterpTo(PinkTeal_ChaseLocation.Y, PinkTeal_ChaseLocation_Target.Y, DeltaTime, PinkTeal_SideLength_LerpSpeed);
	PinkTeal_ChaseLocation.Z = FMath::FInterpTo(PinkTeal_ChaseLocation.Z, PinkTeal_ChaseLocation_Target.Z, DeltaTime, PinkTeal_SideLength_LerpSpeed);
}

void AEnemyAIController::GetPlayerPtr()
{
	AActor* player = UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass());
	m_Player = player;
	if (!player) {
		FTimerHandle h;
		GetWorldTimerManager().SetTimer(h, this, &AEnemyAIController::GetPlayerPtr, 0.5f);
	}
}

void AEnemyAIController::SetChaseState_Start()
{
	Evaluate_ChaseState();
}

void AEnemyAIController::SetChaseState(const EAI_ChaseState NewChaseState)
{
	if (m_EAI_ChaseState == NewChaseState) return;
	m_EAI_ChaseState = NewChaseState;

	switch (m_EAI_ChaseState)
	{
	case EAI_ChaseState::None:
		break;
	case EAI_ChaseState::ChasePlayer_ValidPath:
		TM_AI.ClearTimer(TH_Chase_InvalidPath_Bark);
		TM_AI.ClearTimer(TH_Chase_InvalidPath_JumpAttempt);
		break;
	case EAI_ChaseState::ChasePlayer_InvalidPath:
		ChaseState_InvalidPath_Begin();
		break;
	default:
		break;
	}
}

void AEnemyAIController::Evaluate_ChaseState()
{
	if (GetPathFollowingComponent()->HasValidPath())
		SetChaseState(EAI_ChaseState::ChasePlayer_ValidPath);
	else
		SetChaseState(EAI_ChaseState::ChasePlayer_InvalidPath);
}

void AEnemyAIController::ChaseState_InvalidPath_Begin()
{
	const float barktime = FMath::RandRange(ChaseState_Bark_Interval_Min, ChaseState_Bark_Interval_Min + ChaseState_Bark_Interval_Addition);
	TM_AI.SetTimer(TH_Chase_InvalidPath_Bark, this, &AEnemyAIController::ChaseState_Bark, barktime);

	const float corgijumptime = ChaseState_CorgiJumpTime;
	TM_AI.SetTimer(TH_Chase_InvalidPath_JumpAttempt, this, &AEnemyAIController::ChaseState_CorgiJump, corgijumptime);
}

void AEnemyAIController::ChaseState_Bark()
{
	const float time = FMath::RandRange(ChaseState_Bark_Interval_Min, ChaseState_Bark_Interval_Min + ChaseState_Bark_Interval_Addition);
	TM_AI.SetTimer(TH_Chase_InvalidPath_Bark, this, &AEnemyAIController::ChaseState_Bark, time);

	m_PawnOwner->Bark_Pure();
	const float dmTime = FMath::RandRange(ChaseState_Bark_DisableMovementTimer - 0.2f, ChaseState_Bark_DisableMovementTimer + 0.2f);
	TM_AI.SetTimer(TH_DisabledChaseMovement, dmTime, false);
}

void AEnemyAIController::ChaseState_CorgiJump()
{
	TM_AI.ClearTimer(TH_Chase_InvalidPath_Bark);
	m_PawnOwner->CorgiJump_Pure();
}

void AEnemyAIController::ChasePlayer_Red()
{
	if (!m_Player) return;

	//PRINT("Chasing Player :: RED");
	MoveToLocation(PinkTeal_ChaseLocation);
}

void AEnemyAIController::ChasePlayer_Red_Update()
{
	if (m_Player)
		PinkTeal_ChaseLocation = m_Player->GetActorLocation();
}

void AEnemyAIController::ChasePlayer_Pink()
{
	if (!m_Player) return;

	MoveToLocation(PinkTeal_ChaseLocation);
	if (!GetPathFollowingComponent()->HasValidPath())
		MoveToLocation(m_Player->GetActorLocation());

	//PRINT("Chasing Player :: PINK");
}

void AEnemyAIController::ChasePlayer_Teal()
{
	if (!m_Player || !m_DogPack) return;

	MoveToLocation(PinkTeal_ChaseLocation);
	if (!GetPathFollowingComponent()->HasValidPath())
		MoveToLocation(m_Player->GetActorLocation());

	//PRINT("Chasing Player :: TEAL");
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

	PinkTeal_ChaseLocation_Target = playerLoc + (forwardLength * SMath:: DotGuassian(Dot, 0.5f, -0.5f));
	PinkTeal_ChaseLocation_Target += (rightProj * SMath::DotGuassian(Dot, 0.5f, -1.f) * PinkTeal_SideLength);

	//DrawDebugPoint(GetWorld(), PinkTeal_ChaseLocation_Target, 20.f, FColor::White, false, PinkTeal_UpdateTime, -2);

}

void AEnemyAIController::GuardSpawnUpdate(float DeltaTime)
{
	MoveToLocation(m_GuardLocation);

	if (m_PawnOwner->IsTargetWithinSpawn(m_Player->GetActorLocation()))
	{
		SetState(ESmallEnemyAIState::ChasingTarget);
	}

	//DrawDebugPoint(GetWorld(), m_GuardLocation, 30.f, FColor::Blue, false, 0.f, -2);
}

void AEnemyAIController::GuardSpawnBegin()
{
	ChaseBegin();
	TM_AI.SetTimer(TH_GuardSpawn, [this]() { SetState(ESmallEnemyAIState::Idle); }, GuardSpawn_Time, false);
}

void AEnemyAIController::GuardSpawnEnd()
{
	ChaseEnd();
	TM_AI.ClearTimer(TH_GuardSpawn);
}

#ifdef UE_BUILD_DEBUG
void AEnemyAIController::PrintState()
{
	FString s;
	FString t;
	switch (m_AIState)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		s += "RECENTLY SPAWNED";
		break;
	case ESmallEnemyAIState::Idle:
		s += "IDLE";
		break;
	case ESmallEnemyAIState::Alerted:
		s += "ALERTED";
		break;
	case ESmallEnemyAIState::ChasingTarget:
		s += "CHASE";
		PRINT_ChaseState();
		break;
	case ESmallEnemyAIState::GuardSpawn:
		s += "GUARD";
		break;
	case ESmallEnemyAIState::Attack:
		s += "ATTACKING";
		break;
	case ESmallEnemyAIState::Incapacitated:
		s += "INCAPACITATED";
		break;
	case ESmallEnemyAIState::None:
		s += "NONE";
		break;
	default:
		break;
	}

	switch (m_DogType)
	{
	case EDogType::Red:
		t += "Red";
		break;
	case EDogType::Pink:
		t += "Pink";
		break;
	case EDogType::Teal:
		t += "Teal";
		break;
	default:
		break;
	}

	PRINTPAR("%s - State :: %s", *t, *s);
}

void AEnemyAIController::Print_SetState(ESmallEnemyAIState state)
{
	FString s = "Set State To: ";
	switch (state)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		s += "Recently Spawned";
		break;
	case ESmallEnemyAIState::Idle:
		s += "Idle";
		break;
	case ESmallEnemyAIState::Alerted:
		s += "Alerted";
		break;
	case ESmallEnemyAIState::ChasingTarget:
		s += "Chasing Target";
		break;
	case ESmallEnemyAIState::GuardSpawn:
		s += "Guard Spawn";
		break;
	case ESmallEnemyAIState::Attack:
		s += "Attack";
		break;
	case ESmallEnemyAIState::Incapacitated:
		s += "Incapacitated";
		break;
	case ESmallEnemyAIState::None:
		s += "None";
		break;
	default:
		break;
	}
	s += "\nLeaving State: ";
	// Leaving State
	switch (m_AIState)
	{
	case ESmallEnemyAIState::RecentlySpawned:
		s += "Recently Spawned";
		break;
	case ESmallEnemyAIState::Idle:
		s += "Idle";
		break;
	case ESmallEnemyAIState::Alerted:
		s += "Alerted";
		break;
	case ESmallEnemyAIState::ChasingTarget:
		s += "Chasing Target";
		break;
	case ESmallEnemyAIState::GuardSpawn:
		s += "Guard Spawn";
		break;
	case ESmallEnemyAIState::Attack:
		s += "Attack";
		break;
	case ESmallEnemyAIState::Incapacitated:
		s += "Incapacitated";
		break;
	case ESmallEnemyAIState::None:
		s += "None";
		break;
	default:
		break;
	}
	PRINTPARLONG(1.f, "%s", *s);
}

void AEnemyAIController::PRINT_ChaseState()
{
	FString s = "ChaseState = ";
	switch (m_EAI_ChaseState)
	{
	case EAI_ChaseState::None:
		s += "None";
		break;
	case EAI_ChaseState::ChasePlayer_ValidPath:
		s += "Chaseplayer_ValidPath";
		break;
	case EAI_ChaseState::ChasePlayer_InvalidPath:
		s += "Chaseplayer_InvalidPath";
		break;
	default:
		break;
	}
	PRINTPAR("%s", *s);
}
#endif
