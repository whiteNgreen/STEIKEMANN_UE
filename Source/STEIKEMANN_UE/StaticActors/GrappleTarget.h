// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "GameplayTagAssetInterface.h"

#include "GrappleTarget.generated.h"

UCLASS()
class STEIKEMANN_UE_API AGrappleTarget : public AActor,
	public IGrappleTargetInterface,
	public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGrappleTarget();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FGameplayTagContainer TagsContainer;

	//FGameplayTag GrappleType;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/* --------- Gameplay Tag Interface ------------ */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = TagsContainer; return; }


	/* --------- GrappleTarget Interface --------- */
	//virtual FGameplayTag GetGrappledGameplayTag_Pure() const override { return GrappleTargetType; }

	virtual void TargetedPure() override;

	virtual void UnTargetedPure() override;


	virtual void InReach_Pure() override;

	virtual void OutofReach_Pure() override;


	virtual void HookedPure() override;
	virtual void HookedPure(const FVector InstigatorLocation, bool PreAction = false) override{}

	virtual void UnHookedPure() override;
};
