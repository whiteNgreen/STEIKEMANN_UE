// Fill out your copyright notice in the Description page of Project Settings.

#include "../StaticActors/PlayerRespawn.h"

APlayerRespawn::APlayerRespawn()
{
	//Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	//Mesh->SetupAttachment(Root);
	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(Root);
	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollider"));
	SphereCollider->SetupAttachment(Root);

	SpawnPoint = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnPoint"));
	SpawnPoint->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void APlayerRespawn::BeginPlay()
{
	Super::BeginPlay();

	GTagContainer.AddTag(Tag::PlayerRespawn());

	/* Resets colliders, to make sure they capture actors that start within 
	 *  their bounds at BeginPlay */
	FTimerHandle handle;
	GetWorldTimerManager().SetTimer(handle,
		[this](){
			FVector extent = BoxCollider->GetUnscaledBoxExtent();
			BoxCollider->SetBoxExtent(FVector(0), true);
			BoxCollider->SetBoxExtent(extent, true);

			float radius = SphereCollider->GetUnscaledSphereRadius();
			SphereCollider->SetSphereRadius(0, true);
			SphereCollider->SetSphereRadius(radius, true);
		},
		0.5f, false);
}

// Called every frame
void APlayerRespawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FTransform APlayerRespawn::GetSpawnTransform()
{
	return FTransform(SpawnPoint->GetComponentRotation(), SpawnPoint->GetComponentLocation());
}
