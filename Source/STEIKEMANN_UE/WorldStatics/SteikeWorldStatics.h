// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#define _Statics_PlayerDistaceToActive 1e10
DECLARE_DELEGATE_OneParam(FActiveActorDeleted, AActor* actor)
/**
 * 
 */
class STEIKEMANN_UE_API SteikeWorldStatics
{
public:
	static FVector PlayerLocation;
	static FVector CameraLocation;
	static class ASteikemannCharacter* Player;
	static FActiveActorDeleted ActiveActorDeleted;
};
