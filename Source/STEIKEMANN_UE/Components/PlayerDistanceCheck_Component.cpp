// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerDistanceCheck_Component.h"
#include "../WorldStatics/SteikeWorldStatics.h"
#include "../DebugMacros.h"

// Sets default values for this component's properties
UPlayerDistanceCheck_Component::UPlayerDistanceCheck_Component()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPlayerDistanceCheck_Component::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UPlayerDistanceCheck_Component::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DistanceToPlayer = PlayerDistanceCheck();
	const bool b = DistanceToPlayer < FMath::Square(CheckLength);
	if (bPlayerWithinDistance != b)
	{
		bPlayerWithinDistance = b;
		if (b) {
			OnActorWithinDistance.Broadcast();
		}
		if (!b) {
			OnActorOutsideOfDistance.Broadcast();
		}
	}
}

bool UPlayerDistanceCheck_Component::PlayerWithinDistanceCheck() const
{
	return PlayerDistanceCheck() < FMath::Square(CheckLength);
}

bool UPlayerDistanceCheck_Component::PlayerWithinDistance(const float distance) const
{
	return PlayerDistanceCheck() < FMath::Square(distance);
}

float UPlayerDistanceCheck_Component::PlayerDistanceCheck() const
{
	return FVector::DistSquared(GetOwner()->GetActorLocation(), SteikeWorldStatics::CameraLocation);

}

