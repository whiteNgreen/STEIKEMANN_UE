// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttackInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UAttackInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class STEIKEMANN_UE_API IAttackInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, Category = "Attack|SmackAttack")
		void SmackAttack();
	//virtual void Do_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) = 0;
	virtual void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) = 0;
	virtual void Receive_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) = 0;
	virtual bool GetCanBeSmackAttacked() const = 0;
	virtual void ResetCanBeSmackAttacked() = 0;


	UFUNCTION(BlueprintNativeEvent, Category = "Attack|GroundPound")
		void GroundPound();
	virtual void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) = 0;
	virtual void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) = 0;
	
};
