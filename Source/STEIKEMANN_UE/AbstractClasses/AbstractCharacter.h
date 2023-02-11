// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

// Particles
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "AbstractCharacter.generated.h"

UCLASS(Abstract)
class STEIKEMANN_UE_API AAbstractCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	virtual void AllowActionCancelationWithInput(){}

	virtual void Activate_AttackCollider	(){}
	virtual void Deactivate_AttackCollider	(){}

	virtual void StartAttackBufferPeriod	(){}
	virtual void ExecuteAttackBuffer		(){}
	virtual void EndAttackBufferPeriod		(){}

	virtual void StartAnimLerp_ControlRig	(){}
};


DECLARE_MULTICAST_DELEGATE_OneParam(FParticleUpdate, float /* DeltaTime */)
DECLARE_MULTICAST_DELEGATE_OneParam(FMaterialUpdate, float /* DeltaTime */)
class UNiagaraSystem;
class UNiagaraComponent;

UCLASS(Abstract)
class STEIKEMANN_UE_API ABaseCharacter : public AAbstractCharacter
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	ABaseCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaTime) override;
	virtual void EndTick(float DeltaTime);

	FTimerManager TimerManager;
	FParticleUpdate Delegate_ParticleUpdate;
	FMaterialUpdate Delegate_MaterialUpdate;

	/* Temporary niagara components created when main component is busy */
	TArray<UNiagaraComponent*> TempNiagaraComponents;
	/* Create tmp niagara component */
	UNiagaraComponent* CreateNiagaraComponent(FName Name, USceneComponent* Parent = nullptr, FAttachmentTransformRules AttachmentRule = FAttachmentTransformRules::SnapToTargetIncludingScale, bool bTemp = false);

};