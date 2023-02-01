#pragma once

#include "GameplayTagAssetInterface.h"
#include "DebugMacros.h"

namespace Tag {

	// Player
	static FGameplayTag Player() { 
		static FGameplayTag p{ FGameplayTag::RequestGameplayTag("Pottit") };
		return p; 
	}

	// Enemies
	static FGameplayTag Enemy() { return FGameplayTag::RequestGameplayTag("Enemy"); }
	static FGameplayTag AubergineDoggo() { return FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo"); }

	// Enemy Spawner
	static FGameplayTag EnemySpawner() { return FGameplayTag::RequestGameplayTag("EnemySpawner"); }

	// Pogo 
	static FGameplayTag PogoTarget() { return FGameplayTag::RequestGameplayTag("PogoTarget"); }

	// Grapple hook
	static FGameplayTag GrappleTarget() { return FGameplayTag::RequestGameplayTag("GrappleTarget"); }
	static FGameplayTag GrappleTarget_Static() { return FGameplayTag::RequestGameplayTag("GrappleTarget.Static"); }
	static FGameplayTag GrappleTarget_Dynamic() { return FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic"); }

	// Camera
	static FGameplayTag CameraVolume() { return FGameplayTag::RequestGameplayTag("CameraVolume"); }

	// Player Instant death and respawn
	static FGameplayTag PlayerRespawn() { return FGameplayTag::RequestGameplayTag("PlayerRespawn"); }
	static FGameplayTag DeathZone() { return FGameplayTag::RequestGameplayTag("DeathZone"); }

	// Environment
	static FGameplayTag Environment() { return FGameplayTag::RequestGameplayTag("Environment"); }
	static FGameplayTag CorruptionCore() { return FGameplayTag::RequestGameplayTag("Environment.CorruptionCore"); }
	static FGameplayTag EnvironmentHazard() { return FGameplayTag::RequestGameplayTag("Environment.Hazard"); }
	static FGameplayTag StickingWall() { return FGameplayTag::RequestGameplayTag("Environment.StickingWall"); }

	// Collectible
	static FGameplayTag Collectible() { return FGameplayTag::RequestGameplayTag("Collectible"); }
}