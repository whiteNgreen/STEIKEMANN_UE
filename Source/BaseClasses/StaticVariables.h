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

namespace SMath 
{
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

	/**
	* Gaussian function
	* Allows for various changes to the curves sharpness at points
	* 
	* Expected return for this use is: x > 0
	* as the Exponent_TopSharpness being a float MAY cause the negative returns to reach inf(nan)
	*/
	FORCEINLINE float Gaussian(float x, float BotSharpness = 1.f, float Exponent_TopSharpness = 2.f, float TopPlacement = 0.f, float EndValue = 0.f) {
		return FMath::Exp(-FMath::Pow(x - TopPlacement, Exponent_TopSharpness) * BotSharpness) * (1.f - EndValue) + EndValue;
	}
	/**
	* Same Gaussian function as the one above
	* Exponent is integer so return will work for negative and positive values 
	*/
	FORCEINLINE float Gaussian(float x, float BotSharpness = 1.f, int Exponent_TopSharpness = 2, float TopPlacement = 0.f, float EndValue = 0.f) {
		return FMath::Exp(-FMath::Pow(x - TopPlacement, Exponent_TopSharpness) * BotSharpness) * (1.f - EndValue) + EndValue;
	}

	/**
	* Simple Gaussian function
	* 
	* BotLevel Adheres to TopLevel, meaning it is its units below or above the TopLevel
	*/
	FORCEINLINE float SimpleGaussian(float x, float SharpNess, float TopLevel, float TopPlacement, float BotLevel) {
		return FMath::Exp(-FMath::Pow(x - TopPlacement, 2) * SharpNess) * BotLevel + TopLevel;
	}

	FORCEINLINE float InvertedGaussian(float x, float TopSharpness, float BotSharpness) {
		return FMath::Min(-FMath::Exp(-FMath::Pow(x, BotSharpness) * TopSharpness) + 1.f, 1.f);
	}

	FORCEINLINE FVector ReflectionVector(const FVector& n, const FVector& d, const float& reflectionStrength = 1.f)
	{
		FVector Ortho = FVector::CrossProduct(n, FVector::CrossProduct(d, n));
		FVector proj = d.ProjectOnTo(Ortho);
		return n + (proj * reflectionStrength);
	}
}