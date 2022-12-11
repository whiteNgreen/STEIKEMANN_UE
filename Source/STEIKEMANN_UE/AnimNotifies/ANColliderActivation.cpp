// Fill out your copyright notice in the Description page of Project Settings.


#include "../AnimNotifies/ANColliderActivation.h"


void UANAnimCancel::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
		owner->AllowActionCancelationWithInput();
}


void UANColliderActivation::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
		owner->Activate_AttackCollider();
}

void UANColliderDeactivation::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
		owner->Deactivate_AttackCollider();
}

void UANStartAttackBufferPeriod::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
		owner->StartAttackBufferPeriod();
}

void UANEndAttackBufferPeriod::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	if (owner)
		owner->EndAttackBufferPeriod();

}
