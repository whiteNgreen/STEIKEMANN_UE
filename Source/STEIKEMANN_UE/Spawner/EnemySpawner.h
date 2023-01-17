// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../GameplayTags.h"
#include "../Interfaces/AttackInterface.h"
#include "GameFramework/Actor.h"
#include "EnemySpawner.generated.h"

UENUM()
enum EEnemySpawnType
{
	AubergineDog,
	Character,
	Other
};

UCLASS()
class STEIKEMANN_UE_API AEnemySpawner : public AActor,
	public IGameplayTagAssetInterface,
	public IAttackInterface
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
	UPROPERTY(BlueprintReadWrite)
		USceneComponent* Root;
	UPROPERTY(BlueprintReadWrite)
		USceneComponent* SpawnPoint;

public: // Variables
	FGameplayTagContainer GameplayTags;

	/* Time between getting attacked */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float CanBeAttackedTimer{ 1.f };

	/* When respawning: Only respawn the actors that are outside of this radius from the spawners root */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float SpawnerActiveRadius{ 2000.f };

	float AngleYaw{};
	float AnglePitch{};
	float LaunchVelocity{};
	FVector m_SpawnLocation{};

	EEnemySpawnType m_EENemySpawnType;

	FTimerHandle TH_SpawnTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Spawn_LaunchStrength{ 1000.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D PitchRange{ 40.f, 80.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float m_SpawnTimer{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		uint8 m_SpawnAmount{ 3 };
	uint8 m_SpawnIndex{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSubclassOf<class ASmallEnemy> SpawningActorType;

	TArray<ASmallEnemy*> SpawnedActors;

public: // Functions
	void CheckSpawningActorClass();
	void SpawnActor();
	float RandomFloat(float min, float max);

	void DetermineActorsToRespawn(TArray<ASmallEnemy*>& actorsToRespawn);
	void RespawnActors(TArray<ASmallEnemy*>& actorsToRespawn);
	void RespawnActor();

		// GameplayTags Interface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }

		// Attack Interface
	virtual bool CanBeAttacked() override { return bAICanBeDamaged; }
	virtual void Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType) override;

private: // Functions
	void SpawnAubergineDog();
	ACharacter* SpawnCharacter();

	//void DetermineActorsToRespawn();
};
