// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"
#include "../Enemies/SmallEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "GameframeWork/CharacterMovementComponent.h"

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

	m_SpawnLocation = SpawnPoint->GetComponentLocation();
	//CheckSpawningActorClass();

	FTimerHandle h;
	GetWorldTimerManager().SetTimer(h, this, &AEnemySpawner::SpawnActor, 1.f, false);

	GameplayTags.AddTag(Tag::EnemySpawner());
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemySpawner::CheckSpawningActorClass()
{
	if (SpawningActorType->IsChildOf(ASmallEnemy::StaticClass()))
	{
		m_EENemySpawnType = EEnemySpawnType::AubergineDog;
		PRINTLONG("Spawning AubergineDoggo Enemy");
		return;
	}
	else if (SpawningActorType->IsChildOf(ACharacter::StaticClass()))
	{
		m_EENemySpawnType = EEnemySpawnType::Character;
		PRINTLONG("Spawning AubergineDoggo Enemy");
		return;
	}
	else
	{
		m_EENemySpawnType = EEnemySpawnType::Other;
	}
}

void AEnemySpawner::SpawnActor()
{
	//switch (m_EENemySpawnType)
	//{
	//case EEnemySpawnType::AubergineDog:
		SpawnAubergineDog();
	//	break;
	//case EEnemySpawnType::Character:
	//	SpawnCharacter();
	//	break;
	//case EEnemySpawnType::Other:
	//	break;
	//default:
	//	break;
	//}
	m_SpawnIndex++;
	if (m_SpawnIndex < m_SpawnAmount)
		GetWorldTimerManager().SetTimer(TH_SpawnTimer, this, &AEnemySpawner::SpawnActor, m_SpawnTimer);
}

float AEnemySpawner::RandomFloat(float min, float max)
{
	return rand() * 1.0 / RAND_MAX * (max - min + 1) + min;
}

void AEnemySpawner::DetermineActorsToRespawn(TArray<ASmallEnemy*>& actorsToRespawn)
{
	for (auto& it : SpawnedActors)
	{
		float radius = FVector(it->GetActorLocation() - GetActorLocation()).Length();
		if (radius < SpawnerActiveRadius) continue;
		actorsToRespawn.Add(it);
		DrawDebugLine(GetWorld(), it->GetActorLocation(), GetActorLocation(), FColor::Blue, false, 1.f, 0, 5.f);
	}
	for (auto& it : actorsToRespawn)
		SpawnedActors.Remove(it);
}

void AEnemySpawner::RespawnActors(TArray<ASmallEnemy*>& actorsToRespawn)
{
	m_SpawnIndex = 0;
	m_SpawnAmount = actorsToRespawn.Num();

	for (auto& it : actorsToRespawn)
		it->Destroy();

	if (actorsToRespawn.Num() == 0) {
		GetWorldTimerManager().SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);
		return;
	}
	RespawnActor();
}

void AEnemySpawner::RespawnActor()
{
	SpawnAubergineDog();
	m_SpawnIndex++;
	if (m_SpawnIndex < m_SpawnAmount)
		GetWorldTimerManager().SetTimer(TH_SpawnTimer, this, &AEnemySpawner::RespawnActor, m_SpawnTimer);
	if (m_SpawnIndex >= m_SpawnAmount)
		GetWorldTimerManager().SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);
}

void AEnemySpawner::Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType)
{
	if (!CanBeAttacked()) return;
	bAICanBeDamaged = false;

	DrawDebugSphere(GetWorld(), GetActorLocation(), SpawnerActiveRadius, 30, FColor::Red, false, 1.f, 0, 5.f);
	TArray<ASmallEnemy*> actorsToRespawn;
	DetermineActorsToRespawn(actorsToRespawn);
	RespawnActors(actorsToRespawn);
}

void AEnemySpawner::SpawnAubergineDog()
{
	ASmallEnemy* spawnedDog = (ASmallEnemy*)SpawnCharacter();
	SpawnedActors.Add(spawnedDog);

	// Initiate dog specific animations
}

ACharacter* AEnemySpawner::SpawnCharacter()
{
	float yaw = RandomFloat(0, 360.f);
	float pitch = RandomFloat(PitchRange.X, PitchRange.Y);
	FRotator rot(0.f, yaw, 0.f);

	ASmallEnemy* spawnedCharacter = GetWorld()->SpawnActor<ASmallEnemy>(SpawningActorType, SpawnPoint->GetComponentLocation(), rot);
	
	// Launch character out
	FVector LaunchDirection;
	LaunchDirection = FVector::ForwardVector.RotateAngleAxis(yaw, FVector::UpVector);
	DrawDebugLine(GetWorld(), spawnedCharacter->GetActorLocation(), spawnedCharacter->GetActorLocation() + (LaunchDirection * Spawn_LaunchStrength), FColor::Blue, false, 3.f, 0, 6.f);
	LaunchDirection = (cosf(FMath::DegreesToRadians(pitch)) * LaunchDirection) + (sinf(FMath::DegreesToRadians(pitch)) * FVector::UpVector);

	spawnedCharacter->GetCharacterMovement()->bJustTeleported = false;
	spawnedCharacter->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	spawnedCharacter->GetCharacterMovement()->AddImpulse(LaunchDirection.GetSafeNormal() * Spawn_LaunchStrength, true);
	DrawDebugLine(GetWorld(), spawnedCharacter->GetActorLocation(), spawnedCharacter->GetActorLocation() + (LaunchDirection * Spawn_LaunchStrength), FColor::Red, false, 3.f, 0, 6.f);
	
	return spawnedCharacter;
}


