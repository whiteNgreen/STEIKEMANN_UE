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

		/* Jump */
		bJumping = SteikeOwner->IsJumping();

		// GroundPound
		bGroundPound = SteikeOwner->IsGroundPounding();

		/* Grappling */
		bGrappling = SteikeOwner->IsGrappling();
		GrappledTarget = SteikeOwner->GH_GetTargetLocation();
		//GrappledTarget = SteikeOwner->Active_GrappledActor_Location;
		bControlRigLerp = SteikeOwner->bGH_LerpControlRig;
		ControlRig_LeafAlpha = FMath::FInterpTo(ControlRig_LeafAlpha, (float)bControlRigLerp, DeltaSeconds, 33.f);

		/* Wall Sticking */
		bOnWall = SteikeOwner->Anim_IsOnWall();
		
		// Input angle on wall
		WallInputAngle = SteikeOwner->WallInputDirection;

		/* Ledge Grab */
		bLedgeGrab = SteikeOwner->IsLedgeGrabbing();


		// Booleans for blending various bodyparts
		bBlendLowerBody_Air = bFalling && (bGrappling || SteikeOwner->IsSmackAttacking());

	}
	else {
		SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
	}
}
