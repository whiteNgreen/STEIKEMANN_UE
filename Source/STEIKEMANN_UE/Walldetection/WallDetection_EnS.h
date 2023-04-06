#pragma once
#include "CoreMinimal.h"

#define ECC_PlayerWallDetection ECC_GameTraceChannel3 
#define ECC_EnemyWallDetection ECC_GameTraceChannel4

namespace Wall {
	struct WallData
	{
		bool valid;
		FVector Location;
		FVector Normal;
	};

	struct LedgeData
	{
		FVector Location;
		FVector TraceLocation;
		FVector ActorLocation;
	};
}


UENUM()
enum class EOnWallState : int8
{
	WALL_None,

	WALL_Hang,
	WALL_Drag,

	WALL_Ledgegrab,

	WALL_Leave
};