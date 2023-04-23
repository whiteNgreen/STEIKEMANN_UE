// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "HealthPickupSpawner.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API APickupSpawner : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:
	APickupSpawner();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USceneComponent* HealthPickupPlacement;
	UFUNCTION(BlueprintCallable)
		void RespawnPickup();
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TSubclassOf<class ACollectible> m_PickupClass;
	//UFUNCTION(BlueprintImplementableEvent)
	void SpawnPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float PickupSpawnTime{ 3.f };
	FTimerHandle TH_PickupRespawn;
};
