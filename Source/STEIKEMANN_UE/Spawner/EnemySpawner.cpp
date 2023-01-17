// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"
#include "../Enemies/SmallEnemy.h"

// Sets default values
AEnemySpawner::AEnemySpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>("Spawn Point");
	SpawnPoint->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (SpawningActor == ASmallEnemy::StaticClass())
		PRINTLONG("Spawning AubergineDoggo Enemy");
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemySpawner::SpawnActor()
{
}

float AEnemySpawner::RandomFloat(float min, float max)
{
	return rand() * 1.f / RAND_MAX * (max - min + 1) - min;
}

