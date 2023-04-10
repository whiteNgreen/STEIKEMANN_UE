#pragma once
#include "CoreMinimal.h"

/// SmallEnemy.h
UENUM()
enum class EGravityState : int8
{
	Default,
	LerpToDefault,

	None,
	LerpToNone,
	ForcedNone
};
UENUM()
enum class EEnemyState : int8
{
	STATE_None,

	STATE_OnGround,
	STATE_InAir,
	STATE_PreLaunched,
	STATE_Launched,

	STATE_OnWall
};
UENUM()
enum class EWall : int8
{
	WALL_None,

	WALL_Stuck,

	WALL_Leaving
};
struct SpawnPointData
{
	FVector Location;
	FVector IdleLocation;
	float Radius_Min;
	float Radius_Max;
};


/// EnemyAIController.h
UENUM(BlueprintType)
enum class EDogType : uint8
{
	Red,
	Pink,
	Teal
};
UENUM(BlueprintType)
enum class ESmallEnemyAIState : uint8
{
	RecentlySpawned,
	Idle,
	Alerted,
	ChasingTarget,
	GuardSpawn,
	Attack,

	Incapacitated,

	None
};
UENUM(BlueprintType)
enum class EIdleState : uint8
{
	MoveTo_GuardLocation,
	Guard,

	Sleeping,
	MovingTo_SleepLocation,

	None
};
UENUM(BlueprintType)
enum class EAI_ChaseState : uint8
{
	None,
	ChasePlayer_ValidPath,
	ChasePlayer_InvalidPath
};
UENUM(BlueprintType)
enum class EAIIncapacitatedType : uint8
{
	None,
	Stunned,
	Grappled,
	StuckToWall
};
UENUM(BlueprintType)
enum class EAIPost_IncapacitatedType : uint8
{
	None,
	ChasePlayer,
	ConfusedScreaming
};
