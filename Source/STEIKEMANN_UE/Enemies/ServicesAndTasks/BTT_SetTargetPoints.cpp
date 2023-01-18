// Fill out your copyright notice in the Description page of Project Settings.


#include "BTT_SetTargetPoints.h"
#include "../../DebugMacros.h"
#include "../EnemyAIController.h"
#include "../SmallEnemy.h"

EBTNodeResult::Type UBTT_SetTargetPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    PRINTLONG("SET TARGET POINTS");
    auto c = Cast<AEnemyAIController>(OwnerComp.GetOwner());
    c->SetNewTargetPoints();

    return EBTNodeResult::Type::Succeeded;
}
