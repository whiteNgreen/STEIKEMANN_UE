// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/Collectible.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/SkeletalmeshComponent.h"

// Sets default values
ACollectible::ACollectible()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	Mesh->SetupAttachment(Root);
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(Root);
}


// Called when the game starts or when spawned
void ACollectible::BeginPlay()
{
	Super::BeginPlay();
	
	GTagContainer.AddTag(Tag::Collectible());
}

// Called every frame
void ACollectible::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (bShouldRotate) {
		RotationSpeed_Internal = FMath::FInterpTo(RotationSpeed_Internal, RotationSpeed, DeltaTime, RotationSpeed_LerpSpeed);
		AddActorLocalRotation(FRotator(0.0, RotationSpeed_Internal * DeltaTime, 0.0));
	}
	else {
		RotationSpeed_Internal = FMath::FInterpTo(RotationSpeed_Internal, 0.f, DeltaTime, RotationSpeed_LerpSpeed);
	}
}

void ACollectible::OnCollectibleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
}

void ACollectible::Destruction()
{
	Destruction_IMPL();
	/* Particles and disable mesh + collision */
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathParticles, GetActorLocation());	// SPAWN PARTICLE IN BLUEPRINT TO AVOID NIAGARA INCLUDE
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (Mesh)
		Mesh->SetHiddenInGame(true, true);

	/* Destroy object after 2.f seconds */
	GetWorldTimerManager().SetTimer(FTHDestruction, [this]() { Destroy(); }, 2.f, false);
}

/// <summary>
/// **************************** ACollectible_Static ***********************************
/// </summary>
ACollectible_Static::ACollectible_Static()
{
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(Root);
}
void ACollectible_Static::BeginPlay()
{
	Super::BeginPlay();
	GTagContainer.AddTag(Tag::Collectible());
}
void ACollectible_Static::Destruction()
{
	Destruction_IMPL();
	/* Particles and disable mesh + collision */
	//UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathParticles, GetActorLocation());	// SPAWN PARTICLE IN BLUEPRINT TO AVOID NIAGARA INCLUDE
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	/* Destroy object after 2.f seconds */
	GetWorldTimerManager().SetTimer(FTHDestruction, [this]() { Destroy(); }, 2.f, false);
}