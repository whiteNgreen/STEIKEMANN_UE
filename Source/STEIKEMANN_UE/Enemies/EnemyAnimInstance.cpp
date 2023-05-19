// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/EnemyAnimInstance.h"
#include "SmallEnemy.h"
#include "../DebugMacros.h"

void UEnemyAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	Owner = Cast<ASmallEnemy>(TryGetPawnOwner());
	Owner->m_Anim = this;
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!Owner) return;
	Speed = Owner->GetVelocity().Length();
	if (bIsLaunchedInAir) {
		Launched_SpinAngle.Roll -= DeltaSeconds * (Launched_SpinSpeed * -1.f);
	}
}

void UEnemyAnimInstance::SetLaunchedInAir(FVector direction)
{
	bIsLaunchedInAir = true;
	m_AnimState = EEnemyAnimState::Launched;
	if (FVector::DotProduct(direction, FVector::UpVector) < 0.97f)
		Launched_SpinAngle = FVector(FVector::UpVector + FVector::ForwardVector * 0.4f).Rotation();
	else if (FVector::DotProduct(direction, FVector::DownVector) < 0.97f)
		Launched_SpinAngle = FVector(FVector::DownVector + FVector::ForwardVector * 0.4f).Rotation();
	else {
		Launched_SpinAngle = direction.Rotation();
	}
	Launched_SpinAngle.Pitch = 0.f;
	Launched_SpinAngle.Yaw = Owner->GetActorRotation().Yaw + 85.f;
}
