// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BaseClasses/AbstractClasses/AbstractCharacter.h"
#include "../Interfaces/AttackInterface.h"
#include "../GameplayTags.h"

#include "OrangeGirlCharacter.generated.h"

UENUM(BlueprintType)
enum class EOrangeGirlState : uint8
{
	Idle,
	ClutchStomach,
	BobUpDown,
	NeedSaps,
	GiveSapsPlz
};

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API AOrangeGirlCharacter : public ABaseCharacter,
	public IGameplayTagAssetInterface,
	public IAttackInterface
{
	GENERATED_BODY()
public:
	AOrangeGirlCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:	// Gameplay Tags
	FGameplayTagContainer GameplayTags;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const { TagContainer = GameplayTags; };

public: // AttackInterface
	virtual bool CanBeAttacked() override { return bAICanBeDamaged; }
	virtual void Gen_ReceiveAttack(const FVector Direction, const float Strength, const EAttackType AType, const float Delaytime = -1.f);

public: // Animations
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void Anim_SetIdle();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void Anim_ClutchStomach();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void Anim_BopUpDown();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void Anim_NeedSaps();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void Anim_GiveMeSapsPlz();

	UPROPERTY(BlueprintReadWrite)
		EOrangeGirlState m_EOrangeGirlState;
};
