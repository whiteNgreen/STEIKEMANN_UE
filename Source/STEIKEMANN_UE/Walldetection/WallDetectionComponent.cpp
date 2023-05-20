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
}


// Called when the game starts
void UWallDetectionComponent::BeginPlay()
{
	Super::BeginPlay();
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
	bool b = GetWorld()->SweepMultiByChannel(Hits, Location, Location + ForwardVector, FQuat(1.f, 0, 0, 0), ECC_PlayerWallDetection, m_capsule, Params);
	if (!b) return false;

	// Get Valid point on wall, used for WallJump and Ledgegrab
	b = DetermineValidPoints_IMPL(Hits, Location);
	if (!b) return false;
	GetWallPoint_IMPL(walldata, Hits);

	// If wall is below the waist line then dont grab on to it. Instead call for ledge detection later in SteikeCharacter
	//const FVector ActorMiddle_Start = actor->GetActorLocation() + FVector(0, 0, ActorMiddleOffset);
	//const FVector ActorMiddle_End = ActorMiddle_Start - walldata.Normal * m_capsule.GetCapsuleRadius();
	//FHitResult ActorMiddleHit;
	//const bool actormiddlewall = GetWorld()->LineTraceSingleByChannel(ActorMiddleHit, ActorMiddle_Start, ActorMiddle_End, ECC_PlayerWallDetection, Params);
	//const float MinZForMiddleCheck = actor->GetActorLocation().Z - 70.f;
	//walldata.bInvalidWallDetected_DoLedgeDetection = !actormiddlewall && (ActorMiddleHit.ImpactPoint.Z > MinZForMiddleCheck);
	//if (walldata.bInvalidWallDetected_DoLedgeDetection) {
	//	DrawDebugLine(GetWorld(), ActorMiddle_Start, ActorMiddle_End, FColor::Red, false, 1.f, 0, 4.f);
	//	return false;
	//}

	// Get point valid for Wall Jump mechanic
	TArray<FHitResult> WallJumpHits = Hits;
	const bool walljump = (Valid_WallJumpPoints(WallJumpHits, Location));
	if (walljump) {
		GetWallPoint_IMPL(WallJumpData, WallJumpHits);
		WallJumpData.valid = true;
	}
	else 
		WallJumpData.valid = false;

	return true;
}

bool UWallDetectionComponent::DetectStickyWall(const AActor* actor, const FVector Location, const FVector Forward, Wall::WallData& walldata, ECollisionChannel TraceChannel)
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params = FCollisionQueryParams("", false, actor);
	bool b = GetWorld()->SweepMultiByChannel(Hits, Location, Location + Forward, FQuat(1.f, 0, 0, 0), TraceChannel, m_capsule, Params);
	if (!b) return false;
	GetWallPoint_IMPL(walldata, Hits);

	FHitResult Hit;
	FVector Direction = walldata.Location - Location;
	b = GetWorld()->LineTraceSingleByChannel(Hit, Location, Location + (Direction * 1.1f), ECC_EnemyWallDetection, Params);

	if (b)
	{
		return true;

		// Do I really need to check the tag? This should maybe just be trace channel instead
		IGameplayTagAssetInterface* tag = Cast<IGameplayTagAssetInterface>(Hit.GetActor());
		if (!tag) return false;

		FGameplayTagContainer con;
		tag->GetOwnedGameplayTags(con);
		if (con.HasTag(Tag::StickingWall()))
			return true;
	}

	return false;
}

bool UWallDetectionComponent::DetectStickyWallOnNormalWithinAngle(FVector actorlocation, const float dotprodlimit, FVector normal)
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params = FCollisionQueryParams("", false, GetOwner());
	if (GetWorld()->SweepMultiByChannel(Hits, actorlocation, actorlocation + FVector::ForwardVector, FQuat(1.f, 0, 0, 0), ECC_StickyWall, m_capsule, Params))
	{
		for (const auto& Hit : Hits) {
			const FVector ToPoint = FVector(actorlocation - Hit.ImpactPoint).GetSafeNormal();
			const float Dot = FVector::DotProduct(normal, ToPoint);
			if (Dot > dotprodlimit) 
				return true;
		}
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
	GetAverageWallLocationAndNormalFromSweep(hits, data.Location, data.Normal);
	//TArray<FVector> locations;
	//TArray<FVector> normals;
	//for (const auto& it : hits) 
	//{
	//	locations.Add(it.ImpactPoint);
	//	normals.Add(it.ImpactNormal);
	//}
	//FVector loc;
	//for (const auto& it : locations)
	//	loc += it;
	//loc /= (float)locations.Num();
	//data.Location = loc;

	//FVector n = normals[0];
	//if (normals.Num() > 1) {
	//	for (int32 i{ 1 }; i < normals.Num(); i++)
	//		n += normals[i];
	//	n.Normalize();
	//}
	//data.Normal = n;
}

void UWallDetectionComponent::GetAverageWallLocationAndNormalFromSweep(const TArray<FHitResult>& hits, FVector& location, FVector& normal)
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
	location = loc;

	FVector n = normals[0];
	if (normals.Num() > 1) {
		for (int32 i{ 1 }; i < normals.Num(); i++)
			n += normals[i];
		n.Normalize();
	}
	normal = n;
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

	const bool firstCheck = !GetWorld()->LineTraceSingleByChannel(hit, aboveLocation, inwardsLocation, ECC_PlayerWallDetection, Params);
	if (!firstCheck)
		return false;

	TArray<FHitResult> Hits;
	const bool secondCheck = !GetWorld()->SweepMultiByChannel(Hits, inwardsLocation, inwardsLocation + FVector::DownVector, FQuat(1.f, 0, 0, 0), ECC_PlayerWallDetection, m_capsule, Params);

	if (!secondCheck)
		return false;

	
	if (firstCheck && secondCheck)
	{
		FHitResult f;
		const bool final = GetWorld()->LineTraceSingleByChannel(f, inwardsLocation, inwardsLocation + (FVector::DownVector * height), ECC_PlayerWallDetection, Params);
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
			return true;
		}
	}

	return false;
}

