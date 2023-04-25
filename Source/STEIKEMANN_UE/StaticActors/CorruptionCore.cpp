// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/CorruptionCore.h"
//#include "NiagaraFunctionLibrary.h"
#include "../StaticActors/GrappleTarget.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ACorruptionCore::ACorruptionCore()
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Collider = CreateDefaultSubobject<UCapsuleComponent>("Collider");
	Collider->SetupAttachment(Mesh);
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

void ACorruptionCore::Gen_ReceiveAttack(const FVector Direction, const float Strength, const EAttackType AType, const float Delaytime)
{
	if (Delaytime > 0.f) {
		if (TimerManager.IsTimerActive(TH_Gen_ReceiveAttackDelay)) 
			return;
		TimerManager.SetTimer(TH_Gen_ReceiveAttackDelay, [this, Direction, Strength, AType]() { Gen_ReceiveAttack(Direction, Strength, AType); }, Delaytime, false);
		return;
	}
	if (!bAICanBeDamaged) { return; }
	switch (AType)
	{
	case EAttackType::SmackAttack:
		ReceiveDamage(1);
		break;
	case EAttackType::Environmental:
		ReceiveDamage(2);
		break;
	default:
		break;
	}
	bAICanBeDamaged = false;
	TimerManager.SetTimer(FTHCanBeDamaged, this, &ACorruptionCore::ResetCanbeDamaged, 0.1f);
	Execute_Gen_ReceiveAttack_IMPL(this, Direction, Strength, AType);
}

void ACorruptionCore::ReceiveDamage(const int damage)
{
	Health = FMath::Clamp(Health -= damage, 0, 10);
	HealthUpdate(Health);
	if (Health == 0) { Death(); }
}

void ACorruptionCore::Death()
{
	Death_IMPL();
	/* Particles and disable mesh + collision */
	//UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathParticles, GetActorLocation());
	Collider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	//Mesh->SetHiddenInGame(true, true);

	//DestroyConnectedTendrils();

	/* Destroy object after 10 seconds */
	auto func = [this]() {
		Destroy();
	};
	FTimerDelegate FDelegate;
	FDelegate.BindLambda(func);
	TimerManager.SetTimer(FTHDestruction, FDelegate, 5.f, false);
}

void ACorruptionCore::DestroyConnectedTendrils()
{
	FVector Loc = GetActorLocation();
	for (auto& it : ConnectedTendrils) {
		if (it)
			it->DestroyTendril_Start(Loc);
	}
}
