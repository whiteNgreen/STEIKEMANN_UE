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
		//SpawnData->Radius_Min = SpawnerMinRadius;
		SpawnData->IdleLocation = IdlePoint->GetComponentLocation();
		SpawnData->GuardRadius = SpawnerGuardRadius;
	}

	Async(EAsyncExecution::TaskGraphMainThread, [this]() {
		if (GetWorld())
		BeginActorSpawn(&AEnemySpawner::SpawnActor);
		});
}
void AEnemySpawner::BeginDestroy()
{
	Super::BeginDestroy();
	for (auto& dog : SpawnedActors)
		dog->Destroy();
}

// Called every frame
void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimerManager.Tick(DeltaTime);
}

void AEnemySpawner::BeginActorSpawn(void(AEnemySpawner::* spawnFunction)())
{
	TimerManager.SetTimer(TH_BeginSpawnAction, this, spawnFunction, Timer_BeginSpawnAction, false);
}

void AEnemySpawner::BeginActorRespawn()
{
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
	if (!GetWorld()) return;
	static TFuture<void> asyncSpawn;
	if (TypeIndex >= 3) TypeIndex = 0;
	m_SpawnIndex++;
	asyncSpawn = Async(EAsyncExecution::TaskGraphMainThread, [this]() {
		SpawnAubergineDog(TypeIndex);
		if (m_SpawnIndex < m_SpawnAmount)
			TimerManager.SetTimer(TH_SpawnTimer, this, &AEnemySpawner::SpawnActor, Timer_SpawnIterator);
		});
}


void AEnemySpawner::DetermineActorsToRespawn(TArray<ASmallEnemy*>& actorsToRespawn)
{
	for (auto& it : SpawnedActors)
	{
		if (!it) 
			continue;
		float radius = FVector(it->GetActorLocation() - GetActorLocation()).Length();
		if (radius < SpawnerActiveRadius) continue;
		actorsToRespawn.Add(it);
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
		TimerManager.SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);
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
		TimerManager.SetTimer(TH_SpawnTimer, this, &AEnemySpawner::RespawnActor, Timer_SpawnIterator);
	}

	if (m_SpawnIndex >= m_SpawnAmount) {
		TimerManager.SetTimer(FTHCanBeDamaged, [this]() { bAICanBeDamaged = true; }, CanBeAttackedTimer, false);

		for (auto& it : m_ActorsToDestroy) {
			it->Destroy();
		}
		m_ActorsToDestroy.Empty();
	}
}

void AEnemySpawner::Gen_ReceiveAttack(const FVector Direction, const float Strength, const EAttackType AType, const float Delaytime)
{
	if (Delaytime > 0.f) {
		if (TimerManager.IsTimerActive(TH_Gen_ReceiveAttackDelay))
			return;
		TimerManager.SetTimer(TH_Gen_ReceiveAttackDelay, [this, Direction, Strength, AType]() { Gen_ReceiveAttack(Direction, Strength, AType); }, Delaytime, false);
		return;
	}
	if (!CanBeAttacked()) return;
	bAICanBeDamaged = false;

	BeginActorSpawn(&AEnemySpawner::BeginActorRespawn);
	Execute_Gen_ReceiveAttack_IMPL(this, Direction, Strength, AType);
	Anim_Hit();
}

void AEnemySpawner::SpawnAubergineDog(int& index)
{
	ASmallEnemy* spawnedDog = SpawnCharacter();
	if (!spawnedDog)
		return;
	spawnedDog->m_DogPack = m_AuberginePack;

	switch (index++)
	{
	case 0:
		spawnedDog->SetDogType(EDogType::Red);
		m_AuberginePack->Red = spawnedDog;
		break;
	case 1:
		spawnedDog->SetDogType(EDogType::Pink);
		m_AuberginePack->Pink = spawnedDog;
		break;
	case 2:
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

ASmallEnemy* AEnemySpawner::SpawnCharacter()
{
	if (!GetWorld()) return nullptr;

	float yaw = RandomFloat(-90.f, 90.f);
	float pitch = RandomFloat(PitchRange.X, PitchRange.Y);
	FRotator rot(0.f, yaw, 0.f);
	FVector LaunchDirection;
	LaunchDirection = GetActorForwardVector().RotateAngleAxis(yaw, FVector::UpVector);
	LaunchDirection = (cosf(FMath::DegreesToRadians(pitch)) * LaunchDirection) + (sinf(FMath::DegreesToRadians(pitch)) * FVector::UpVector);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ASmallEnemy* spawnedDog = GetWorld()->SpawnActor<ASmallEnemy>(SpawningActorType, SpawnPoint->GetComponentLocation() + (LaunchDirection * 100.f), rot, Params);
	if (!spawnedDog)
		return nullptr;

	spawnedDog->Launched(LaunchDirection);
	spawnedDog->GetCharacterMovement()->bJustTeleported = false;
	spawnedDog->GetCharacterMovement()->bRunPhysicsWithNoController = true;
	spawnedDog->GetCharacterMovement()->AddImpulse(LaunchDirection.GetSafeNormal() * Spawn_LaunchStrength, true);
	
	return spawnedDog;
}

void EDogPack::AlertPack(ASmallEnemy* Instigator)
{
	if (!Instigator) return;
	if (Red)
		if (Red != Instigator)
			Red->Alert(*Instigator);
	if (Pink)
		if (Pink != Instigator)
			Pink->Alert(*Instigator);
	if (Teal)
		if (Teal != Instigator)
			Teal->Alert(*Instigator);
}
