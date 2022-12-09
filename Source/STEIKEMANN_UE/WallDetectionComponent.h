// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DebugMacros.h"
#include "STEIKEMANN_UE.h"
#include "WallDetectionComponent.generated.h"

#define ECC_WallDetection ECC_GameTraceChannel3 

namespace Wall {
	struct WallData
	{
		bool valid;
		FVector Location;
		FVector Normal;
	};
	
	struct LedgeData
	{
		FVector Location;
		FVector TraceLocation;
		FVector ActorLocation;
	};
}


UENUM()
enum class EOnWallState : int8
{
	WALL_None,

	WALL_Hang,
	WALL_Drag,
	
	WALL_Ledgegrab,
	
	WALL_Leave
};

UCLASS(BlueprintType)
class STEIKEMANN_UE_API UWallDetectionComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:	
	// Sets default values for this component's properties
	//UWallDetectionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	void SetDebugStatus(bool b) { bShowDebug = b; }

	// Capusle size
	void SetCapsuleSize(float radius, float halfheight) { m_capsule.SetCapsule(radius, halfheight); }
	
	void SetHeight(float minHeight, float playerCapsuleHalfHeight) { m_MinHeight = minHeight, m_OwnerHalfHeight = playerCapsuleHalfHeight; }
	void SetMinLengthToWall(float length) { m_MinLengthToWall = length; }

private:
	bool bShowDebug{};
	FCollisionShape m_capsule;
	float Angle_UpperLimit{  0.7f };
	float Angle_LowerLimit{ -0.7f };
	float m_MinLengthToWall{ 40.f };

public:	// Wall Detection
	// Viable wall angles between upper and lower limit. 

	bool DetectWall(const AActor* actor, const FVector Location, const FVector ForwardVector, Wall::WallData& walldata, Wall::WallData& WallJumpData);

private:
	float m_OwnerHalfHeight{};
	float m_MinHeight{ 40.f };	// from root

	bool DetermineValidPoints_IMPL(TArray<FHitResult>& hits, const FVector& Location);
	bool Valid_WallJumpPoints(TArray<FHitResult>& hits, const FVector& Location);
	void GetWallPoint_IMPL(Wall::WallData& data, const TArray<FHitResult>& hits);

	float GetMinHeight(float z);
	bool ValidLengthToCapsule(FVector Hit, FVector capsuleLocation, float capsuleHeight);

public:	// Ledge detection
	float Ledge_Anglelimit{ 0.8f };

	bool DetectLedge(Wall::LedgeData& ledge, const AActor* actor, const FVector actorLocation, const FVector actorUp, const Wall::WallData& wall, const float height, const float inwardsLength);
private:

	
};
