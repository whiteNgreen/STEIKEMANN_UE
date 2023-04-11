// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AttackInterface.generated.h"

UENUM(BlueprintType)
enum EAttackType
{
	None,
	SmackAttack,
	//Scoop,
	GroundPound,
	PogoBounce,
	Environmental
};

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
	bool bAICanBeDamaged{ true };
	FTimerHandle FTHCanBeDamaged;
	virtual void ResetCanbeDamaged() { bAICanBeDamaged = true; }
	virtual bool CanBeAttacked() = 0;
	virtual void Gen_Attack(IAttackInterface* OtherInterface, AActor* OtherActor, EAttackType& AType){}
	virtual void Gen_ReceiveAttack(const FVector& Direction, const float& Strength, EAttackType& AType){}

	UFUNCTION(BlueprintNativeEvent, Category = "Attack|SmackAttack")
		void SmackAttack();
	//virtual void Do_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) = 0;
	virtual void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor){}
	virtual void Receive_SmackAttack_Pure(const FVector Direction, const float Strength, const bool bOverrideStrength = false){}
	virtual bool GetCanBeSmackAttacked() const { return false; }
	virtual void ResetCanBeSmackAttacked(){}


	UFUNCTION(BlueprintNativeEvent, Category = "Attack|GroundPound")
		void GroundPound();
	virtual void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor){}
	virtual void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) {}
	virtual void Receive_Pogo_GroundPound_Pure(){}


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackInterface|Grapplehook")
		float AttackInterface_LeewayPause_Time{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AttackInterface|Grapplehook")
		float AttackInterface_LeewayPause_Timedilation{ 0.35f };
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Stun")
		void Receive_LeewayPause();
	UFUNCTION(BlueprintNativeEvent, Category = "Attack|Stun")
		void Cause_LeewayPause();
	virtual void Cause_LeewayPause_Pure(float Pausetime) {}
	virtual void Receive_LeewayPause_Pure(float Pausetime) {}
};
