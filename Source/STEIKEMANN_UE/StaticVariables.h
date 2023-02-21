#pragma once
#include "CoreMinimal.h"

namespace Statics {


	static inline float AttackContactTimeDilation{ 0.1f };

}

static inline FVector GetRandomLocationAroundPoint2D(const FVector& location, const float& radius, const float& minRadius = 0.f)
{
	float angle = FMath::RandRange(0.f, 360.f);

	float r = (FMath::Max(FMath::RandRange(0.f, radius), minRadius)) / radius;
	float length = FMath::Sqrt(r)/* * FMath::Pow(r, 2)*/;

	return location + (FVector::ForwardVector.RotateAngleAxis(angle, FVector::UpVector) * length * radius);
}

static inline float DotInverted_Normal(const float& dot)
{
	return (-dot / 2.f) + 0.5f;
}

/* A common DotGuassian effect. Used with a dot product */
static inline float DotGuassian(const float& dot, float a = 0.3f, float b = 0.0f)
{
	return FMath::Exp(-(FMath::Pow(dot - b, 2)) / (2.0 * FMath::Pow(a, 2)));
}
