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

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PRINTPARLONG("AI Possessed %s", *InPawn->GetName());
	InitializeBlackboard(*BBComponent, *BB);
	SetState_Idle();
	
	BBComponent->SetValueAsObject("Player", UGameplayStatics::GetActorOfClass(GetWorld(), ASteikemannCharacter::StaticClass()));
	BBComponent->SetValueAsVector("APosition", FVector(0, 0, 0));
	BBComponent->SetValueAsBool("bFollowPlayer", bFollowPlayer);
}

void AEnemyAIController::SetState_Idle()
{
	BTComponent->StopTree();
	BTComponent->StartTree(*BTIdle);
}

void AEnemyAIController::SetState_Attack()
{
	BTComponent->StopTree();
	BTComponent->StartTree(*BTAttack);
}

