// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Base/BaseStaticActor.h"
#include "../Interfaces/AttackInterface.h"
#include "CorruptionTendril.h"


#include "CorruptionCore.generated.h"

class UCapsuleComponent;
class UTimelineComponent;

UCLASS()
class ACorruptionCore : public ABaseStaticActor,
	public IAttackInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACorruptionCore();

public: // Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		USkeletalMeshComponent* Mesh{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UCapsuleComponent* Collider{ nullptr };

public: // Timeline --  For the more advanced smacked animation method
	//UTimelineComponent* TLComp_Smacked;
	//UPROPERTY(EditAnywhere)
		//UCurveFloat* Curve_SmackAnim;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Variables", meta = (UIMin = "0", UIMAX = "10"))
		int Health{ 4 };

	virtual bool CanBeAttacked() override { return true; }
	virtual void Gen_ReceiveAttack(const FVector Direction, const float Strength, const EAttackType AType, const float Delaytime = -1.f) override;

	void ReceiveDamage(int damage);
	UFUNCTION(BlueprintImplementableEvent)
		void HealthUpdate(int newhealth);
	void Death();
	UFUNCTION(BlueprintImplementableEvent)
		void Death_IMPL();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<ACorruptionTendril*> ConnectedTendrils;
	
	UFUNCTION(BlueprintCallable)
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
