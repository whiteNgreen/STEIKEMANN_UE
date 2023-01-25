// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikeAnimInstance.h"
#include "../Steikemann/SteikemannCharacter.h"

void USteikeAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();

	SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
}

void USteikeAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);


	//if (SteikeOwner.IsValid()) {
	if (SteikeOwner) {

		/* Set speed variables */
		Speed = SteikeOwner->GetVelocity().Size();
		VerticalSpeed = SteikeOwner->GetVelocity().Z;
		{
			FVector Vel = SteikeOwner->GetVelocity();
			Vel.Z = 0.f;
			HorizontalSpeed = Vel.Size();
		}

		/* In Air or on the Ground? */
		bFalling = SteikeOwner->IsFalling();
		bOnGround = SteikeOwner->IsOnGround();

		/* Crouch */
		bCrouch = SteikeOwner->bIsCrouchWalking;
		bAnimCrouchSliding = SteikeOwner->bCrouchSliding;

		/* Jump */
		bJumping = SteikeOwner->IsJumping();

		/* Dash */
		//bDashing = SteikeOwner->IsDashing();

		/* Grappling */
		bGrappling = SteikeOwner->IsGrappling();

		/* Wall Sticking */
		bOnWall = SteikeOwner->Anim_IsOnWall();
		
		// Input angle on wall
		//InputToActorForwardAngle // TODO: IMPLEMENT
		WallInputAngle = SteikeOwner->WallInputDirection;


		/* Ledge Grab */
		bLedgeGrab = SteikeOwner->IsLedgeGrabbing();
	}
	else {
		SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
	}
}

