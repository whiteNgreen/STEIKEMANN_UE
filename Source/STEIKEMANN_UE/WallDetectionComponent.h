// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DebugMacros.h"
#include "STEIKEMANN_UE.h"
#include "WallDetectionComponent.generated.h"

#define ECC_WallDetection ECC_GameTraceChannel3 

struct WallData
{
	FVector Location;
	FVector Normal;
};

UENUM()
enum class EOnWallState : int32
{
	WALL_None,
	WALL_Hang,
	WALL_Drag,
	WALL_Leave
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STEIKEMANN_UE_API UWallDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWallDetectionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// Capusle size
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection|Capsule")
		float Capsule_Radius{ 75.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection|Capsule")
		float Capsule_HalfHeight{ 100.f };

	// Viable wall angles between upper and lower limit. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection|Viable Angles")
		float Angle_UpperLimit{ 0.7f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detection|Viable Angles")
		float Angle_LowerLimit{ -0.7f };

	bool DetectWall(WallData& data);
private:
	bool DetermineValidPoints_IMPL(TArray<FHitResult>& hits);
	void GetWallPoint_IMPL(WallData& data, const TArray<FHitResult>& hits);

};
