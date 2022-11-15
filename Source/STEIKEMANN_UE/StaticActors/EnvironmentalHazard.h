// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../STEIKEMANN_UE.h"

#include "EnvironmentalHazard.generated.h"

UCLASS()
class STEIKEMANN_UE_API AEnvironmentalHazard : public AActor,
	public IGameplayTagAssetInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnvironmentalHazard();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Root")
		USceneComponent* Root{ nullptr };
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mesh")
		UStaticMeshComponent* Mesh{ nullptr };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		UBoxComponent* BoxCollider{ nullptr };
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		USphereComponent* SphereCollider{ nullptr };

	FGameplayTagContainer GTagContainer;
	void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GTagContainer; }


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
