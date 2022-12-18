// Fill out your copyright notice in the Description page of Project Settings.


#include "WallDetectionComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
UWallDetectionComponent::UWallDetectionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UWallDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	//capsule = FCollisionShape::MakeCapsule(Capsule_Radius, Capsule_HalfHeight);
}


// Called every frame
void UWallDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


bool UWallDetectionComponent::DetectWall(const AActor* actor, const FVector Location, const FVector ForwardVector, Wall::WallData& walldata, Wall::WallData& WallJumpData)
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params = FCollisionQueryParams("", false, actor);
	bool b = GetWorld()->SweepMultiByChannel(Hits, Location, Location + ForwardVector, FQuat(1.f, 0, 0, 0), ECC_WallDetection, m_capsule, Params);

	if (bShowDebug)
		DrawDebugCapsule(GetWorld(), Location, m_capsule.GetCapsuleHalfHeight(), m_capsule.GetCapsuleRadius(), FQuat(1.f, 0, 0, 0), FColor(.3f, .3f, 1.f, 1.f), false, 0, 0, 1.f);

	if (!b) return false;
	
	if (bShowDebug)// Debug
		for (const auto& it : Hits)
			DrawDebugPoint(GetWorld(), it.ImpactPoint, 5.f, FColor::Blue, false, 1, 1);

	// Get Valid point on wall, used for WallJump and Ledgegrab
	b = DetermineValidPoints_IMPL(Hits, Location);
	if (!b) return false;
	GetWallPoint_IMPL(walldata, Hits);

	if (bShowDebug)// Debug
		for (const auto& it : Hits)
			DrawDebugPoint(GetWorld(), it.ImpactPoint, 7.f, FColor::Orange, false, 2, 1);


	// Get point valid for Wall Jump mechanic
	TArray<FHitResult> WallJumpHits = Hits;
	const bool walljump = (Valid_WallJumpPoints(WallJumpHits, Location));
	if (walljump) {
		GetWallPoint_IMPL(WallJumpData, WallJumpHits);
		WallJumpData.valid = true;
	}
	else WallJumpData.valid = false;


	if (bShowDebug && WallJumpData.valid) {// Debug
		DrawDebugPoint(GetWorld(), WallJumpData.Location, 12.f, FColor::Red, false, 0, 1);
		DrawDebugLine(GetWorld(), WallJumpData.Location, WallJumpData.Location + WallJumpData.Normal * 50.f, FColor::Purple, false, 0, 1, 4.f);
	}

	return true;
}

bool UWallDetectionComponent::DetectStickyWall(const AActor* actor, const FVector Location, const FVector Forward, Wall::WallData& walldata)
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params = FCollisionQueryParams("", false, actor);
	bool b = GetWorld()->SweepMultiByChannel(Hits, Location, Location + Forward, FQuat(1.f, 0, 0, 0), ECC_WallDetection, m_capsule, Params);

	if (bShowDebug)
		DrawDebugCapsule(GetWorld(), Location, m_capsule.GetCapsuleHalfHeight(), m_capsule.GetCapsuleRadius(), FQuat(1.f, 0, 0, 0), FColor(.3f, .3f, 1.f, 1.f), false, 0, 0, 1.f);

	if (!b) return false;

	if (bShowDebug)// Debug
		for (const auto& it : Hits)
			DrawDebugPoint(GetWorld(), it.ImpactPoint, 5.f, FColor::Blue, false, 1, 1);

	// Get Valid point on wall, used for WallJump and Ledgegrab
	b = DetermineValidPoints_IMPL(Hits, Location);
	if (!b) return false;
	GetWallPoint_IMPL(walldata, Hits);

	if (bShowDebug) {// Debug
		DrawDebugPoint(GetWorld(), walldata.Location, 12.f, FColor::Red, false, 0, 1);
		DrawDebugLine(GetWorld(), walldata.Location, walldata.Location + walldata.Normal * 50.f, FColor::Purple, false, 0, 1, 4.f);
	}

	FHitResult Hit;
	FVector Direction = walldata.Location - Location;
	b = GetWorld()->LineTraceSingleByChannel(Hit, Location, Location + (Direction * 1.1f), ECC_WallDetection, Params);

	if (b)
	{
		DrawDebugLine(GetWorld(), Location, Location + (Direction * 1.1f), FColor::Purple, false, 0, 1, 5.f);

		IGameplayTagAssetInterface* tag = Cast<IGameplayTagAssetInterface>(Hit.GetActor());
		if (!tag) return false;

		FGameplayTagContainer con;
		tag->GetOwnedGameplayTags(con);
		if (con.HasTag(Tag::StickingWall()))
			return true;
	}

	return false;
}

bool UWallDetectionComponent::DetermineValidPoints_IMPL(TArray<FHitResult>& hits, const FVector& Location)
{
	// Get Capsule height
	float h = m_capsule.GetCapsuleHalfHeight() * 2;
	float r = m_capsule.GetCapsuleRadius();
	float a = h - (2*r);
	FVector aLineStart = Location - FVector(0, 0, a / 2);
	if (bShowDebug)
		DrawDebugLine(GetWorld(), aLineStart, aLineStart + (FVector::UpVector * a), FColor::Red, false, 0, 1, 5.f);

	// Cull non-valid points
	for (int32 i{}; i < hits.Num();)
	{
		float angle = hits[i].ImpactNormal.Z;
		if ((angle > Angle_UpperLimit || angle < Angle_LowerLimit)) {
			hits.RemoveAt(i);
			continue;
		}
		if (hits[i].ImpactPoint.Z < GetMinHeight(Location.Z)) {
			hits.RemoveAt(i);
			continue;
		}
		i++;
	}
	if (hits.Num() == 0) return false;
	return true;
}

