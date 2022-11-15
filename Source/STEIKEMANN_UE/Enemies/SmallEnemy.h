// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/AttackInterface.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../DebugMacros.h"
#include "GameplayTagAssetInterface.h"
//#include "../GameplayTags.h"

#include "SmallEnemy.generated.h"

UCLASS()
class STEIKEMANN_UE_API ASmallEnemy : public ACharacter,
	public IAttackInterface,
	public IGameplayTagAssetInterface,
	public IGrappleTargetInterface
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
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayTags")
		//FGameplayTag* Enemy{ nullptr };

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; return; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	void RotateActorYawToVector(FVector AimVector, float DeltaTime = 0);

public: 
	bool bCanBeGrappleHooked{ true };
	/* The internal cooldown before enemy can be grapplehooked again */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappleHookedInternalCooldown{ 0.5f };

	FTimerHandle Handle_GrappledCooldown;
	void ResetCanBeGrappleHooked() { bCanBeGrappleHooked = true; }

	/* Choice between the first and second grapplelaunch method */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		bool bUseFirstGrappleLaunchMethod{ true };

	/* When grapplehooked by the player, launch towards them with this strength : 1st method */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappledLaunchStrength{ 1000.f };

	/* When grapplehooked by the player, launch them upwards of this angle : 1st method*/	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappledLaunchAngle{ 45.f };

	/* Time it should take to reach the Grappled Instigator : 2nd method */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappledLaunchTime{ 1.f };

	/* ----- Grapple Interface ------ */
	virtual void TargetedPure() override;

	virtual void UnTargetedPure() override;

	virtual void InReach_Pure() override;

	virtual void OutofReach_Pure() override;

	virtual void HookedPure() override;
	virtual void HookedPure(const FVector InstigatorLocation, bool PreAction = false) override;

	virtual void UnHookedPure() override;

	//virtual FGameplayTag GetGrappledGameplayTag_Pure() const override { return Enemy; }

public:
	bool bCanBeSmackAttacked{ true };

	FTimerHandle THandle_GotSmackAttacked{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|SmackAttack")
		float SmackAttack_OnGroundMultiplication{ 0.1f };

	/**
	*	Sets a timer before character can be damaged again 
	*	With respect to the specific handle it should use */
	void WaitBeforeNewDamage(FTimerHandle TimerHandle, float Time);	
	
	bool CanBeAttacked() override;

	void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting SmackAttacked
	void Receive_SmackAttack_Pure(const FVector& Direction, const float& Strength) override;
	bool GetCanBeSmackAttacked() const override { return bCanBeSmackAttacked; }
	void ResetCanBeSmackAttacked() override { bCanBeSmackAttacked = true; }


	void Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting Scooped
	void Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength) override;

	void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override {}
	void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) override;
};
