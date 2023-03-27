// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Collectible.h"
#include "CollectibleSpawner.generated.h"

UENUM()
enum class ECollectibleSpawnerType
{
	BoxVolume,
	Spline
};

UCLASS()
class STEIKEMANN_UE_API ACollectibleSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACollectibleSpawner();

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Components")
		UPrimitiveComponent* SpawnerComponent { nullptr };


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Type")
		ECollectibleSpawnerType Type;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (UIMin = "0", UIMax = "15"))
		int SpawnAmount{ 5 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (UIMin = "0", UIMax = "3000"))
		int RandomSeed{ 3 };


	/* Should the collectibles Z placement be aligned with the ground? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
		bool bPlaceOnGround{};
	/* How far above the ground should the collectible be placed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (EditCondition = "bPlaceOnGround", EditConditionHides))
		float ZGroundPlacement{ 80.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
		TSubclassOf<ACollectible> SpawnedCollectible;

public:
	UFUNCTION(BlueprintCallable)
		void SpawnCollectibles();

	UFUNCTION(BlueprintCallable)
		void InitRandomSeed();
	UFUNCTION(BlueprintCallable)
		FVector GetRandomBoxLocation(UBoxComponent* box);

private:
	void SplineSpawnCollectibles();
	void BoxSpawnCollectibles();

	float TraceZLocation(FVector& loc, bool& traceSuccess, int index = -1);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
