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

	/* Jump velocity */
	if (bJumping && CharacterOwner_Steikemann->bJumping)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Magenta, FString::Printf(TEXT("Jumping")));
		Velocity.Z += 100;
	}
}

bool USteikemannCharMovementComponent::DoJump(bool bReplayingMoves)
{
	//GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Black, FString::Printf(TEXT("DoJump Check")));
	//Super::DoJump(bReplayingMoves);


	if (CharacterOwner_Steikemann && (CharacterOwner_Steikemann->CanJump() || CharacterOwner_Steikemann->CanDoubleJump()))
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			//Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity);
			////AddImpulse(FVector(0, 0, 200), true);
			//SetMovementMode(MOVE_Falling);


			bJumping = true;
			return true;
		}
	}



	return false;
}
