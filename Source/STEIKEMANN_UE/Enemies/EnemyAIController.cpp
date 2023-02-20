// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/EnemyAIController.h"
#include "../DebugMacros.h"
#include "../GameplayTags.h"

#include "Kismet/GameplayStatics.h"
#include "SmallEnemy.h"
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

	AActor* player = UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass());
	InitiateChase(player);
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
		break;
	case ESmallEnemyAIState::Alerted:
		break;
	case ESmallEnemyAIState::ChasingTarget:
		ChaseUpdate(DeltaTime);
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

	SetState(ESmallEnemyAIState::Idle);

	AActor* player = UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass());
	InitiateChase(player);
}

void AEnemyAIController::AIOnSeePawn(APawn* pawn)
{
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

void AEnemyAIController::InitiateChase(AActor* Player)
{
	m_Player = Player;
	m_AIState = ESmallEnemyAIState::ChasingTarget;

	if (!Player) return;
	switch (m_DogType)
	{
	case EDogType::Red:
		ChasePlayer_Red_Update();
		break;
	case EDogType::Pink:
		ChasePlayer_Pink_Update();
		break;
	case EDogType::Teal:
		break;
	default:
		break;
	}
}

void AEnemyAIController::ChaseUpdate(float DeltaTime)
{
	switch (m_DogType)
	{
	case EDogType::Red:
		ChasePlayer_Red();
		break;
	case EDogType::Pink:
		ChasePlayer_Pink();
		break;
	case EDogType::Teal:
		break;
	default:
		break;
	}

	// Updating pink chaselocation
	Pink_ChaseLocation.X = FMath::FInterpTo(Pink_ChaseLocation.X, Pink_ChaseLocation_Target.X, DeltaTime, Pink_SideLength_LerpSpeed);
	Pink_ChaseLocation.Y = FMath::FInterpTo(Pink_ChaseLocation.Y, Pink_ChaseLocation_Target.Y, DeltaTime, Pink_SideLength_LerpSpeed);
	Pink_ChaseLocation.Z = FMath::FInterpTo(Pink_ChaseLocation.Z, Pink_ChaseLocation_Target.Z, DeltaTime, Pink_SideLength_LerpSpeed);
}

void AEnemyAIController::ChasePlayer_Red()
{
	if (!m_Player) return;

	PRINT("Chasing Player :: RED");
	MoveToLocation(m_Player->GetActorLocation());
}

void AEnemyAIController::ChasePlayer_Red_Update()
{
}

void AEnemyAIController::ChasePlayer_Pink()
{
	if (!m_Player) return;

	ChasePlayer_Pink_Update();
	
	//if (bPinkCloseToPlayer) {
		//PRINT("Direct to player");
		//MoveToLocation(m_Player->GetActorLocation());
	//}
	//else {

		MoveToLocation(Pink_ChaseLocation);
	//}
	PRINT("Chasing Player :: PINK");
}

void AEnemyAIController::ChasePlayer_Pink_Update()
{
	//TM_AI.SetTimer(TH_ChaseUpdate, this, &AEnemyAIController::ChasePlayer_Pink_Update, 0.2f);
	
	FVector playerLoc = m_Player->GetActorLocation();
	FVector forward = m_Player->GetActorForwardVector();

	bPinkCloseToPlayer = FVector::Dist(playerLoc, GetPawn()->GetActorLocation()) <= Pink_ActivationRange ||
		FVector::Dist(Pink_ChaseLocation, GetPawn()->GetActorLocation()) <= Pink_ActivationRange;
	//if (bPinkCloseToPlayer)	// If close to the player, and PackAttack is available, CHOMP!
	//return;

	FVector forwardLength = forward * Pink_ForwardChaseLength;

	FHitResult Hit;
	FCollisionQueryParams Params("", false, m_Player);
	Params.AddIgnoredActor(GetPawn());
	if (GetWorld()->LineTraceSingleByChannel(Hit, playerLoc, playerLoc + forwardLength, ECC_WorldStatic/* CHANGE TO ECC_WALLS OR SOMETHING */, Params))
	{
		forwardLength = forward * FVector::Dist(playerLoc, Hit.ImpactPoint);
	}


	// Make the dog run a slight cirle around the player by adding to the players right vector and the dogs target location
	FVector right = m_Player->GetActorRightVector();
	FVector FromPlayerToPawn = GetPawn()->GetActorLocation() - playerLoc;
	float Dot = FVector::DotProduct(FromPlayerToPawn.GetSafeNormal2D(), forward);
	FVector rightProj = FromPlayerToPawn.ProjectOnTo(right).GetSafeNormal();

	Pink_ChaseLocation_Target = playerLoc + (forwardLength * DotGuassian(Dot, 0.5f, -0.5f));
	Pink_ChaseLocation_Target += (rightProj * DotGuassian(Dot, 0.5f, -1.f) * Pink_SideLength);

	PRINTPAR("Dot Inverted = %f", DotGuassian(Dot, 0.5f, -1.f));
	PRINTPAR("Dot Guassian = %f", DotGuassian(Dot, 0.5f, -0.5f));

	DrawDebugPoint(GetWorld(), Pink_ChaseLocation_Target, 20.f, FColor::Red, false, 0, -2);
	DrawDebugPoint(GetWorld(), Pink_ChaseLocation, 30.f, FColor::Purple, false, 0, -1);
}



