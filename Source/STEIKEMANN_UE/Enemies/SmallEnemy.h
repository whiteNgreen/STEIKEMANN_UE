// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/AttackInterface.h"
#include "../DebugMacros.h"
#include "GameplayTagAssetInterface.h"

#include "SmallEnemy.generated.h"

UCLASS()
class STEIKEMANN_UE_API ASmallEnemy : public ACharacter,
	public IAttackInterface,
	public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASmallEnemy();

	/*
	GameplayTags
	*/

	UPROPERTY(BlueprintReadOnly, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayTags")
		FGameplayTag Enemy;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; return; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


public:
	bool bCanBeSmackAttacked{ true };

	FTimerHandle THandle_GotSmackAttacked{};

	/**
	*	Sets a timer before character can be damaged again 
	*	With respect to the specific handle it should use */
	void WaitBeforeNewDamage(FTimerHandle TimerHandle, float Time);	
	

	void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting SmackAttacked
	void Receive_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) override;
	bool GetCanBeSmackAttacked() const override { return bCanBeSmackAttacked; }
	void ResetCanBeSmackAttacked() override { bCanBeSmackAttacked = true; }

	void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override {}
	void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) override;
};