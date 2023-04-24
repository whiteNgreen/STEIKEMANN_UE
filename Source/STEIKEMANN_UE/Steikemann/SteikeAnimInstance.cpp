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

		/* Foot IK Surface Placement */
		float IK_RaycastLength{ 100.f };
		//bIK_Foot_L_Active = SteikeOwner->bIK_Foot_L;
		//bIK_Foot_R_Active = SteikeOwner->bIK_Foot_R;
		//bIK_Foot_L_Active ? PRINT("IK Left  Active") : PRINT("IK Left  Inactive");
		//bIK_Foot_R_Active ? PRINT("IK Right Active") : PRINT("IK Right Inactive");

		//Hip_BaseTransform = SteikeOwner->GetMesh()->GetSocketTransform("Hip_Socket");
		//FTransform FootSocket_L = SteikeOwner->GetMesh()->GetSocketTransform("Foot_Socket_L");
		//IKFoot_BaseTransform_L = FootSocket_L;
		//FTransform FootSocket_R = SteikeOwner->GetMesh()->GetSocketTransform("Foot_Socket_R");
		//IKFoot_BaseTransform_R = FootSocket_R;
		//return;


		//if (bIK_Foot_L_Active)
		//{
		//	FIK_RaycastReturn RR = SteikeOwner->RaycastForIKPlacement(FName("Foot_Socket_L"), IK_RaycastLength);
		//	bIK_Foot_L_Active = RR.bHitSurface;
		//	if (RR.bHitSurface) {
		//		IK_Foot_L_SurfaceLocation = FootSocket_L.GetLocation();
		//		IK_Foot_L_SurfaceLocation.Z = RR.SurfaceLocation.Z;
		//		IK_Foot_L_SurfaceDirection = RR.SurfaceNormal;
		//
		//		IK_Foot_L_NewRotation = GetNewFootIKDirection(SteikeOwner->GetActorForwardVector(), RR.SurfaceNormal);
		//	}
		//}
		//if (bIK_Foot_R_Active) 
		//{
		//	FIK_RaycastReturn RR = SteikeOwner->RaycastForIKPlacement(FName("Foot_Socket_R"), IK_RaycastLength);
		//	bIK_Foot_R_Active = RR.bHitSurface;
		//	if (RR.bHitSurface) {
		//		IK_Foot_R_SurfaceLocation = FootSocket_R.GetLocation();
		//		IK_Foot_R_SurfaceLocation.Z = RR.SurfaceLocation.Z;
		//		//IK_Foot_R_SurfaceLocation = RR.SurfaceLocation;
		//		IK_Foot_R_SurfaceDirection = RR.SurfaceNormal;
		//
		//		//IK_Foot_R_NewRotation = GetNewFootIKDirection(FootSocket_R.Rotator().Vector(), RR.SurfaceNormal);
		//		IK_Foot_R_NewRotation = GetNewFootIKDirection(SteikeOwner->GetActorForwardVector(), RR.SurfaceNormal);
		//	}
		//}
	}
	else {
		SteikeOwner = Cast<ASteikemannCharacter>(TryGetPawnOwner());
	}
}

FQuat USteikeAnimInstance::GetNewFootIKDirection(const FVector CurrentDirection, const FVector SurfaceDirection)
{
	FVector Ortho = FVector::CrossProduct(SurfaceDirection, FVector::CrossProduct(CurrentDirection, SurfaceDirection));
	return Ortho.Rotation().Quaternion();
}

