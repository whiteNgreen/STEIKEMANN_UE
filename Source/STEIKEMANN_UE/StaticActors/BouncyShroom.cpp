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
		OUT_direction = SMath::ReflectionVector(ShroomDirection, IncommingDirection, ReflectionStrength);
	}
	else {
		OUT_direction = ShroomDirection;
	}
	OUT_strength = BounceStrength;
	ShroomBounce_Impl();
	return true;
}


void ABouncyShroom::DrawProjectedBouncePath(float time, float drawtime, float GravityMulti)
{
	if (!GetWorld()) return;
	float delta = 1.f / 60.f;

	FVector Start = DirectionArrowComp->GetComponentLocation();
	FVector Direction = DirectionArrowComp->GetComponentRotation().Vector();
	FVector Velocity = Direction * BounceStrength;
	FVector Gravity = FVector(0, 0, -980.f * GravityMulti);
	FVector End = Start + (Velocity * delta);
	FHitResult Hit;
	FCollisionQueryParams Params("", false, this);
	for (float i = 0.f; i < time; i += delta)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, drawtime, 0, 5.f);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params)) {
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 35.f, FColor::Red, false, drawtime, 0);
			return;
		}
		Velocity += Gravity * delta;
		Start = End;
		End += Velocity * delta;
	}
}

