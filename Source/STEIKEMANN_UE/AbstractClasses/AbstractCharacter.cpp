// Fill out your copyright notice in the Description page of Project Settings.


#include "../AbstractClasses/AbstractCharacter.h"
#include "../DebugMacros.h"

ABaseCharacter::ABaseCharacter()
{
}

ABaseCharacter::ABaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	AttackContactDelegate.AddUObject(this, &ABaseCharacter::AttackContact);
	//AttackContactDelegate_Instigator.AddUObject(this, &ABaseCharacter::AttackContact_Instigator);
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


void ABaseCharacter::AttackContact(AActor* target)
{
	// Cancel function if target has already been hit
	if (AttackContactedActors.Find(target) == 0) {
		return;
	}
	AttackContactedActors.Add(target);

	// Do Whatever needs to be done to the actor here	// Like f.ex starting an animation before starting the time dilation
	this->CustomTimeDilation = Statics::AttackContactTimeDilation;

	FTimerHandle h;
	GetWorldTimerManager().SetTimer(h, [target]() {		// Using world timer manager for timers meant to run in realtime
		target->CustomTimeDilation = 1.f;
		}, AttackContactTimer, false);

	PRINTLONG("ATTACK CONTACT");

	if (GetWorldTimerManager().IsTimerActive(TH_AttackContact_Instigator)) 
		return;
	target->CustomTimeDilation = Statics::AttackContactTimeDilation;
	GetWorldTimerManager().SetTimer(TH_AttackContact_Instigator, [this]() {		// Using world timer manager for timers meant to run in realtime
		this->CustomTimeDilation = 1.f;
		}, AttackContactTimer, false);
}
