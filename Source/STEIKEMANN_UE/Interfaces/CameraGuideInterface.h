// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../DebugMacros.h"
#include "CameraGuideInterface.generated.h"

USTRUCT(BlueprintType)
struct FocusPoint 
{
	GENERATED_BODY()

	UObject* Obj{ nullptr };
	int Priority{};
	float Weight{};
	FVector Location{};
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

	virtual void GuideCamera()=0;	
	
	virtual void AddCameraGuide(const FocusPoint& Point);
	virtual void RemoveCameraGuide(UObject* object);
};
