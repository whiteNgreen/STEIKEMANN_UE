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

FORCEINLINE float DotInverted_Normal(const float& dot)
{
	return (-dot / 2.f) + 0.5f;
}

/* A common DotGuassian effect. Used with a dot product */
FORCEINLINE float DotGuassian(const float& dot, const float& a = 0.3f, const float& b = 0.0f)
{
	return FMath::Exp(-(FMath::Pow(dot - b, 2)) / 
					(2.0 * FMath::Pow(a, 2)));
}

FORCEINLINE float Gaussian(const float& x, const float& a = 2.f, const float& b = 1.f)
{
	return FMath::Exp(-FMath::Pow(x, a) * b);
}

FORCEINLINE FVector ReflectionVector(const FVector& n, const FVector& d, const float& reflectionStrength = 1.f)
{
	FVector Ortho = FVector::CrossProduct(n, FVector::CrossProduct(d, n));
	FVector proj = d.ProjectOnTo(Ortho);
	return n + (proj * reflectionStrength);
}
