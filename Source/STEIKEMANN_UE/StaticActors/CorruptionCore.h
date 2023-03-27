// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Base/BaseStaticActor.h"
#include "../Interfaces/AttackInterface.h"
#include "CorruptionTendril.h"


#include "CorruptionCore.generated.h"

class UCapsuleComponent;

UCLASS()
class ACorruptionCore : public ABaseStaticActor,
	public IAttackInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACorruptionCore();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		USkeletalMeshComponent* Mesh{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UCapsuleComponent* Collider{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class USphereComponent* Sphere{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables", meta = (UIMin = "0", UIMAX = "10"))
		int Health{ 4 };

	virtual bool CanBeAttacked() override { return true; }
	virtual void Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType) override;

	void ReceiveDamage(int damage);
	void Death();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<ACorruptionTendril*> ConnectedTendrils;
	void DestroyConnectedTendrils();

	UPROPERTY(EditAnywhere)
		TSubclassOf<AActor> SpawnedCollectible;

	FGameplayTagContainer GTagContainer;
	void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GTagContainer; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Particles")
		class UNiagaraSystem* DeathParticles{ nullptr };

	FTimerHandle FTHDestruction;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
