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
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;

	SpawnPoint = CreateDefaultSubobject<USceneComponent>("Spawn Point");
	SpawnPoint->SetupAttachment(Root);
	IdlePoint = CreateDefaultSubobject<USceneComponent>("Idle Point");
	IdlePoint->SetupAttachment(Root);

#ifdef WITH_EDITOR 
	IdlePointSprite = CreateDefaultSubobject<UBillboardComponent>("Sprite");
	IdlePointSprite->SetupAttachment(IdlePoint);
	IdlePointSprite->SetWorldScale3D(FVector(0.5f));
#endif // WITHEDITOR
}

// Called when the game starts or when spawned
void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	m_SpawnLocation = SpawnPoint->GetComponentLocation();
	GameplayTags.AddTag(Tag::EnemySpawner());
	m_AuberginePack = MakeShared<EDogPack>();

	{
		SpawnData = MakeShared<SpawnPointData>();
		SpawnData->Location = GetActorLocation();
		SpawnData->Radius_Max = SpawnerActiveRadius;
		SpawnData->Radius_Min = SpawnerMinRadius;
		SpawnData->IdleLocation = IdlePoint->GetComponentLocation();
	}

	Async(EAsyncExecution::TaskGraphMainThread, [this]() { BeginActorSpawn(&AEnemySpawner::SpawnActor); });
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DrawDebugSphere(GetWorld(), GetActorLocation(), SpawnerActiveRadius, 30, FColor::Red, false, 0.f, 0, 5.f);
}

void AEnemySpawner::BeginActorSpawn(void(AEnemySpawner::* spawnFunction)())
{
	GetWorldTimerManager().SetTimer(TH_BeginSpawnAction, this, spawnFunction, Timer_BeginSpawnAction, false);
}

void AEnemySpawner::BeginActorRespawn()
{
	DrawDebugSphere(GetWorld(), GetActorLocation(), SpawnerActiveRadius, 30, FColor::Red, false, 1.f, 0, 5.f);
	DetermineActorsToRespawn(m_ActorsToRespawn);
	RespawnActors(m_ActorsToRespawn);
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
	static TFuture<void> asyncSpawn;
	if (TypeIndex >= 3) TypeIndex = 0;
	m_SpawnIndex++;
	asyncSpawn = Async(EAsyncExecution::TaskGraphMainThread, [this]() {
		SpawnAubergineDog(TypeIndex);
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
	m_ActorsToDestroy = actorsToRespawn;

	if (actorsToRespawn.Num() == 0) {
		GetWorldTimerManager().SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);
		return;
	}
	RespawnActor();
}

void AEnemySpawner::RespawnActor()
{
	int type = (int)m_ActorsToRespawn[m_ActorsToRespawn.Num() - 1]->m_DogType;
	m_ActorsToRespawn.RemoveAt(m_ActorsToRespawn.Num() - 1);

	SpawnAubergineDog(type);
	m_SpawnIndex++;
	if (m_SpawnIndex < m_SpawnAmount) {
		GetWorldTimerManager().SetTimer(TH_SpawnTimer, this, &AEnemySpawner::RespawnActor, Timer_SpawnIterator);
	}

	if (m_SpawnIndex >= m_SpawnAmount) {
		GetWorldTimerManager().SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);

		for (auto& it : m_ActorsToDestroy) {
			it->Destroy();
		}
		m_ActorsToDestroy.Empty();
	}
}

void AEnemySpawner::Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType)
{
	if (!CanBeAttacked()) return;
	bAICanBeDamaged = false;

	BeginActorSpawn(&AEnemySpawner::BeginActorRespawn);
}

void AEnemySpawner::SpawnAubergineDog(int& index)
{
	ASmallEnemy* spawnedDog = Cast<ASmallEnemy>(SpawnCharacter());
	if (!spawnedDog)
		return;
	spawnedDog->m_DogPack = m_AuberginePack;

	switch (index++)
	{
	case 0:
		PRINTLONG("Spawning : RED");
		spawnedDog->SetDogType(EDogType::Red);
		m_AuberginePack->Red = spawnedDog;
		break;
	case 1:
		PRINTLONG("Spawning : PINK");
		spawnedDog->SetDogType(EDogType::Pink);
		m_AuberginePack->Pink = spawnedDog;
		break;
	case 2:
		PRINTLONG("Spawning : TEAL");
		spawnedDog->SetDogType(EDogType::Teal);
		m_AuberginePack->Teal = spawnedDog;
		index = 0;
		break;
	default:
		break;
	}

	SpawnedActors.Add(spawnedDog);

	spawnedDog->SetSpawnPointData(SpawnData);

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

void EDogPack::AlertPack(ASmallEnemy* Instigator)
{
	if (Red != Instigator)
		Red->Alert(*Instigator);
	if (Pink != Instigator)
		Pink->Alert(*Instigator);
	if (Teal != Instigator)
		Teal->Alert(*Instigator);
}
