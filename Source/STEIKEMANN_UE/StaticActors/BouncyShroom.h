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
		float BounceStrength{ 2500.f };
	//UPROPERTY(EditAnywhere, BlueprintReadOnly)
		//float BounceMultiplier{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		bool bReflectDirection{ true };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bReflectDirection", EditConditionHides))
		float ReflectionStrength{ 1.f };
public:
	ABouncyShroom();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	FVector ShroomDirection;
	FVector ShroomLocation;
public:
	bool GetBounceInfo(const FVector actorLocation, const FVector normalImpulse, const FVector IncommingDirection, FVector& OUT_direction, float& OUT_strength);

	UFUNCTION(BlueprintCallable)
		void DrawProjectedBouncePath(float time, float drawtime, float GravityMulti);
};
