// Fill out your copyright notice in the Description page of Project Settings.


#include "WallDetectionTester.h"

// Sets default values
AWallDetectionTester::AWallDetectionTester()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AWallDetectionTester::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWallDetectionTester::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Draw_WallDetection(DeltaTime);
}

void AWallDetectionTester::Draw_WallDetection(float DeltaTime)
{
	if (!GetWorld()) return;
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params("", false, this);
	FCollisionShape capsule = FCollisionShape::MakeCapsule(40.f, 100.f);
	FColor color = FColor::Red;

	const bool b = GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation() + (FVector::UpVector * 100.f), FQuat(), ECC_GameTraceChannel3, capsule, Params);
	if (b)
	{
		color = FColor::Emerald;
		GEngine->AddOnScreenDebugMessage(0, 0, FColor::Cyan, FString::Printf(TEXT("Hits = %i"), Hits.Num()));
		for (const auto& hit : Hits) {
			FVector loc = hit.ImpactPoint;
			DrawDebugPoint(GetWorld(), loc, 15.f, FColor::Blue, false, 0, 1);
			DrawDebugLine(GetWorld(), loc, loc + (hit.ImpactNormal * 50.f), FColor::White, false, 0, -1, 3.f);
			GEngine->AddOnScreenDebugMessage(0, 0, FColor::Cyan, FString::Printf(TEXT("Loc = %s"), *loc.ToString()));

		}
	}

	DrawDebugCapsule(GetWorld(), GetActorLocation(), capsule.GetCapsuleHalfHeight(), capsule.GetCapsuleRadius(), FQuat(1, 0, 0, 0), color, false, 0, 0, 4.f);
}
