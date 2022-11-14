// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../GameplayTags.h"
#include "../STEIKEMANN_UE.h"

#include "Collectible.generated.h"

UENUM()
enum class ECollectibleType : uint8
{
	Common,
	Health,
	CorruptionCore
};

UCLASS()
class STEIKEMANN_UE_API ACollectible : public AActor,
	public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACollectible();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		USceneComponent* Root { nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UStaticMeshComponent* Mesh { nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class USphereComponent* Sphere{ nullptr };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		ECollectibleType CollectibleType;

	FGameplayTagContainer GTagContainer;
	void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GTagContainer; }

	UFUNCTION()
		void OnCollectibleBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particles")
		class UNiagaraSystem* DeathParticles{ nullptr };

	/* Buffer/Timer before collectible can be collected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnSpawn")
		float InitTimer{ 0.5f };
	FTimerHandle FTHInit;
	void Init();

	FTimerHandle FTHDestruction;
	void Destruction();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
