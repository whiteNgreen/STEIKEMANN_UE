// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/DeathZone.h"

ADeathZone::ADeathZone()
{
	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(Root);
}

void ADeathZone::BeginPlay()
{
	GTagContainer.AddTag(Tag::DeathZone());
}
