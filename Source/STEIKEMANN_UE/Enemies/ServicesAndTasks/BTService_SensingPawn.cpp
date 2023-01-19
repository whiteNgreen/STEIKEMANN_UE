// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_SensingPawn.h"
#include "../../DebugMacros.h"
#include "../EnemyAIController.h"

void UBTService_SensingPawn::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	//PRINTPARLONG("Service DeltaSeconds = %f", DeltaSeconds);
	auto ai = Cast<AEnemyAIController>(OwnerComp.GetOwner());
	ai->SensedPawnsDelegate.Broadcast();
}