bool UWallDetectionComponent::Valid_WallJumpPoints(TArray<FHitResult>& hits, const FVector& Location)
{
	// Get Capsule height
	float h = m_capsule.GetCapsuleHalfHeight() * 2;
	float r = m_capsule.GetCapsuleRadius();
	float a = h - (2 * r);
	FVector aLineStart = Location - FVector(0, 0, a / 2);
	if (a <= 0.5f) {
		a = 0;
		aLineStart = Location;
	}
	if (bShowDebug)
		DrawDebugLine(GetWorld(), aLineStart, aLineStart + (FVector::UpVector * a), FColor::Red, false, 0, 1, 5.f);
	for (int32 i{}; i < hits.Num();)
	{
		if (!ValidLengthToCapsule(hits[i].ImpactPoint, aLineStart, a)) {
			hits.RemoveAt(i);
			continue;
		}
		i++;
	}
	if (hits.Num() == 0) return false;
	return true;
}

void UWallDetectionComponent::GetWallPoint_IMPL(Wall::WallData& data, const TArray<FHitResult>& hits)
{
	TArray<FVector> locations;
	TArray<FVector> normals;
	for (const auto& it : hits) 
	{
		locations.Add(it.ImpactPoint);
		normals.Add(it.ImpactNormal);
	}

	FVector loc;
	for (const auto& it : locations)
		loc += it;
	loc /= (float)locations.Num();
	data.Location = loc;

	FVector n = normals[0];
	if (normals.Num() > 1) {
		for (int32 i{ 1 }; i < normals.Num(); i++)
			n += normals[i];
		n.Normalize();
	}
	data.Normal = n;
}

float UWallDetectionComponent::GetMinHeight(float z)
{
	return z + (m_MinHeight - m_OwnerHalfHeight);
}

bool UWallDetectionComponent::ValidLengthToCapsule(FVector Hit, FVector capsuleLocation, float capsuleHeight)
{
	// Get point along capsule line
	float zCapMin = capsuleLocation.Z;
	float zCapMax = capsuleHeight;
	
	float zLine = FMath::Clamp(Hit.Z - zCapMin, 0.f, zCapMax);
	float alpha = zLine / zCapMax;
	FVector Point = capsuleLocation + (FVector::UpVector * (capsuleHeight * alpha));

	float length = FVector(Hit - Point).Size();

	if (length < m_MinLengthToWall)
		return true;

	return false;
}

bool UWallDetectionComponent::DetectLedge(Wall::LedgeData& ledge, const AActor* actor, const FVector actorLocation, const FVector actorUp, const Wall::WallData& wall, const float height, const float inwardsLength)
{
	FVector inwardsVec = FVector::VectorPlaneProject(wall.Normal * -1, FVector::UpVector);
	inwardsVec.Normalize();
	FVector aboveLocation = actorLocation + (actorUp * height);
	FVector inwardsLocation = aboveLocation + (inwardsVec * inwardsLength);

	FHitResult hit;
	FCollisionQueryParams Params = FCollisionQueryParams("", false, actor);

	const bool firstCheck = !GetWorld()->LineTraceSingleByChannel(hit, aboveLocation, inwardsLocation, ECC_WallDetection, Params);
	if (bShowDebug)
		DrawDebugLine(GetWorld(), aboveLocation, inwardsLocation, FColor::Red, false, 5.f, 0, 5.f);

	if (!firstCheck)
		return false;

	TArray<FHitResult> Hits;
	const bool secondCheck = !GetWorld()->SweepMultiByChannel(Hits, inwardsLocation, inwardsLocation + FVector::DownVector, FQuat(1.f, 0, 0, 0), ECC_WallDetection, m_capsule, Params);

	if (bShowDebug)
		DrawDebugCapsule(GetWorld(), inwardsLocation, m_capsule.GetCapsuleHalfHeight(), m_capsule.GetCapsuleRadius(), FQuat(1.f, 0, 0, 0), FColor(1.f, .3f, 0.3f, 1.f), false, 5.f, 0, 1.5f);
	if (bShowDebug)// Debug
		for (const auto& it : Hits)
			DrawDebugPoint(GetWorld(), it.ImpactPoint, 9.f, FColor::White, false, 5.f, 0);

	if (!secondCheck)
		return false;

	
	if (firstCheck && secondCheck)
	{
		FHitResult f;
		const bool final = GetWorld()->LineTraceSingleByChannel(f, inwardsLocation, inwardsLocation + (FVector::DownVector * height), ECC_WallDetection, Params);
		if (bShowDebug)
			DrawDebugLine(GetWorld(), inwardsLocation, inwardsLocation + (FVector::DownVector * height), FColor::Blue, false, 5.f, 0, 5.f);

		if (final)
		{
			if (FVector::DotProduct(f.ImpactNormal, FVector::UpVector) < Ledge_Anglelimit)
				return false;

			ledge.Location = f.ImpactPoint;
			FVector newloc = ledge.Location;
			newloc -= inwardsVec * inwardsLength;
			ledge.TraceLocation = newloc - (actorUp * 5.f);
			newloc -= actorUp * (height/2.f);// TODO: NEW LEDGE GRAB HEIGHT. Detection and ledge grab height being two different things
			ledge.ActorLocation = newloc;
			if (bShowDebug)
				DrawDebugPoint(GetWorld(), ledge.Location, 10.f, FColor::Purple, false, 10.f, 0);
			return true;
		}
	}

	return false;
}

