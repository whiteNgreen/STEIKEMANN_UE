// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../GameplayTags.h"
#include "../Interfaces/AttackInterface.h"
#include "../Enemies/SmallEnemy.h"
#include "GameFramework/Actor.h"

#ifdef WITH_EDITOR
#include "Components/BillboardComponent.h"
#endif

#include "EnemySpawner.generated.h"

UENUM()
enum EEnemySpawnType
{
	AubergineDog,
	Character,
	Other
};

struct EDogPack
{
	ASmallEnemy* Red{ nullptr };
	ASmallEnemy* Pink{ nullptr };
	ASmallEnemy* Teal{ nullptr };

	void AlertPack(ASmallEnemy* Instigator);
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
		USceneComponent* SpawnPoint;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USceneComponent* IdlePoint;

#ifdef WITH_EDITOR
		UBillboardComponent* IdlePointSprite;
#endif // WITH_EDITOR


public: // Variables
	FGameplayTagContainer GameplayTags;
	int TypeIndex{};
	TSharedPtr<EDogPack> m_AuberginePack;
	TSharedPtr<SpawnPointData> SpawnData;


	/* Time between getting attacked */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float CanBeAttackedTimer{ 1.f };

	/* When spawned actor look for a location within spawn, they will not look inside this radius */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float SpawnerMinRadius{ 500.f };
	/* When respawning: Only respawn the actors that are outside of this radius from the spawners root */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float SpawnerActiveRadius{ 2000.f };

	float AngleYaw{};
	float AnglePitch{};
	float LaunchVelocity{};
	FVector m_SpawnLocation{};

	EEnemySpawnType m_EENemySpawnType;

	FTimerHandle TH_BeginSpawnAction;
	FTimerHandle TH_SpawnTimer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Spawn_LaunchStrength{ 1000.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FVector2D PitchRange{ 40.f, 80.f };
	/* Time before spawning actors begin */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Timer_BeginSpawnAction{ 1.f };
	/* Time between spawning actors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float Timer_SpawnIterator{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		uint8 m_SpawnAmount{ 3 };
	uint8 m_SpawnIndex{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSubclassOf<class ASmallEnemy> SpawningActorType;

	TArray<ASmallEnemy*> SpawnedActors;
	TArray<ASmallEnemy*> m_ActorsToRespawn;
	TArray<ASmallEnemy*> m_ActorsToDestroy;

public: // Functions
	void BeginActorSpawn(void(AEnemySpawner::* spawnFunction)());
	void BeginActorRespawn();
	void CheckSpawningActorClass();
	void SpawnActor();

	void DetermineActorsToRespawn(TArray<ASmallEnemy*>& actorsToRespawn);
	void RespawnActors(TArray<ASmallEnemy*>& actorsToRespawn);
	void RespawnActor();

		// GameplayTags Interface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; }

		// Attack Interface
	virtual bool CanBeAttacked() override { return bAICanBeDamaged; }
	virtual void Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType) override;

public: // Functions - Called by SpawnedActor

private: // PRIVATE Functions
	void SpawnAubergineDog(int& index);
	ASmallEnemy* SpawnCharacter();

	//void SpawnAuberginePack();
};
