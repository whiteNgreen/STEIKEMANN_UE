// Fill out your copyright notice in the Description page of Project Settings.


#include "../AbstractClasses/AbstractCharacter.h"

ABaseCharacter::ABaseCharacter()
{
}

ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimerManager.Tick(DeltaTime);
}

void ABaseCharacter::EndTick(float DeltaTime)
{
	Delegate_ParticleUpdate.Broadcast(DeltaTime);
	Delegate_MaterialUpdate.Broadcast(DeltaTime);
}

UNiagaraComponent* ABaseCharacter::CreateNiagaraComponent(FName Name, USceneComponent* Parent, FAttachmentTransformRules AttachmentRule, bool bTemp)
{
	UNiagaraComponent* TempNiagaraComp = NewObject<UNiagaraComponent>(this, Name);
	if (Parent)
		TempNiagaraComp->AttachToComponent(Parent, AttachmentRule);
	TempNiagaraComp->RegisterComponent();

	if (bTemp) { TempNiagaraComponents.Add(TempNiagaraComp); } // Adding as temp comp

	return TempNiagaraComp;
}
