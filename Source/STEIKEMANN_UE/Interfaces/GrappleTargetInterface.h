// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "../GameplayTags.h"

#include "GrappleTargetInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UGrappleTargetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class STEIKEMANN_UE_API IGrappleTargetInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, Category = "GrappleHook Targeting")
		FGameplayTag GetGrappledGameplayTag() const;
	virtual FGameplayTag GetGrappledGameplayTag_Pure() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "GrappleHook Targeting")
		void Targeted();
	virtual void TargetedPure() = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "GrappleHook Targeting")
		void UnTargeted();
	virtual void UnTargetedPure() = 0;



	UFUNCTION(BlueprintNativeEvent, Category = "GrappleHook Targeting")
		void Hooked();
	virtual void HookedPure() = 0;
	virtual void HookedPure(const FVector InstigatorLocation) = 0;

	UFUNCTION(BlueprintNativeEvent, Category = "GrappleHook Targeting")
		void UnHooked();
	virtual void UnHookedPure() = 0;


};
