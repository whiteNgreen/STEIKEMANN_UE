// Fill out your copyright notice in the Description page of Project Settings.


#include "SteikeWorldStatics.h"
#include "../Steikemann/SteikemannCharacter.h"

FVector SteikeWorldStatics::PlayerLocation = FVector();
FVector SteikeWorldStatics::CameraLocation = FVector();
ASteikemannCharacter* SteikeWorldStatics::Player = nullptr;
FActiveActorDeleted SteikeWorldStatics::ActiveActorDeleted = FActiveActorDeleted();