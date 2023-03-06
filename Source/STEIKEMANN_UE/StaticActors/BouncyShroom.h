// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "Components/ArrowComponent.h"
#include "BouncyShroom.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API ABouncyShroom : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UArrowComponent* DirectionArrowComp;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UBoxComponent* CollisionBox;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		USkeletalMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float BounceMultiplier{ 1.f };
public:
	ABouncyShroom();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	FVector ShroomDirection;
public:
	bool GetBounceInfo(const FVector normalImpulse, FVector& OUT_direction, float& OUT_strength);
};
