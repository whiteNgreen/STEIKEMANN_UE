// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Attack.h"
#include "../EnemyAIController.h"

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	auto ai = Cast<AEnemyAIController>(OwnerComp.GetOwner());
	ai->Attack();

	return EBTNodeResult::Succeeded;
}
