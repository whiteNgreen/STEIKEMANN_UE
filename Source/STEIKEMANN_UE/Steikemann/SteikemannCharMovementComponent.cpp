// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "Engine.h"

USteikemannCharMovementComponent::USteikemannCharMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USteikemannCharMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner_Steikemann = Cast<ASteikemannCharacter>(GetCharacterOwner());

}

void USteikemannCharMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* Friction */
	{
		GroundFriction = CharacterFriction * Traced_GroundFriction;
	}

	/* Gravity */
	if (CharacterOwner_Steikemann->IsJumping() || MovementMode == MOVE_Walking)
	{
		GravityScale = FMath::FInterpTo(GravityScale, 1.f, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}
	else
	{
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}


	/* Jump velocity */
	if (CharacterOwner_Steikemann->IsJumping())
	{
		Velocity.Z = FMath::FInterpTo(Velocity.Z, JumpZVelocity, GetWorld()->GetDeltaSeconds(), JumpInterpSpeed);
		SetMovementMode(MOVE_Falling);
	}

	PRINTPAR("Speed: %f", Velocity.Size());

}


bool USteikemannCharMovementComponent::DoJump(bool bReplayingMoves)
{
	if (CharacterOwner_Steikemann && CharacterOwner_Steikemann->bCanEdgeJump)
	{
		return true;
	}

	if (CharacterOwner_Steikemann && (CharacterOwner_Steikemann->CanJump() || CharacterOwner_Steikemann->CanDoubleJump()))
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			return true;
		}
	}


	return false;
}

void USteikemannCharMovementComponent::Bounce(FVector surfacenormal)
{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (surfacenormal * 200), FColor::Blue, false, 10.f, 0, 4.f);
	FVector negVel = Velocity * -1.f;
	FVector curVel = Velocity;
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Velocity * 200), FColor::Black, false, 10.f, 0, 4.f);
	float expo = FVector::DotProduct(curVel, surfacenormal);
	float tran = curVel.Size() * surfacenormal.Size();
	float angle = acosf(expo / tran) * (180 / PI);

	//FVector orthoVel = FVector::CrossProduct(curVel, FVector::CrossProduct(surfacenormal, curVel));
	FVector orthoVel = FVector::CrossProduct(Velocity, FVector::CrossProduct(surfacenormal, Velocity));
	orthoVel.Normalize();
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (orthoVel * 200), FColor::Yellow, false, 10.f, 0, 4.f);
	//orthoVel *= Velocity.Size();
	//PRINTPARLONG("OrthoVel: %f", )

	curVel.Normalize();
	FVector newVel = (cosf(-angle) * curVel) + (sinf(-angle) * orthoVel);
	newVel.Normalize();
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (newVel * 200), FColor::Purple, false, 10.f, 0, 4.f);
	newVel *= Velocity.Size();
	Velocity = newVel;
}
