// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "Base/BaseStaticActor.h"

#include "GrappleTarget.generated.h"

UCLASS()
class STEIKEMANN_UE_API AGrappleTarget : public ABaseStaticActor,
	public IGrappleTargetInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGrappleTarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

/* Grapple Target Interface */
public:
	virtual void TargetedPure() override;
	virtual void UnTargetedPure() override;

	virtual void InReach_Pure() override;
	virtual void OutofReach_Pure() override;

	virtual void HookedPure() override;
	virtual void HookedPure(const FVector InstigatorLocation, bool PreAction = false) override{}

	virtual void UnHookedPure() override;
};
