#pragma once

#include "GameplayTagAssetInterface.h"

/* Player GameplayTag */
static FGameplayTag Tag_Player = FGameplayTag::RequestGameplayTag("Pottit");

/* Enemy GameplayTags */
static FGameplayTag Tag_Enemy = FGameplayTag::RequestGameplayTag("Enemy");

static FGameplayTag Tag_EnemyAubergineDoggo = FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo");


/* Grappletargets GameplayTags */
static FGameplayTag Tag_GrappleTarget = FGameplayTag::RequestGameplayTag("GrappleTarget");

static FGameplayTag Tag_GrappleTarget_Static = FGameplayTag::RequestGameplayTag("GrappleTarget.Static");

static FGameplayTag Tag_GrappleTarget_Dynamic = FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic");

