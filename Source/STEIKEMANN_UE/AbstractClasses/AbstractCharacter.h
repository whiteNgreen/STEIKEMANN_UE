// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../StaticVariables.h"

// Particles
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "../Delegates_Shared.h"

#include "AbstractCharacter.generated.h"

UCLASS(Abstract)
class STEIKEMANN_UE_API AAbstractCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	virtual void AllowActionCancelationWithInput()	PURE_VIRTUAL(AAbstractCharacter::AllowActionCancelationWithInput);

	virtual void Activate_AttackCollider()			PURE_VIRTUAL(AAbstractCharacter::Activate_AttackCollider);
	virtual void Deactivate_AttackCollider()		PURE_VIRTUAL(AAbstractCharacter::Deactivate_AttackCollider);

	virtual void StartAttackBufferPeriod()			PURE_VIRTUAL(AAbstractCharacter::StartAttackBufferPeriod);
	virtual void ExecuteAttackBuffer()				PURE_VIRTUAL(AAbstractCharacter::ExecuteAttackBuffer);
	virtual void EndAttackBufferPeriod()			PURE_VIRTUAL(AAbstractCharacter::EndAttackBufferPeriod);

	virtual void StartAnimLerp_ControlRig()			PURE_VIRTUAL(AAbstractCharacter::StartAnimLerp_ControlRig);
};


DECLARE_MULTICAST_DELEGATE_OneParam(FParticleUpdate, float DeltaTime)
DECLARE_MULTICAST_DELEGATE_OneParam(FMaterialUpdate, float DeltaTime)
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS(Abstract)
class STEIKEMANN_UE_API ABaseCharacter : public AAbstractCharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	ABaseCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndTick(float DeltaTime);

	FTimerManager TimerManager;
	FParticleUpdate Delegate_ParticleUpdate;
	FMaterialUpdate Delegate_MaterialUpdate;

	/* Temporary niagara components created when main component is busy */
	TArray<UNiagaraComponent*> TempNiagaraComponents;
	/* Create tmp niagara component */
	UNiagaraComponent* CreateNiagaraComponent(FName Name, USceneComponent* Parent = nullptr, FAttachmentTransformRules AttachmentRule = FAttachmentTransformRules::SnapToTargetIncludingScale, bool bTemp = false);


	// Attack Contact
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|AttackContact")
		float AttackContactTimer{ 0.3f };
	FAttackContact_Other_Delegate AttackContactDelegate;
	FTimerHandle TH_AttackContact_Instigator;

	/*	*	When attacking with the staff keep a list of actors hit during the attack
		*	The 'void Attack Contact Function' will only be called once per actor
		*	This array is cleaned in 'void StopAttack'								*/

	TArray<AActor*> AttackContactedActors;
	virtual void AttackContact(AActor* target);

	bool bCanBounce{ true };
	virtual bool ShroomBounce(FVector direction, float strength);

	FVector LandVelocity{};
	virtual void Landed(const FHitResult& Hit) override;

};