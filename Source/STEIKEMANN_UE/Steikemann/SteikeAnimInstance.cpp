// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikeAnimInstance.h"
#include "../Steikemann/SteikemannCharacter.h"

void USteikeAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());

	/* Assign this animinstance the owners animinstance ptr */
	if (SteikeOwner) {
		SteikeOwner->AssignAnimInstance(this);
	}
}

void USteikeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);


	//if (SteikeOwner.IsValid()) {
	if (SteikeOwner) {

		/* Assign this animinstance the owners animinstance ptr */
		if (!SteikeOwner->GetAnimInstance()) { 
			SteikeOwner->AssignAnimInstance(this); 
		}

		/* Set speed variables */
		Speed = SteikeOwner->GetVelocity().Size();
		VerticalSpeed = SteikeOwner->GetVelocity().Z;
		{
			FVector Vel = SteikeOwner->GetVelocity();
			Vel.Z = 0.f;
			HorizontalSpeed = Vel.Size();
		}

		/* Is Character freefalling in air or on the ground? */
		if (!bActivateJump) {
			bFalling = SteikeOwner->IsFalling();

		}
		bOnGround = SteikeOwner->IsOnGround();
		PRINTPAR("AnimInstance Falling: %s", bFalling ? (TEXT("True")) : (TEXT("False")));

		/* Jump */
		if (bActivateJump) {
			static float Timer{};
			Timer += DeltaSeconds;
			if (Timer >= 0.15f) {
				Timer = 0.f;
				bActivateJump = false;
			}
			bFalling = false;
		}

		/* Dash */
		bDashing = SteikeOwner->IsDashing();

		/* Grappling */
		bGrappling = SteikeOwner->IsGrappling();
	}
	else {
		SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
	}
}

void USteikeAnimInstance::ActivateJump()
{
	PRINTLONG("Animinstance JUMP");

	bActivateJump = true;

}
