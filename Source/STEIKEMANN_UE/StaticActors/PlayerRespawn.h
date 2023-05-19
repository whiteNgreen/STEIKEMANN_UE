// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "PlayerRespawn.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API APlayerRespawn : public ABaseStaticActor
{
	GENERATED_BODY()

public:
	APlayerRespawn();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		UBoxComponent* BoxCollider{ nullptr };
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		USphereComponent* SphereCollider{ nullptr };


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SpawnPoint")
		UBoxComponent* SpawnPoint{ nullptr };
		
	FTransform GetSpawnTransform();
	FTimerManager TimerManager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