bool UWallDetectionComponent::DetectLedge_Sweep(Wall::LedgeData& ledge, const AActor* actor, const FVector actorLocation, const FVector actorUp, const float height, const float inwardsLength)
{
	const FCollisionQueryParams Params = FCollisionQueryParams("", false, actor);
	FHitResult wallHit;
	FCollisionShape sphere = FCollisionShape::MakeSphere(250.f);
	GetWorld()->SweepSingleByChannel(wallHit, actor->GetActorLocation(), actor->GetActorLocation() + FVector::DownVector, FQuat(1, 0, 0, 0), ECC_PlayerWallDetection, sphere, Params);
	const FVector inwardsVec = -wallHit.ImpactNormal;

	FVector aboveLocation = actorLocation + (actorUp * height);
	FVector inwardsLocation = aboveLocation + (inwardsVec * inwardsLength);

	FHitResult hit;

	const bool firstCheck = !GetWorld()->LineTraceSingleByChannel(hit, aboveLocation, inwardsLocation, ECC_PlayerWallDetection, Params);
	if (!firstCheck)
		return false;

	TArray<FHitResult> Hits;
	const bool secondCheck = !GetWorld()->SweepMultiByChannel(Hits, inwardsLocation, inwardsLocation + FVector::DownVector, FQuat(1.f, 0, 0, 0), ECC_PlayerWallDetection, m_capsule, Params);

	if (!secondCheck)
		return false;


	if (firstCheck && secondCheck)
	{
		FHitResult f;
		const bool final = GetWorld()->LineTraceSingleByChannel(f, inwardsLocation, inwardsLocation + (FVector::DownVector * height), ECC_PlayerWallDetection, Params);
		if (final)
		{
			if (FVector::DotProduct(f.ImpactNormal, FVector::UpVector) < Ledge_Anglelimit)
				return false;

			ledge.Location = f.ImpactPoint;
			FVector newloc = ledge.Location;
			newloc -= inwardsVec * inwardsLength;
			ledge.TraceLocation = newloc - (actorUp * 5.f);
			newloc -= actorUp * (height / 2.f);// TODO: NEW LEDGE GRAB HEIGHT. Detection and ledge grab height being two different things
			ledge.ActorLocation = newloc;
			return true;
		}
	}

	return false;
}

bool UWallDetectionComponent::DetectLedge_New(Wall::LedgeData& ledge)
{
	const FCollisionQueryParams Params = FCollisionQueryParams("", false, GetOwner());


	// Detect nearby wall and get the average normal and location from it
	TArray<FHitResult> Hits;
	const FCollisionShape capsule = FCollisionShape::MakeCapsule(LD_inward, LD_height);
	const FVector WallD_Start = GetOwner()->GetActorLocation();
	const FVector WallD_End = WallD_Start + GetOwner()->GetActorForwardVector();
	const bool b1 = GetWorld()->SweepMultiByChannel(Hits, WallD_Start, WallD_End, FQuat(1, 0, 0, 0), ECC_PlayerWallDetection, capsule, Params);
	if (!b1)
		return false;
	FVector WallLoc;
	FVector WallNormal;
	GetAverageWallLocationAndNormalFromSweep(Hits, WallLoc, WallNormal);

	// Detect Ledge
	FVector inwardsVec = FVector::VectorPlaneProject(WallNormal * -1, FVector::UpVector);
	inwardsVec.Normalize();
	FVector aboveLocation = GetOwner()->GetActorLocation() + (GetOwner()->GetActorUpVector() * LD_height);
	FVector inwardsLocation = aboveLocation + (inwardsVec * LD_inward);

	FHitResult hit;
	const bool firstCheck = !GetWorld()->LineTraceSingleByChannel(hit, aboveLocation, inwardsLocation, ECC_PlayerWallDetection, Params);
	if (!firstCheck)
		return false;

	const bool secondCheck = !GetWorld()->SweepMultiByChannel(Hits, inwardsLocation, inwardsLocation + FVector::DownVector, FQuat(1.f, 0, 0, 0), ECC_PlayerWallDetection, m_capsule, Params);
	if (!secondCheck)
		return false;


	if (firstCheck && secondCheck)
	{
		FHitResult f;
		const bool final = GetWorld()->LineTraceSingleByChannel(f, inwardsLocation, inwardsLocation + (FVector::DownVector * LD_height), ECC_PlayerWallDetection, Params);
		if (final)
		{
			if (FVector::DotProduct(f.ImpactNormal, FVector::UpVector) < Ledge_Anglelimit)
				return false;

			ledge.Location = f.ImpactPoint;
			FVector newloc = ledge.Location;
			newloc -= inwardsVec * LD_inward;
			ledge.TraceLocation = newloc - (GetOwner()->GetActorUpVector() * 5.f);
			newloc -= GetOwner()->GetActorUpVector() * (LD_height / 2.f);// TODO: NEW LEDGE GRAB HEIGHT. Detection and ledge grab height being two different things
			ledge.ActorLocation = newloc;
			return true;
		}
	}

	return false;
}

