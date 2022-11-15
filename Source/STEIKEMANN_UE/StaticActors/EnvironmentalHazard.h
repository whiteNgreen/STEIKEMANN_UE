// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Base/BaseStaticActor.h"

#include "EnvironmentalHazard.generated.h"

UCLASS()
class STEIKEMANN_UE_API AEnvironmentalHazard : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEnvironmentalHazard();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		UStaticMeshComponent* Mesh{ nullptr };
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		UBoxComponent* BoxCollider{ nullptr };
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision")
		USphereComponent* SphereCollider{ nullptr };



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
