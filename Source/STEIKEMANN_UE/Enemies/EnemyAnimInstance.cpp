// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/EnemyAnimInstance.h"
#include "SmallEnemy.h"

void UEnemyAnimInstance::NativeBeginPlay()
{
	Super::NativeBeginPlay();
	Owner = Cast<ASmallEnemy>(TryGetPawnOwner());
	Owner->AnimInstance = this;
}

void UEnemyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!Owner) return;

	Velocity = Owner->GetVelocity();
	Speed = Velocity.Length();

	float dir = FVector::DotProduct(Velocity.GetSafeNormal(), Owner->GetActorForwardVector());
	if (bIsLaunchedInAir) {
		Launched_SpinAngle.Roll -= DeltaSeconds * (Launched_SpinSpeed * FMath::Sign(dir));
	}
}

void UEnemyAnimInstance::SetLaunchedInAir(FVector direction)
{
	bIsLaunchedInAir = true;
	Launched_SpinAngle = direction.Rotation();
	Launched_SpinAngle.Pitch = 0.f;
	Launched_SpinAngle.Yaw = Owner->GetActorRotation().Yaw + 85.f;

}
