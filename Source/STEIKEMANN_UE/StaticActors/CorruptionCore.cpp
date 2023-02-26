// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/CorruptionCore.h"
#include "NiagaraFunctionLibrary.h"
#include "../StaticActors/GrappleTarget.h"

// Sets default values
ACorruptionCore::ACorruptionCore()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(Root);

}

// Called when the game starts or when spawned
void ACorruptionCore::BeginPlay()
{
	Super::BeginPlay();

	GTagContainer.AddTag(Tag::CorruptionCore());
}

// Called every frame
void ACorruptionCore::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACorruptionCore::Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType)
{
	if (!bAICanBeDamaged) { return; }
	switch (AType)
	{
	case EAttackType::SmackAttack:
		ReceiveDamage(1);
		break;
	default:
		break;
	}
	bAICanBeDamaged = false;
	GetWorldTimerManager().SetTimer(FTHCanBeDamaged, this, &ACorruptionCore::ResetCanbeDamaged, 0.5f);
}

void ACorruptionCore::ReceiveDamage(const int damage)
{
	//Health > 0 ? Health -= damage : Health = 0;
	Health = FMath::Clamp(Health -= damage, 0, 10);
	if (Health == 0) { Death(); }
}

void ACorruptionCore::Death()
{
	/* Particles and disable mesh + collision */
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathParticles, GetActorLocation());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetHiddenInGame(true, true);

	/* Spawn collectible */
	FVector Loc = GetActorLocation() + FVector(0,0,200);
	GetWorld()->SpawnActor<AActor>(SpawnedCollectible, Loc, FRotator());

	/* Destroy object after 10 seconds */
	auto func = [this]() {
		Destroy();
	};
	FTimerDelegate FDelegate;
	FDelegate.BindLambda(func);
	GetWorldTimerManager().SetTimer(FTHDestruction, FDelegate, 5.f, false);
}