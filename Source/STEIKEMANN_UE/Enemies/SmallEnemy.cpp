// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/SmallEnemy.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Gameframework/CharacterMovementComponent.h"

// Sets default values
ASmallEnemy::ASmallEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;



}

// Called when the game starts or when spawned
void ASmallEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	/*
	* Adding GameplayTags to the GameplayTagsContainer
	*/
	Enemy = Tag_EnemyAubergineDoggo;
	GameplayTags.AddTag(Enemy);
}

// Called every frame
void ASmallEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASmallEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ASmallEnemy::RotateActorYawToVector(FVector AimVector, float DeltaTime)
{
	FVector Aim = AimVector;
	Aim.Normalize();

	FVector AimXY = Aim;
	AimXY.Z = 0.f;
	AimXY.Normalize();

	float YawDotProduct = FVector::DotProduct(AimXY, FVector::ForwardVector);
	float Yaw = FMath::RadiansToDegrees(acosf(YawDotProduct));

	/*		Check if yaw is to the right or left		*/
	float RightDotProduct = FVector::DotProduct(AimXY, FVector::RightVector);
	if (RightDotProduct < 0.f) { Yaw *= -1.f; }

	SetActorRotation(FRotator(GetActorRotation().Pitch, Yaw, 0.f), ETeleportType::TeleportPhysics);
}

void ASmallEnemy::TargetedPure()
{
	//PRINTPAR("I; %s, am grapple targeted", *GetName());
	Execute_Targeted(this);
}

void ASmallEnemy::UnTargetedPure()
{
	Execute_UnTargeted(this);
}

void ASmallEnemy::InReach_Pure()
{
	Execute_InReach(this);
}

void ASmallEnemy::OutofReach_Pure()
{
	Execute_OutofReach(this);
}

void ASmallEnemy::HookedPure()
{
	Execute_Hooked(this);
}

void ASmallEnemy::HookedPure(const FVector InstigatorLocation, bool PreAction /*=false*/)
{
	/* During Pre Action, Rotate Actor towards instigator - Yaw */
	if (PreAction)
	{
		FVector Direction = InstigatorLocation - GetActorLocation();
		RotateActorYawToVector(Direction.GetSafeNormal());
		return;
	}

	if (bCanBeGrappleHooked)
	{
		GetCharacterMovement()->Velocity *= 0.f;

		/* Rotate again towards Instigator - Yaw*/
		FVector Direction3D = InstigatorLocation - GetActorLocation();
		RotateActorYawToVector(Direction3D.GetSafeNormal());

		/* 1st method. Static launch strength and angle */
		if (bUseFirstGrappleLaunchMethod)
		{
			float angle = FMath::DegreesToRadians(GrappledLaunchAngle);
			FVector LaunchDirection = (cosf(angle) * Direction3D.GetSafeNormal2D()) + (sinf(angle) * FVector::UpVector);

			GetCharacterMovement()->AddImpulse(LaunchDirection * GrappledLaunchStrength, true);
		}
		/* 2nd method */
		else
		{
			//PRINTLONG("Second Method Launch");
			//FVector Direction3D = InstigatorLocation - GetActorLocation();
			FVector Direction2D = Direction3D;
			Direction3D.Z = 0.f;
			//Direction2D.Normalize();

			FVector Velocity = Direction2D / GrappledLaunchTime;

			Velocity.Z = 0.5f * GetCharacterMovement()->GetGravityZ() * GrappledLaunchTime * -1.f;

			GetCharacterMovement()->AddImpulse(Velocity, true);
		}

		bCanBeGrappleHooked = false;
		GetWorldTimerManager().SetTimer(Handle_GrappledCooldown, this, &ASmallEnemy::ResetCanBeGrappleHooked, GrappleHookedInternalCooldown);
	}
}

void ASmallEnemy::UnHookedPure()
{
	Execute_UnHooked(this);
}

void ASmallEnemy::CanBeAttacked()
{
}

void ASmallEnemy::Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
}

void ASmallEnemy::Receive_SmackAttack_Pure(const FVector& Direction, const float& Strength)
{
	if (GetCanBeSmackAttacked())
	{
		float s;
		/* Weaker smack attack if actor on ground than in air */
		GetMovementComponent()->IsFalling() ? s = Strength : s = Strength * SmackAttack_OnGroundMultiplication;

		bCanBeSmackAttacked = false;
		SetActorRotation(FVector(Direction.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
		GetCharacterMovement()->Velocity *= 0.f;
		GetCharacterMovement()->AddImpulse(Direction * s, true);

		/* Sets a timer before character can be damaged by the same attack */
		GetWorldTimerManager().SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, 0.5f, false);
	}
}

void ASmallEnemy::Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
}

void ASmallEnemy::Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength)
{
	if (GetCanBeSmackAttacked())
	{
		//PRINTPARLONG("IM(%s) BEING ATTACKED", *GetName());
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Direction * Strength), FColor::Yellow, false, 2.f, 0, 3.f);

		bCanBeSmackAttacked = false;
		SetActorRotation(FVector(Direction.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
		GetCharacterMovement()->Velocity *= 0.f;
		GetCharacterMovement()->AddImpulse(Direction * Strength, true);

		/* Sets a timer before character can be damaged by the same attack */
		GetWorldTimerManager().SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, 0.2f, false);
	}
}

void ASmallEnemy::Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength)
{
	//PRINTPARLONG("IM(%s) BEING GROUNDPOUNDED", *GetName());
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (PoundDirection * GP_Strength), FColor::Yellow, false, 2.f, 0, 3.f);

	//bCanBeSmackAttacked = false;
	SetActorRotation(FVector(PoundDirection.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(PoundDirection * GP_Strength, true);

	/* Sets a timer before character can be damaged by the same attack */
	//GetWorldTimerManager().SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, 0.5f, false);
}
