#pragma once

#include "GameplayTagAssetInterface.h"


	/* Player GameplayTag */
	static FGameplayTag Tag_Player;
	static FGameplayTag GetTag_Player()
	{
		Tag_Player = FGameplayTag::RequestGameplayTag("Pottit");
		return Tag_Player;
	}

	static FGameplayTag Tag_Enemy;
	static FGameplayTag GetTag_Enemy()
	{
		Tag_Enemy = FGameplayTag::RequestGameplayTag("Enemy");
		return Tag_Enemy;
	}

	static FGameplayTag Tag_EnemyAubergineDoggo;
	static FGameplayTag GetTag_EnemyAubergineDoggo()
	{
		Tag_EnemyAubergineDoggo = FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo");
		return Tag_EnemyAubergineDoggo;
	}

	static FGameplayTag Tag_GrappleTarget;
	static FGameplayTag GetTag_GrappleTarget()
	{
		Tag_GrappleTarget = FGameplayTag::RequestGameplayTag("GrappleTarget");
		return Tag_GrappleTarget;
	}

	static FGameplayTag Tag_GrappleTarget_Static;
	static FGameplayTag GetTag_GrappleTarget_Static() 
	{ 
		Tag_GrappleTarget_Static = FGameplayTag::RequestGameplayTag("GrappleTarget.Static");
		return Tag_GrappleTarget_Static; 
	}

	static FGameplayTag Tag_GrappleTarget_Dynamic;
	static FGameplayTag GetTag_GrappleTarget_Dynamic()
	{
		Tag_GrappleTarget_Dynamic = FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic");
		return Tag_GrappleTarget_Dynamic;
	}

