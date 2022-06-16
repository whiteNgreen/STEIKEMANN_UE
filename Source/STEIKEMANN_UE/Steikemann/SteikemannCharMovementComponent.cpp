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
	FVector negativeVelocity = Velocity * -1.f;
	negativeVelocity.Normalize();
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (negativeVelocity * Velocity.Size()), FColor::Black, false, 10.f, 0, 4.f);

	FVector orthogonalVelocity = FVector::CrossProduct(negativeVelocity, FVector::CrossProduct(surfacenormal, negativeVelocity));
	orthogonalVelocity.Normalize();

	float dotproduct = FVector::DotProduct(negativeVelocity, surfacenormal);
	float angle = acosf(dotproduct);

	FVector newVelocity = (cos(angle * 2) * negativeVelocity) + (sin(angle * 2) * orthogonalVelocity);
	newVelocity *= Velocity.Size();
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (newVelocity), FColor::Purple, false, 10.f, 0, 4.f);
		

	Velocity = newVelocity;
}
