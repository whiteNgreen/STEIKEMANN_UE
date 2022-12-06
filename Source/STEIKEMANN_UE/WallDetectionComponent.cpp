// Fill out your copyright notice in the Description page of Project Settings.


#include "WallDetectionComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UWallDetectionComponent::UWallDetectionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UWallDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	capsule = FCollisionShape::MakeCapsule(Capsule_Radius, Capsule_HalfHeight);
}


// Called every frame
void UWallDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

bool UWallDetectionComponent::DetectWall(const AActor* actor, const FVector Location, const FVector ForwardVector, WallData& data)
{
	if (!actor) return false;
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params = FCollisionQueryParams("", false, actor);
	bool b = GetWorld()->SweepMultiByChannel(Hits, Location, Location + ForwardVector * Capsule_Radius, FQuat(1.f, 0, 0, 0), ECC_WallDetection, capsule, Params);
		DrawDebugCapsule(GetWorld(), Location, capsule.GetCapsuleHalfHeight(), capsule.GetCapsuleRadius(), FQuat(1.f, 0, 0, 0), FColor(.3f, .3f, 1.f, 1.f), false, 0, 0, 1.f);
	if (!b) 
		return false;
	// Debug
		for (const auto& it : Hits)
			DrawDebugPoint(GetWorld(), it.ImpactPoint, 5.f, FColor::Blue, false, 0, 1);

	b = DetermineValidPoints_IMPL(Hits);
	if (!b)
		return false;

	GetWallPoint_IMPL(data, Hits);


	DrawDebugPoint(GetWorld(), data.Location, 12.f, FColor::Red, false, 0, 1);
	DrawDebugLine(GetWorld(), data.Location, data.Location + data.Normal * 50.f, FColor::Purple, false, 0, 1, 4.f);


	return true;
}

bool UWallDetectionComponent::DetermineValidPoints_IMPL(TArray<FHitResult>& hits)
{
	for (int32 i{}; i < hits.Num();)
	{
		float angle = hits[i].ImpactNormal.Z;
		if (angle > Angle_UpperLimit || angle < Angle_LowerLimit) {
			hits.RemoveAt(i);
			continue;
		}
		i++;
	}
	if (hits.Num() == 0) return false;
	return true;
}

void UWallDetectionComponent::GetWallPoint_IMPL(WallData& data, const TArray<FHitResult>& hits)
{
	TArray<FVector> locations;
	TArray<FVector> normals;
	for (const auto& it : hits) 
	{
		locations.Add(it.ImpactPoint);
		normals.Add(it.ImpactNormal);
	}

	FVector loc;
	for (const auto& it : locations)
		loc += it;
	loc /= (float)locations.Num();
	data.Location = loc;

	FVector n = normals[0];
	if (normals.Num() > 1) {
		for (int32 i{ 1 }; i < normals.Num(); i++)
			n += normals[i];
		n.Normalize();
	}
	data.Normal = n;
}

