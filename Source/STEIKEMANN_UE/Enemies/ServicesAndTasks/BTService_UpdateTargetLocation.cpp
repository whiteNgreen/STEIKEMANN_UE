// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdateTargetLocation.h"
#include "../EnemyAIController.h"


void UBTService_UpdateTargetLocation::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AEnemyAIController* owner = Cast<AEnemyAIController>(OwnerComp.GetOwner());
	owner->UpdateTargetPosition();
}
