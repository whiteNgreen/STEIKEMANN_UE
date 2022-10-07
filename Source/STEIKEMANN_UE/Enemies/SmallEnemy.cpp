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

void ASmallEnemy::Do_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength)
{
}

void ASmallEnemy::Recieve_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength)
{
	if (GetCanBeSmackAttacked())
	{
		PRINTLONG("IM BEING ATTACKED");
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Direction * AttackStrength), FColor::Yellow, false, 2.f, 0, 3.f);

		bCanBeSmackAttacked = false;
		SetActorRotation(FVector(Direction.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
		GetCharacterMovement()->Velocity *= 0.f;
		GetCharacterMovement()->AddImpulse(Direction * AttackStrength, true);

		/* Sets a timer before character can be damaged by the same attack */
		GetWorldTimerManager().SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, 0.5f, false);
	}
}
