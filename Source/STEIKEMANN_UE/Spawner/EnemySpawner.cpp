// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawner.h"

#include "../CommonFunctions.h"

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
	GameplayTags.AddTag(Tag::EnemySpawner());

	Async(EAsyncExecution::TaskGraphMainThread, [this]() { BeginActorSpawn(&AEnemySpawner::SpawnActor); });
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AEnemySpawner::BeginActorSpawn(void(AEnemySpawner::* spawnFunction)())
{
	GetWorldTimerManager().SetTimer(TH_BeginSpawnAction, this, spawnFunction, Timer_BeginSpawnAction, false);
}

void AEnemySpawner::BeginActorRespawn()
{
	DrawDebugSphere(GetWorld(), GetActorLocation(), SpawnerActiveRadius, 30, FColor::Red, false, 1.f, 0, 5.f);
	TArray<ASmallEnemy*> actorsToRespawn;
	DetermineActorsToRespawn(actorsToRespawn);
	RespawnActors(actorsToRespawn);
}

void AEnemySpawner::CheckSpawningActorClass()
{
	if (SpawningActorType->IsChildOf(ASmallEnemy::StaticClass()))
	{
		m_EENemySpawnType = EEnemySpawnType::AubergineDog;
		return;
	}
	else if (SpawningActorType->IsChildOf(ACharacter::StaticClass()))
	{
		m_EENemySpawnType = EEnemySpawnType::Character;
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
	//	break;
	//case EEnemySpawnType::Character:
	//	SpawnCharacter();
	//	break;
	//case EEnemySpawnType::Other:
	//	break;
	//default:
	//	break;
	//}
	static TFuture<void> asyncSpawn;
	//if (asyncSpawn.WaitFor(FTimespan::FromSeconds(0.2))) {
	//	UE_LOG(LogTemp, Error, TEXT("%s, Spawner failed Async WaitFor"), *GetName());
	//	return;
	//}
	
	m_SpawnIndex++;
	asyncSpawn = Async(EAsyncExecution::TaskGraphMainThread, [this]() {
		SpawnAubergineDog();
		if (m_SpawnIndex < m_SpawnAmount)
			GetWorldTimerManager().SetTimer(TH_SpawnTimer, this, &AEnemySpawner::SpawnActor, Timer_SpawnIterator);
		});
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
		GetWorldTimerManager().SetTimer(TH_SpawnTimer, this, &AEnemySpawner::RespawnActor, Timer_SpawnIterator);
	if (m_SpawnIndex >= m_SpawnAmount)
		GetWorldTimerManager().SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);
}

void AEnemySpawner::Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType)
{
	if (!CanBeAttacked()) return;
	bAICanBeDamaged = false;

	BeginActorSpawn(&AEnemySpawner::BeginActorRespawn);
}

void AEnemySpawner::SpawnAubergineDog()
{
	ASmallEnemy* spawnedDog = (ASmallEnemy*)SpawnCharacter();
	if (!spawnedDog)
		return;
	SpawnedActors.Add(spawnedDog);

	SpawnPointData data;
	data.Location = GetActorLocation();
	data.Radius_Max = SpawnerActiveRadius;
	data.Radius_Min = SpawnerMinRadius;
	spawnedDog->m_SpawnPointData = data;

	// Initiate dog specific animations
		// Should be called at ASmallEnemy::BeginPlay() or ASmallEnemy::Respawn() ??
}

ACharacter* AEnemySpawner::SpawnCharacter()
{
	float yaw = RandomFloat(0, 360.f);
	float pitch = RandomFloat(PitchRange.X, PitchRange.Y);
	FRotator rot(0.f, yaw, 0.f);

	ASmallEnemy* spawnedCharacter = GetWorld()->SpawnActor<ASmallEnemy>(SpawningActorType, SpawnPoint->GetComponentLocation(), rot);
	if (!spawnedCharacter)
		return nullptr;
	// Launch character out
	FVector LaunchDirection;
	LaunchDirection = FVector::ForwardVector.RotateAngleAxis(yaw, FVector::UpVector);
	LaunchDirection = (cosf(FMath::DegreesToRadians(pitch)) * LaunchDirection) + (sinf(FMath::DegreesToRadians(pitch)) * FVector::UpVector);

	spawnedCharacter->GetCharacterMovement()->bJustTeleported = false;
	spawnedCharacter->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	spawnedCharacter->GetCharacterMovement()->AddImpulse(LaunchDirection.GetSafeNormal() * Spawn_LaunchStrength, true);
	
	return spawnedCharacter;
}


