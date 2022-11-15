// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "DeathZone.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API ADeathZone : public ABaseStaticActor
{
	GENERATED_BODY()

public:
	ADeathZone();
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		UBoxComponent* BoxCollider{ nullptr };

};
