// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Base/BaseStaticActor.h"
#include "EnS/StaticActors_EnS.h"

#include "Collectible.generated.h"



UCLASS()
class STEIKEMANN_UE_API ACollectible : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACollectible();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mesh")
		USkeletalMeshComponent* Mesh { nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class USphereComponent* Sphere{ nullptr };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		ECollectibleType CollectibleType;

	UFUNCTION()
		void OnCollectibleBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particles")
		class UNiagaraSystem* DeathParticles{ nullptr };


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables")
		bool bShouldRotate{ true };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables")
		float RotationSpeed{ 100.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables")
		float RotationSpeed_LerpSpeed{ 2.f };
	float RotationSpeed_Internal{};


	bool bWasSpawnedByOwner{};
	AActor* SpawningOwner{ nullptr };
	FTimerHandle FTHDestruction;
	UFUNCTION(BlueprintImplementableEvent)
		void Destruction_IMPL();
	void Destruction();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};

UCLASS()
class STEIKEMANN_UE_API ACollectible_Static : public ABaseStaticActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ACollectible_Static();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class USphereComponent* Sphere{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Type")
		ECollectibleType CollectibleType;

	bool bWasSpawnedByOwner{};
	AActor* SpawningOwner{ nullptr };

	FTimerHandle FTHDestruction;
	UFUNCTION(BlueprintImplementableEvent)
		void Destruction_IMPL();
	void Destruction();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
};

UCLASS()
class STEIKEMANN_UE_API ANewspaper : public ACollectible_Static
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Type")
		int NewspaperIndex{ 0 };
};