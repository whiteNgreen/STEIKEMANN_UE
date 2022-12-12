// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/SmallEnemy.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Gameframework/CharacterMovementComponent.h"
#include "../GameplayTags.h"

// Sets default values
ASmallEnemy::ASmallEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WallDetector = CreateDefaultSubobject<UWallDetectionComponent>(TEXT("Wall Detection Component"));

}

// Called when the game starts or when spawned
void ASmallEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	WallDetector->SetCapsuleSize(WDC_Capsule_Radius, WDC_Capsule_Halfheight);
	WallDetector->SetHeight(WDC_MinHeight, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	WallDetector->SetDebugStatus(bWDC_Debug);

	// Adding gameplay tags
	GameplayTags.AddTag(Tag::AubergineDoggo());

	auto i = Cast<UCharacterMovementComponent>(GetMovementComponent());
	GravityScale = i->GravityScale;
}

// Called every frame
void ASmallEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	SetDefaultState();

	const bool wall = WallDetector->DetectStickyWall(this, GetActorLocation(), m_WallData);
	if (wall && m_State == EEnemyState::STATE_InAir)
	{
		m_State = EEnemyState::STATE_OnWall;
		m_WallState = EWall::WALL_Stuck;
	}

	switch (m_State)
	{
	case EEnemyState::STATE_None:
		break;
	case EEnemyState::STATE_OnGround:
		break;
	case EEnemyState::STATE_InAir:
		break;
	case EEnemyState::STATE_OnWall:
		StickToWall();
		break;
	default:
		break;
	}

	auto i = Cast<UCharacterMovementComponent>(GetMovementComponent());
	switch (m_Gravity)
	{
	case EGravityState::Default:
		i->GravityScale = GravityScale;
		break;
	case EGravityState::LerpToDefault:
		break;
	case EGravityState::None:
		break;
	case EGravityState::LerpToNone:
		break;
	case EGravityState::ForcedNone:
		i->GravityScale = 0.f;
		i->Velocity *= 0.f;
		break;
	default:
		break;
	}
}

// Called to bind functionality to input
void ASmallEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void ASmallEnemy::SetDefaultState()
{
	switch (m_State)
	{
	case EEnemyState::STATE_None:
		break;
	case EEnemyState::STATE_OnGround:
		break;
	case EEnemyState::STATE_InAir:
		break;
	case EEnemyState::STATE_OnWall:
		return;
	default:
		break;
	}

	//auto i = Cast<UCharacterMovementComponent>(GetMovementComponent());
	auto i = GetMovementComponent();
	if (i->IsFalling()) {
		m_State = EEnemyState::STATE_InAir;
		return;
	}
	// Default Case
	m_State = EEnemyState::STATE_OnGround;
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

void ASmallEnemy::StickToWall()
{
	PRINTPAR("STUCK TO WALL: %s", *GetName());
	auto i = Cast<UCharacterMovementComponent>(GetMovementComponent());
	i->Velocity *= 0.f;
	i->GravityScale = 0;
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

void ASmallEnemy::HookedPure(const FVector InstigatorLocation, bool OnGround, bool PreAction /*=false*/)
{
	/* During Pre Action, Rotate Actor towards instigator - Yaw */
	if (PreAction)
	{
		FVector Direction = InstigatorLocation - GetActorLocation();
		RotateActorYawToVector(Direction.GetSafeNormal());
		m_Gravity = EGravityState::ForcedNone;
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
			m_Gravity = EGravityState::Default;
			GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			FVector Direction2D = Direction3D;
			Direction3D.Z = 0.f;

			FVector Velocity = Direction2D / GrappledLaunchTime;

			//Velocity.Z = 0.5f * GetCharacterMovement()->GetGravityZ() * GrappledLaunchTime * -1.f;
			float z{};
			OnGround ? z = (InstigatorLocation.Z - GetActorLocation().Z) : z = ((InstigatorLocation.Z + GrappledZInstigator) - GetActorLocation().Z);

			Velocity.Z = (z / GrappledLaunchTime) + (0.5 * GetCharacterMovement()->GetGravityZ() * GrappledLaunchTime * -1.f);
			GetCharacterMovement()->AddImpulse(Velocity, true);

			FTimerHandle h;
			GetWorldTimerManager().SetTimer(h, [this](){ GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }, FMath::Clamp(GrappledLaunchTime-0.05f, 0.f, GrappledLaunchTime), false);
		}

		bCanBeGrappleHooked = false;
		GetWorldTimerManager().SetTimer(Handle_GrappledCooldown, this, &ASmallEnemy::ResetCanBeGrappleHooked, GrappleHookedInternalCooldown);
	}
}

void ASmallEnemy::UnHookedPure()
{
	Execute_UnHooked(this);
}

bool ASmallEnemy::CanBeAttacked()
{
	return false;
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
		GetWorldTimerManager().SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, SmackAttack_InternalTimer, false);
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
