// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../DebugMacros.h"
#include "CameraGuideInterface.generated.h"

UENUM()
enum class EPointType
{
	LOOKAT_Absolute,
	LOOKAT_Lean,
	CAMERA_Absolute,
	CAMERA_Lean
};

USTRUCT(BlueprintType)
struct FocusPoint 
{
	GENERATED_BODY()

	UObject* Obj{ nullptr };
	FVector Location{};
	/* Priority of Focus Point among other potential Focus Points 
	 * Priority of 0 will ignore every other volume, if two priority 0's overlap, only the first entered will be acknowledged */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint", meta = (UIMin = "0", DisplayPriority = "0"))
		int Priority;

	UPROPERTY(EditAnywhere, Category = "FocusPoint", meta = (DisplayPriority = "1"))
		EPointType Type;

	
	/* -------------------------------- LOOKAT_Absolute -------------------------------- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint", meta = (UIMin = "0.0",
		EditCondition = "Type == EPointType::LOOKAT_Absolute", EditConditionHides))
		float LerpSpeed{ 0.2f };
	
	/* -------------------------------- LOOKAT_Lean -------------------------------- */
	/* Lean Amount: How far towards the target will the camera lean
	 * 1 is directly towards the target, 
	 * 0.5 is 90 degrees 
	 * and 0 in the opposite direction, essentially 0 effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint", meta = (UIMin = "0.0", UIMax = "1.0",
		EditCondition = "Type == EPointType::LOOKAT_Lean", EditConditionHides))
		float LeanAmount{ 0.8f };
	/* Similar to the LeanAmount, but regarding the point the lean should start to relax
	 * NB! Should ALWAYS be lower than the lean amount */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint", meta = (UIMin = "0.0", UIMax = "1.0",
		EditCondition = "Type == EPointType::LOOKAT_Lean", EditConditionHides))
		float LeanRelax{ 0.6f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint", meta = (UIMin = "0.0", UIMax = "1.0",
		EditCondition = "Type == EPointType::LOOKAT_Lean", EditConditionHides))
		float LeanSpeed{ 0.2f };

	/* If the lean strength will be stronger the further away the camera is rotated from the target, 
	 * and get weaker the closer it gets to the desired rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint", meta = (UIMin = "1.0", UIMax = "5.0",
		EditCondition = "Type == EPointType::LOOKAT_Lean", EditConditionHides))
		float LeanMultiplier{ 1.f };

	
	/* -------------------------------- CAMERA_Absolute -------------------------------- */



	/* -------------------------------- CAMERA_Lean -------------------------------- */
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UCameraGuideInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class STEIKEMANN_UE_API ICameraGuideInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	bool bIsWithinCameraGuideVolume{};
	TArray<FocusPoint> mFocusPoints{};

	virtual void GuideCamera(float DeltaTime)=0;	
	
	virtual void AddCameraGuide(const FocusPoint& Point);
	virtual void RemoveCameraGuide(UObject* object);
};
