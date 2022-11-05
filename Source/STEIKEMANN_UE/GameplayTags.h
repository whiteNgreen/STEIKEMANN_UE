#pragma once

#include "GameplayTagAssetInterface.h"
#include "DebugMacros.h"


	
static FGameplayTag TAG_Player() { return FGameplayTag::RequestGameplayTag("Pottit"); }

static FGameplayTag TAG_Enemy() { return FGameplayTag::RequestGameplayTag("Enemy"); }
static FGameplayTag TAG_AubergineDoggo() { return FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo"); }

static FGameplayTag TAG_GrappleTarget() { return FGameplayTag::RequestGameplayTag("GrappleTarget"); }
static FGameplayTag TAG_GrappleTarget_Static() { return FGameplayTag::RequestGameplayTag("GrappleTarget.Static"); }
static FGameplayTag TAG_GrappleTarget_Dynamic() { return FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic"); }

static FGameplayTag TAG_CameraVolume(){ return FGameplayTag::RequestGameplayTag("CameraVolume"); }

