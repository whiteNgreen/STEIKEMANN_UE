#pragma once

#include "GameplayTagAssetInterface.h"
#include "DebugMacros.h"

namespace Tag {

	static FGameplayTag Player() { return FGameplayTag::RequestGameplayTag("Pottit"); }


	static FGameplayTag Enemy() { return FGameplayTag::RequestGameplayTag("Enemy"); }
	static FGameplayTag AubergineDoggo() { return FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo"); }

	static FGameplayTag GrappleTarget() { return FGameplayTag::RequestGameplayTag("GrappleTarget"); }
	static FGameplayTag GrappleTarget_Static() { return FGameplayTag::RequestGameplayTag("GrappleTarget.Static"); }
	static FGameplayTag GrappleTarget_Dynamic() { return FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic"); }

	static FGameplayTag CameraVolume() { return FGameplayTag::RequestGameplayTag("CameraVolume"); }


	static FGameplayTag Environment() { return FGameplayTag::RequestGameplayTag("Environment"); }
	static FGameplayTag CorruptionCore() { return FGameplayTag::RequestGameplayTag("Environment.CorruptionCore"); }
	static FGameplayTag EnvironmentHazard() { return FGameplayTag::RequestGameplayTag("Environment.Hazard"); }

	static FGameplayTag Collectible() { return FGameplayTag::RequestGameplayTag("Collectible"); }
}