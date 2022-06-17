// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "Engine.h"

USteikemannCharMovementComponent::USteikemannCharMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USteikemannCharMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner_Steikemann = Cast<ASteikemannCharacter>(GetCharacterOwner());

}

void USteikemannCharMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* Friction */
	{
		GroundFriction = CharacterFriction * Traced_GroundFriction;
	}

	/* Gravity */
	if (CharacterOwner_Steikemann->IsJumping() || MovementMode == MOVE_Walking)
	{
		GravityScale = FMath::FInterpTo(GravityScale, 1.f, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}
	else{
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}
	if (CharacterOwner_Steikemann->IsDashing()) 
	{
		GravityScale = 0.f;
	}


	/* Jump velocity */
	if (CharacterOwner_Steikemann->IsJumping())
	{
		Velocity.Z = FMath::FInterpTo(Velocity.Z, JumpZVelocity, GetWorld()->GetDeltaSeconds(), JumpInterpSpeed);
		SetMovementMode(MOVE_Falling);
	}

	//PRINTPAR("Speed: %f", Velocity.Size());

	/* Dash */
	if (CharacterOwner_Steikemann->IsDashing())
	{
		Update_Dash(DeltaTime);
	}

	/* Wall Jump / Sticking to wall */
	if ((CharacterOwner_Steikemann->bFoundStickableWall && CharacterOwner_Steikemann->bCanStickToWall) && GetMovementName() == "Falling")
	{
		PRINT("STICKTOWALL");
		bStickingToWall = StickToWall();
	}
}


bool USteikemannCharMovementComponent::DoJump(bool bReplayingMoves)
{
	if (CharacterOwner_Steikemann && CharacterOwner_Steikemann->bCanEdgeJump)
	{
		return true;
	}

	if (CharacterOwner_Steikemann && (CharacterOwner_Steikemann->CanJump() || CharacterOwner_Steikemann->CanDoubleJump()))
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			return true;
		}
	}


	return false;
}

void USteikemannCharMovementComponent::Bounce(FVector surfacenormal)
{
	FVector negativeVelocity = Velocity * -1.f;
	negativeVelocity.Normalize();

	FVector orthogonalVelocity = FVector::CrossProduct(negativeVelocity, FVector::CrossProduct(surfacenormal, negativeVelocity));
	orthogonalVelocity.Normalize();

	float dotproduct = FVector::DotProduct(negativeVelocity, surfacenormal);
	float angle = acosf(dotproduct);

	FVector newVelocity = (cos(angle * 2) * negativeVelocity) + (sin(angle * 2) * orthogonalVelocity);
	newVelocity *= Velocity.Size();

	Velocity = newVelocity;
}

void USteikemannCharMovementComponent::Start_Dash(float dashTime, float dashLength, FVector dashdirection)
{
	PRINTLONG("Start Dash");
	fDashTimerLength = dashTime;
	fDashLength = dashLength;
	DashDirection = dashdirection;
	DashDirection.Normalize();
}

void USteikemannCharMovementComponent::Update_Dash(float deltaTime)
{
	float speed = fDashLength / fDashTimerLength;

	if (fDashTimer < fDashTimerLength)
	{
		fDashTimer += deltaTime;
		Velocity = DashDirection * speed;
	}
	else
	{
		fDashTimer = 0.f;
		Velocity *= 0;
		CharacterOwner_Steikemann->bDash = false;
	}
}

bool USteikemannCharMovementComponent::StickToWall()
{
	if (Velocity.Z > 0.f) { return false; }

	if (Velocity.Size() < WallJump_MaxStickingSpeed)
	{
		Velocity *= 0;
		GravityScale = 0;
		return true;
	}
	return false;
}
