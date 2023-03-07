// Fill out your copyright notice in the Description page of Project Settings.


#include "WaterPuddle.h"

AWaterPuddle::AWaterPuddle()
{
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh Plane");
	Mesh->SetupAttachment(Root);
	WaterCollision = CreateDefaultSubobject<UBoxComponent>("Water Collider");
	WaterCollision->SetupAttachment(Mesh);
	WaterCollision->SetBoxExtent(FVector(50.f));
	WaterCollision->SetRelativeLocation(FVector(0, 0, -50.f));
}

void AWaterPuddle::BeginPlay()
{
	Super::BeginPlay();

	GTagContainer.AddTag(Tag::WaterPuddle());
}
