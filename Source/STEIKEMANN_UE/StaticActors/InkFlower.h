// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "../Interfaces/AttackInterface.h"
#include "InkFlower.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API AInkFlower : public ABaseStaticActor,
	public IAttackInterface
{
	GENERATED_BODY()
	
public:
	AInkFlower();
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USphereComponent* SphereCollider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USphereComponent* CollectibleCollider;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USkeletalMeshComponent* Mesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		USkeletalMeshComponent* BeakMesh;

public: // Animation
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Open();

public: // Collectible
	UFUNCTION(BlueprintCallable)
		void SpawnInkCollectible();
	//UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep, const FHitResult&, SweepResult);

	UFUNCTION()
		void OnCollectibleBeginOverlap(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit);
	void PlayerCollectInk(AActor* player);
	UFUNCTION(BlueprintImplementableEvent)
		void Collected();

public:// Attack Interface
	virtual bool CanBeAttacked() override { return bAICanBeDamaged; }
	virtual void Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType);

};
