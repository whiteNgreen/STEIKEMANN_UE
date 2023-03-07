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
	ShroomLocation = CollisionBox->GetComponentLocation();
}

bool ABouncyShroom::GetBounceInfo(const FVector actorLocation, const FVector normalImpulse, const FVector IncommingDirection, FVector& OUT_direction, float& OUT_strength)
{
	if (normalImpulse.SizeSquared() >= 0.2) {
		if (FVector::DotProduct(normalImpulse.GetSafeNormal(), ShroomDirection) < 0.7f) 
			return false;
	}
	else {
		if (FVector::DotProduct(FVector(actorLocation - ShroomLocation).GetSafeNormal(), ShroomDirection) < 0.f)
			return false;
	}
	
	if (bReflectDirection){
		OUT_direction = ReflectionVector(ShroomDirection, IncommingDirection, ReflectionStrength);

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + OUT_direction * 100.f, FColor::White, false, 2.f, -1, 8.f);
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + ShroomDirection * 100.f, FColor::Blue, false, 2.f, -1, 8.f);
	}
	else {
		OUT_direction = ShroomDirection;
	}
	OUT_strength *= BounceMultiplier;
	return true;
}

