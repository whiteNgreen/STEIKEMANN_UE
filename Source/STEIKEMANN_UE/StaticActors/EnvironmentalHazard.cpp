// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/EnvironmentalHazard.h"

// Sets default values
AEnvironmentalHazard::AEnvironmentalHazard()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(Root);	
	SphereCollider = CreateDefaultSubobject<USphereComponent>(TEXT("SphereCollider"));
	SphereCollider->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void AEnvironmentalHazard::BeginPlay()
{
	Super::BeginPlay();
	
	GTagContainer.AddTag(Tag::EnvironmentHazard());
}

// Called every frame
void AEnvironmentalHazard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

