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
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}
	else if (CharacterOwner_Steikemann->IsDashing()) 
	{
		GravityScale = 0.f;
	}
	else if (bStickingToWall && bWallSlowDown)
	{
		GravityScale = FMath::FInterpTo(GravityScale, 1.f, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}
	else{
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride_Freefall, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}


	/* Jump velocity */
	if (/*CharacterOwner_Steikemann->IsJumping()*/ CharacterOwner_Steikemann->bAddJumpVelocity)
	{
		if (bWallJump) {
			Velocity = WallJump_VelocityDirection;
			SetMovementMode(MOVE_Falling);
		}
		else {
			Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity);
			SetMovementMode(MOVE_Falling);
		}
	}

	/* Dash */
	if (CharacterOwner_Steikemann->IsDashing())
	{
		Update_Dash(DeltaTime);
	}

	/* Wall Jump / Sticking to wall */
	if ((CharacterOwner_Steikemann->bFoundStickableWall && CharacterOwner_Steikemann->bCanStickToWall) && GetMovementName() == "Falling") {
		bStickingToWall = StickToWall(DeltaTime);
	}
	else if ((!CharacterOwner_Steikemann->bFoundStickableWall && !CharacterOwner_Steikemann->IsStickingToWall()) || GetMovementName() == "Falling") {
		bWallSlowDown = false;
	}
}


bool USteikemannCharMovementComponent::DoJump(bool bReplayingMoves)
{
	if (!CharacterOwner_Steikemann) { return false; }

	if (CharacterOwner_Steikemann->bCanPostEdgeJump)	{ return true; }
	if (CharacterOwner_Steikemann->bCanEdgeJump)		{ return true; }

	if (CharacterOwner_Steikemann->CanJump() || CharacterOwner_Steikemann->CanDoubleJump())
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

void USteikemannCharMovementComponent::Start_Dash(float preDashTime, float dashTime, float dashLength, FVector dashdirection)
{
	//PRINTLONG("Start Dash");
	fPreDashTimerLength = preDashTime;
	fDashTimerLength = dashTime;
	fDashLength = dashLength;
	DashDirection = dashdirection;
	DashDirection.Normalize();
}

void USteikemannCharMovementComponent::Update_Dash(float deltaTime)
{
	const bool activateDash = fPreDashTimer > fPreDashTimerLength;
	fPreDashTimer += deltaTime;

	if (activateDash) {
		float speed = fDashLength / fDashTimerLength;

		if (fDashTimer < fDashTimerLength)
		{
			fDashTimer += deltaTime;
			Velocity = DashDirection * speed;
		}
		else
		{
			fPreDashTimer = 0.f;
			fDashTimer = 0.f;
			Velocity *= 0;
			CharacterOwner_Steikemann->bDash = false;
		}
	}
}

bool USteikemannCharMovementComponent::WallJump(const FVector& ImpactNormal)
{
	FVector OrthoVector = FVector::CrossProduct(ImpactNormal, FVector::CrossProduct(FVector::UpVector, ImpactNormal));
	OrthoVector.Normalize();

	float radians = WallJump_JumpAngle * (PI / 180);
	WallJump_VelocityDirection = (cosf(radians) * ImpactNormal) + (sinf(radians) * OrthoVector);
	WallJump_VelocityDirection.Normalize();
	WallJump_VelocityDirection *= JumpZVelocity;

	DrawDebugLine(GetWorld(), CharacterOwner_Steikemann->GetActorLocation(), CharacterOwner_Steikemann->GetActorLocation() + (ImpactNormal * 300.f), FColor::Blue, false, 2.f, 0, 4.f);
	DrawDebugLine(GetWorld(), CharacterOwner_Steikemann->GetActorLocation(), CharacterOwner_Steikemann->GetActorLocation() + WallJump_VelocityDirection, FColor::Yellow, false, 2.f, 0, 4.f);

	bWallJump = true;
	return false;
}

bool USteikemannCharMovementComponent::StickToWall(float DeltaTime)
{
	if (Velocity.Z > 0.f) { return false; }

	if (Velocity.Size() < WallJump_MaxStickingSpeed)
	{
		Velocity *= 0;
		GravityScale = 0;
		return true;
	}
	else {
		FVector Vel = Velocity;
		Vel.Normalize();
		(Velocity.Size()>1000.f) ? Velocity -= (Vel * (WallJump_WalltouchSlow * Velocity.Size()/1000.f)) : Velocity -= (Vel * WallJump_WalltouchSlow);
		bWallSlowDown = true;
	}
	return false;
}
