// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"


UCLASS()
class STEIKEMANN_UE_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnemySpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public: // Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USceneComponent* Root;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USceneComponent* SpawnPoint;

public: // Variables
	float AngleYaw{};
	float AnglePitch{};
	float LaunchVelocity{};

	FTimerHandle TH_SpawnTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		uint8 m_SpawnAmount{ 3 };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSubclassOf<AActor> SpawningActor;

public: // Functions
	void SpawnActor();
	float RandomFloat(float min, float max);
};
