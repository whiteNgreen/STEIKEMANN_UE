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


	if (SteikeOwner.IsValid()) {
	//if (SteikeOwner) {

		/* Set speed variables */
		Speed = SteikeOwner->GetVelocity().Size();
		VerticalSpeed = SteikeOwner->GetVelocity().Z;
		{
			FVector Vel = SteikeOwner->GetVelocity();
			Vel.Z = 0.f;
			HorizontalSpeed = Vel.Size();
		}

		/* Is Character freefalling in air or on the ground? */
		bFalling = SteikeOwner->IsFalling();
		bOnGround = SteikeOwner->IsOnGround();

		/* Jump */
		bJumping = SteikeOwner->IsJumping();

		/* Dash */
		bDashing = SteikeOwner->IsDashing();

		/* Grappling */
		bGrappling = SteikeOwner->IsGrappling();

		/* Wall Sticking */
		bOnWall = SteikeOwner->IsOnWall();
		bStickingToWall = SteikeOwner->IsStickingToWall();
		//OnWallRotation = SteikeOwner->InputAngleToForward * -1.f;
		OnWallRotation = FMath::FInterpTo(OnWallRotation,FMath::Clamp(SteikeOwner->InputAngleToForward * -1.f, -90.f, 90.f), DeltaSeconds, SteikeOwner->OnWall_InterpolationSpeed);
		//PRINTPAR("OnWallRotation = %f", OnWallRotation);

		/* Ledge Grab */
		bLedgeGrab = SteikeOwner->IsLedgeGrabbing();
		bLedgeGrab ? PRINT("anim bLedgeGrab = true") : PRINT("anim bLedgeGrab = true");
	}
	else {
		SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
	}
}
