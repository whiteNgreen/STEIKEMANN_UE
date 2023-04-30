// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerDistanceCheck_Component.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActorWithinDistance);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActorOutsideOfDistance);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STEIKEMANN_UE_API UPlayerDistanceCheck_Component : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPlayerDistanceCheck_Component();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable)
		FActorWithinDistance OnActorWithinDistance;
	UPROPERTY(BlueprintAssignable)
		FActorOutsideOfDistance OnActorOutsideOfDistance;

	UPROPERTY(BlueprintReadOnly)
		bool bPlayerWithinDistance{};

	UPROPERTY(BlueprintReadOnly)
		float DistanceToPlayer{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float CheckLength{ 5000 };

	UFUNCTION(BlueprintCallable)
		bool PlayerWithinDistanceCheck() const;

	UFUNCTION(BlueprintCallable)
		bool PlayerWithinDistance(const float distance) const;

	UFUNCTION(BlueprintCallable)
		float PlayerDistanceCheck() const;
};
