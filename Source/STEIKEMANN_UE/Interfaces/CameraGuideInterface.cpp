// Fill out your copyright notice in the Description page of Project Settings.


#include "../Interfaces/CameraGuideInterface.h"

// Add default functionality here for any ICameraGuideInterface functions that are not pure virtual.

void ICameraGuideInterface::AddCameraGuide(const FocusPoint& Point)
{
	for (const auto& it : mFocusPoints) {
		if (it.Obj == Point.Obj) { return; }
	}

	mFocusPoints.Add(Point);
}

void ICameraGuideInterface::RemoveCameraGuide(UObject* object)
{
	for (size_t i{}; i < mFocusPoints.Num(); i++){
		if (mFocusPoints[i].Obj == object){
			mFocusPoints.RemoveAt(i);
			return;
		}
	}
}
