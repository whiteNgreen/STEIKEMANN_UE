// Fill out your copyright notice in the Description page of Project Settings.


#include "BouncyShroom.h"

ABouncyShroom::ABouncyShroom()
{
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("Shroom Mesh");
	Mesh->SetupAttachment(Root);
	CollisionBox = CreateDefaultSubobject<UBoxComponent>("Box Collision");
	CollisionBox->SetupAttachment(Mesh, FName("Socket_ShroomCollision"));
	DirectionArrowComp = CreateDefaultSubobject<UArrowComponent>("Direction Arrow");
	DirectionArrowComp->SetupAttachment(Mesh);
}

void ABouncyShroom::BeginPlay()
{
	Super::BeginPlay();

	GTagContainer.AddTag(Tag::BouncyShroom());
	ShroomDirection = DirectionArrowComp->GetComponentRotation().Vector();
}

bool ABouncyShroom::GetBounceInfo(const FVector normalImpulse, FVector& OUT_direction, float& OUT_strength)
{
	//if (FVector::DotProduct(normalImpulse.GetSafeNormal(), ShroomDirection) < 0.7) 
		//return false;

	OUT_direction = ShroomDirection;
	OUT_strength *= BounceMultiplier;
	return true;
}

