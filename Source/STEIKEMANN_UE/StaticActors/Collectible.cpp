// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/Collectible.h"
#include "NiagaraFunctionLibrary.h"
//#include "../Steikemann/SteikemannCharacter.h"

// Sets default values
ACollectible::ACollectible()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	Sphere->SetupAttachment(Root);
}


// Called when the game starts or when spawned
void ACollectible::BeginPlay()
{
	Super::BeginPlay();
	
	GTagContainer.AddTag(Tag::Collectible());
	//GetWorldTimerManager().SetTimer(FTHInit, this, &ACollectible::Init, InitTimer);
}
//void ACollectible::Init()
//{
	//Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACollectible::OnCollectibleBeginOverlap);
//}

// Called every frame
void ACollectible::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACollectible::OnCollectibleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

}


void ACollectible::Destruction()
{
	/* Particles and disable mesh + collision */
	UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DeathParticles, GetActorLocation());
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetHiddenInGame(true, true);

	/* Destroy object after 10 seconds */
	auto func = [this]() {
		Destroy();
	};
	FTimerDelegate FDelegate;
	FDelegate.BindLambda(func);
	GetWorldTimerManager().SetTimer(FTHDestruction, FDelegate, 2.f, false);
}
