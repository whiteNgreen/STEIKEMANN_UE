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
	Super::BeginPlay();
	SpawnPickup();
}

void APickupSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void APickupSpawner::RespawnPickup()
{
	PRINTLONG(PickupSpawnTime, "Respawn Pickup");
	TimerManager.SetTimer(TH_PickupRespawn, this, &APickupSpawner::SpawnPickup, PickupSpawnTime);
}

void APickupSpawner::SpawnPickup()
{
	if (!this) return;
	if (!m_PickupClass) return;
	FActorSpawnParameters Params;
	ACollectible_Static* collectible = GetWorld()->SpawnActor<ACollectible_Static>(m_PickupClass, HealthPickupPlacement->GetComponentTransform());
	collectible->bWasSpawnedByOwner = true;
	collectible->SpawningOwner = this;
}
