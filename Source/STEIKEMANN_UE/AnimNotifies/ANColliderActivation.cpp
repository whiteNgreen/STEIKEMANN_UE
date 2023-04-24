// Fill out your copyright notice in the Description page of Project Settings.


#include "../AnimNotifies/ANColliderActivation.h"


// Animation Canceling
void UANAnimCancel::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::AllowActionCancelationWithInput);
}

// Attack Collider
void UANColliderActivation::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::Activate_AttackCollider);
}

void UANColliderDeactivation::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::Deactivate_AttackCollider);
}

// Buffer Attack
void UANStartAttackBufferPeriod::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::StartAttackBufferPeriod);
}

void UANExecuteAttackBuffer::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::ExecuteAttackBuffer);
}

void UANEndAttackBufferPeriod::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::EndAttackBufferPeriod);
}

void UANStartControlRigLerp::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	CallOwnerFunction(MeshComp, &AAbstractCharacter::StartAnimLerp_ControlRig);
}

void UANControlRigRaycast_Start::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
	{
		switch (IKRaycast_Type)
		{
		case EIKRaycast_Type::Foot_L:
			owner->bIK_Foot_L = true;
			break;
		case EIKRaycast_Type::Foot_R:
			owner->bIK_Foot_R = true;
			break;
		default:
			break;
		}
	}
}
void UANControlRigRaycast_End::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
	{
		switch (IKRaycast_Type)
		{
		case EIKRaycast_Type::Foot_L:
			owner->bIK_Foot_L = false;
			break;
		case EIKRaycast_Type::Foot_R:
			owner->bIK_Foot_R = false;
			break;
		default:
			break;
		}
	}
}