// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/CollectibleSpawner.h"
#include "Components/SplineComponent.h"
#include "STEIKEMANN_UE/STEIKEMANN_UE.h"	// Common includes

// Sets default values
ACollectibleSpawner::ACollectibleSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void ACollectibleSpawner::SpawnCollectibles()
{
	switch (Type)
	{
	case ECollectibleSpawnerType::BoxVolume:
		BoxSpawnCollectibles();
		break;
	case ECollectibleSpawnerType::Spline:
		SplineSpawnCollectibles();
		break;
	default:
		break;
	}
}

void ACollectibleSpawner::InitRandomSeed()
{
	srand(RandomSeed);
}

FVector ACollectibleSpawner::GetRandomBoxLocation(UBoxComponent* box)
{
	FVector actorloc{ GetActorLocation() };
	FVector extent{ box->GetScaledBoxExtent() };
	FVector zero = FVector(actorloc.X - extent.X, actorloc.Y - extent.Y, actorloc.Z);
	float Z = GetActorLocation().Z;

	FVector vec{ zero }; // zero is at the top left corner of the spawn box
	vec.X += (rand() % ((int)extent.X * 2) + 0);
	vec.Y += (rand() % ((int)extent.Y * 2) + 0);
	FVector tospawn = vec - actorloc;	// find vector from actor zero to spawnpoint
	tospawn = tospawn.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector);	// rotate tospawn vector
	vec = actorloc + tospawn;	// location is actor location + tospawn
	return vec;
}

void ACollectibleSpawner::SplineSpawnCollectibles()
{
	USplineComponent* spline = Cast<USplineComponent>(SpawnerComponent);
	if (!spline) { return; }

	int div = FMath::Clamp(SpawnAmount - 1, 1, 1000);
	UWorld* world = GetWorld();
	for (size_t i{}; i < SpawnAmount; i++)
	{
		float time = (float)i / ((float)div);
		FVector position = spline->GetLocationAtTime(time, ESplineCoordinateSpace::World);

		if (bPlaceOnGround) {
			bool b;
			position.Z = TraceZLocation(position, b, i);
			if (!b) { continue; }
		}
		world->SpawnActor<ACollectible>(SpawnedCollectible, position, FRotator());
	}
}

// Uniform eller random hadde vært fint 
void ACollectibleSpawner::BoxSpawnCollectibles()
{
	UBoxComponent* box = Cast<UBoxComponent>(SpawnerComponent);
	if (!box) { return; }
	UWorld* world = GetWorld();

	FVector actorloc{ GetActorLocation() };
	FVector extent{ box->GetScaledBoxExtent() };
	FVector zero = FVector(actorloc.X - extent.X, actorloc.Y - extent.Y, actorloc.Z);
	float Z = GetActorLocation().Z;

	srand(RandomSeed);
	/* Getting a random location within the box extent, taking its rotation into account */
	auto getloc = [&actorloc, &zero, &extent, this]() {
		FVector vec{ zero }; // zero is at the top left corner of the spawn box
		vec.X += (rand() % ((int)extent.X * 2) + 0);
		vec.Y += (rand() % ((int)extent.Y * 2) + 0);
		FVector tospawn = vec - actorloc;	// find vector from actor zero to spawnpoint
		tospawn = tospawn.RotateAngleAxis(GetActorRotation().Yaw, FVector::UpVector);	// rotate tospawn vector
		vec = actorloc + tospawn;	// location is actor location + tospawn
		return vec;
	};

	for (size_t i{}; i < SpawnAmount; i++)
	{
		FVector loc = getloc();

		/* Raytrace to the ground and place the collectible on top of it */
		if (bPlaceOnGround) {
			bool b;
			loc.Z = TraceZLocation(loc, b, i);
			if (!b) { continue; }
		}
		world->SpawnActor<ACollectible>(SpawnedCollectible, loc, FRotator(0));
	}
}



float ACollectibleSpawner::TraceZLocation(FVector& loc, bool& traceSuccess, int index)
{
	FHitResult Hit;
	FCollisionQueryParams params("", false, this);
	traceSuccess = GetWorld()->LineTraceSingleByChannel(Hit, loc, loc - FVector(0, 0, 3000), ECC_Visibility, params);
	if (!traceSuccess) {
		UE_LOG(LogTemp, Warning, TEXT("%s failed spawning of collectible %i, no ground below"), *GetName(), index);
		//continue;
	}
	return Hit.ImpactPoint.Z + ZGroundPlacement;
}

// Called when the game starts or when spawned
void ACollectibleSpawner::BeginPlay()
{
	Super::BeginPlay();
	//SpawnCollectibles();
}

// Called every frame
void ACollectibleSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

