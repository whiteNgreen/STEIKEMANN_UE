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

void ASmallEnemy::TargetedPure()
{
	PRINTPAR("I; %s, am grapple targeted", *GetName());
	Targeted();
}

void ASmallEnemy::UnTargetedPure()
{
	UnTargeted();
}

void ASmallEnemy::HookedPure()
{
	Hooked();
}

void ASmallEnemy::HookedPure(const FVector InstigatorLocation)
{
	/* 1st method. Static launch strength and angle */
	if (bUseFirstGrappleLaunchMethod)
	{
		FVector Direction3D = InstigatorLocation - GetActorLocation();

		float angle = FMath::DegreesToRadians(GrappledLaunchAngle);
		FVector LaunchDirection = (cosf(angle) * Direction3D.GetSafeNormal2D()) + (sinf(angle) * FVector::UpVector);
		PRINTPARLONG("LaunchDirection LENGTH = %f", LaunchDirection.Size());

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (LaunchDirection * GrappledLaunchStrength), FColor::Red, false, 1.f, 0, 4.f);
		GetCharacterMovement()->AddImpulse(LaunchDirection * GrappledLaunchStrength, true);
	}
	/* 2nd method */
	else
	{
		PRINTLONG("Second Method Launch");
		FVector Direction3D = InstigatorLocation - GetActorLocation();
		FVector Direction2D = Direction3D; 
		Direction3D.Z = 0.f; 
		//Direction2D.Normalize();

		FVector Velocity = Direction2D / GrappledLaunchTime;

		Velocity.Z = 0.5f * GetCharacterMovement()->GetGravityZ() * GrappledLaunchTime * -1.f;

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Velocity, FColor::Red, false, 1.f, 1, 4.f);
		GetCharacterMovement()->AddImpulse(Velocity, true);
	}
}

void ASmallEnemy::UnHookedPure()
{
	UnHooked();
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
		PRINTLONG("IM BEING ATTACKED");
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Direction * Strength), FColor::Yellow, false, 2.f, 0, 3.f);

		bCanBeSmackAttacked = false;
		SetActorRotation(FVector(Direction.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
		GetCharacterMovement()->Velocity *= 0.f;
		GetCharacterMovement()->AddImpulse(Direction * Strength, true);

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
		PRINTLONG("IM BEING ATTACKED");
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Direction * Strength), FColor::Yellow, false, 2.f, 0, 3.f);

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
	PRINTLONG("IM BEING GROUNDPOUNDED");
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (PoundDirection * GP_Strength), FColor::Yellow, false, 2.f, 0, 3.f);

	//bCanBeSmackAttacked = false;
	SetActorRotation(FVector(PoundDirection.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(PoundDirection * GP_Strength, true);

	/* Sets a timer before character can be damaged by the same attack */
	//GetWorldTimerManager().SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, 0.5f, false);
}
