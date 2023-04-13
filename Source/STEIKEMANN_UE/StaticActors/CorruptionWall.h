// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "CorruptionWall.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API ACorruptionWall : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
		void OpenWall_Pure();
	UFUNCTION(BlueprintImplementableEvent)
		void OpenWall_Impl();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void MeatWall_Pulse();
};
