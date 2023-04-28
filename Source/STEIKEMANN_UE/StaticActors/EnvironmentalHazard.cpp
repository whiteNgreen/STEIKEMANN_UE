// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/EnvironmentalHazard.h"

// Sets default values
AEnvironmentalHazard::AEnvironmentalHazard()
{
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

void AEnvironmentalHazard_Empty::BeginPlay()
{
	GTagContainer.AddTag(Tag::EnvironmentHazard());
}
