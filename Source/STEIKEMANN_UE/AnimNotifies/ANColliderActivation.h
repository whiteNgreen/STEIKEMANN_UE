// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "../DebugMacros.h"
#include "../AbstractClasses/AbstractCharacter.h"
#include "ANColliderActivation.generated.h"

UENUM()
enum class EActorType : int8
{
	Player,
	AubergineDoggo
};

DECLARE_DELEGATE(AnimDelegate);

inline void CallOwnerFunction(USkeletalMeshComponent* MeshComp, void(AAbstractCharacter::* func)())
{
	AAbstractCharacter* owner = Cast<AAbstractCharacter>(MeshComp->GetOwner());
	AnimDelegate del;
	if (owner)
	{
		del.BindUObject(owner, func);
		del.ExecuteIfBound();
	}
}
															

// ----------------------------------------------------- ANIM CANCEL -----------------------------------------------------------------
UCLASS()
class STEIKEMANN_UE_API UANAnimCancel : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
		EActorType m_ActorType;
};

// ----------------------------------------------------- COLLIDER -----------------------------------------------------------------
UCLASS()
class STEIKEMANN_UE_API UANColliderActivation : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
		EActorType m_ActorType;
};

UCLASS()
class STEIKEMANN_UE_API UANColliderDeactivation : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
		EActorType m_ActorType;
};


// ----------------------------------------------------- ATTACK BUFFER -----------------------------------------------------------------
UCLASS()
class STEIKEMANN_UE_API UANStartAttackBufferPeriod : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
		EActorType m_ActorType;
};

UCLASS()
class STEIKEMANN_UE_API UANExecuteAttackBuffer : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
		EActorType m_ActorType;
};

UCLASS()
class STEIKEMANN_UE_API UANEndAttackBufferPeriod : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;

	UPROPERTY(EditAnywhere)
		EActorType m_ActorType;
};

UCLASS()
class STEIKEMANN_UE_API UANStartControlRigLerp : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(class USkeletalMeshComponent* MeshComp, class UAnimSequenceBase* Animation) override;
};