// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickupSpawner.h"
#include "Collectible.h"

APickupSpawner::APickupSpawner()
{
	PrimaryActorTick.bCanEverTick = true;
	HealthPickupPlacement = CreateDefaultSubobject<USceneComponent>("HealthPickupPlacement");
	HealthPickupPlacement->SetupAttachment(Root);
}

void APickupSpawner::BeginPlay()
{
	SpawnPickup();
}

void APickupSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APickupSpawner::RespawnPickup()
{
	//PRINTPARLONG(PickupSpawnTime, "Spawning new pickup in %f seconds", PickupSpawnTime);
	//TimerManager.SetTimer(TH_PickupRespawn, this, &APickupSpawner::SpawnPickup, PickupSpawnTime);
	GetWorldTimerManager().SetTimer(TH_PickupRespawn, this, &APickupSpawner::SpawnPickup, PickupSpawnTime);
}

void APickupSpawner::SpawnPickup()
{
	if (!this) return;
	if (!m_PickupClass) return;
	//PRINTLONG(PickupSpawnTime, "Spawn Pickup");
	FActorSpawnParameters Params;
	ACollectible* collectible = GetWorld()->SpawnActor<ACollectible>(m_PickupClass, HealthPickupPlacement->GetComponentTransform());
	collectible->bWasSpawnedByOwner = true;
	collectible->SpawningOwner = this;
}
