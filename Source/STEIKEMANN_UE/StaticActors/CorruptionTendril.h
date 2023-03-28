// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "CorruptionTendril.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API ACorruptionTendril : public ABaseStaticActor
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void TendrilPulse_Start();
	UFUNCTION(BlueprintImplementableEvent)
		void DestroyTendril_Start();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void DestroyTendril_End();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<class ACorruptionWall*> ConnectedWalls;
};
