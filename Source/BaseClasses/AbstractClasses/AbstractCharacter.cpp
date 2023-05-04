// Fill out your copyright notice in the Description page of Project Settings.


#include "AbstractCharacter.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraComponent.h"
//#include "../DebugMacros.h"
// Base components
#include "../StaticVariables.h"

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

	const auto i = GetCharacterMovement();
	m_GravityScale = i->GravityScale;
	m_BaseGravityZ = i->GetGravityZ();
}

void ABaseCharacter::BeginDestroy()
{
	Cancel_AttackContact();
	Super::BeginDestroy();
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimerManager.Tick(DeltaTime);
	if (Delegate_NextFrameDelegate.IsBound()) {
		Delegate_NextFrameDelegate.Broadcast();
		Delegate_NextFrameDelegate.Clear();
	}
}

void ABaseCharacter::EndTick(float DeltaTime)
{
	Delegate_ParticleUpdate.Broadcast(DeltaTime);
	Delegate_MaterialUpdate.Broadcast(DeltaTime);
}
void ABaseCharacter::EnableCollisions()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}
void ABaseCharacter::DisableCollisions()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

	if (GetWorldTimerManager().IsTimerActive(TH_AttackContact_Instigator)) 
		return;
	target->CustomTimeDilation = Statics::AttackContactTimeDilation;
	GetWorldTimerManager().SetTimer(TH_AttackContact_Instigator, [this]() {		// Using world timer manager for timers meant to run in realtime
		if (this)
			this->CustomTimeDilation = 1.f;
		}, AttackContactTimer, false);
}

void ABaseCharacter::Cancel_AttackContact()
{
	if (GetWorld())
		GetWorldTimerManager().ClearTimer(TH_AttackContact_Instigator);
}

bool ABaseCharacter::ShroomBounce(FVector direction, float strength)
{
	if (!bCanBounce) return false;
	auto c = GetCharacterMovement();
	c->Velocity *= 0.f;
	c->AddImpulse(direction * strength, true);

	bCanBounce = false;
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { bCanBounce = true; }, 0.1f, false);
	return true;
}

void ABaseCharacter::Landed(const FHitResult& Hit)
{
	LandVelocity = GetCharacterMovement()->Velocity;
}

bool ABaseCharacter::CheckStaticWorldBeneathCharacter(float LengthBeneath) const
{
	FHitResult Hit;
	FCollisionQueryParams Params("", false, this);
	FCollisionShape capsule = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius() * 0.95f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.95f);
	FVector below(0, 0, LengthBeneath);
	FVector location = GetActorLocation() - below;

	const bool b  = GetWorld()->SweepSingleByChannel(Hit, location, location, FQuat(1, 0, 0, 0), ECC_StaticWorldChannel, capsule, Params);
	return b;
}


