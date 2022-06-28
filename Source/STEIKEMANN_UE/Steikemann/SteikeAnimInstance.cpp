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
		/* Set speed variables */
		Speed = SteikeOwner->GetVelocity().Size();
		VerticalSpeed = SteikeOwner->GetVelocity().Z;

		/* Is Character freefalling in air or on the ground? */
		bFalling = SteikeOwner->IsFalling();
		bOnGround = SteikeOwner->IsOnGround();
		bOnGround ? PRINT("On Ground") : PRINT("In Air");

		/* Jump */
		bActivateJump = SteikeOwner->IsJumping();
		bPressedJump = SteikeOwner->bPressedJump;
		bActivateJump ? PRINT("JUMPING") : PRINT("NOT jumping");

		//bPressedJump = SteikeOwner->bJumping;
		//bPressedJump = SteikeOwner->bAddJumpVelocity;
		//bPressedJump = SteikeOwner->bPressedJump && SteikeOwner->IsJumping();

		//if (bActivateJump) {
		//	SteikeOwner->bActivateJump = true;
		//	//bActivateJump = false;
		//	//PRINTLONG("Anim Script: Activate Jump");
		//	static float Timer{};
		//	PRINTPAR("Timer: %f", Timer);
		//	Timer += DeltaSeconds;
		//	if (Timer >= 0.5f) {
		//		Timer = 0.f;
		//		bActivateJump = false;
		//	}
		//}
	}
	else {
		SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
	}
}
