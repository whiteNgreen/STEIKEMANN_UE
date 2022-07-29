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

	GroundFriction = CharacterFriction;
}

void USteikemannCharMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* Friction */
	{
		//GroundFriction = CharacterFriction * Traced_GroundFriction;
	}

	/* Gravity */
	if (CharacterOwner_Steikemann->IsJumping() || MovementMode == MOVE_Walking || bWallJump)
	{
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride, DeltaTime, GravityScaleOverride_InterpSpeed);
	}
	else if (CharacterOwner_Steikemann->IsDashing()) 
	{
		GravityScale = 0.f;
	}
	else if (bStickingToWall || bWallSlowDown)
	{
		GravityScale = FMath::FInterpTo(GravityScale, 1.f, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}
	else{
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride_Freefall, DeltaTime, GravityScaleOverride_InterpSpeed);
	}
	//PRINTPAR("GravityScale: %f", GravityScale);


	/* Jump velocity */
	if (CharacterOwner_Steikemann->IsJumping() /*CharacterOwner_Steikemann->bAddJumpVelocity*/)
	{
		if (bWallJump) {
			Velocity = WallJump_VelocityDirection;
			SetMovementMode(MOVE_Falling);
		}
		else if (bLedgeJump) {
			Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity * (1.f + LedgeJumpBoost_Multiplier));
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
	if (GetMovementName() == "Falling")
	{
		if (CharacterOwner_Steikemann->bOnWallActive)
		{
			if (bLedgeGrab) {
				//PRINT("bLedgeGrab = true");
				Update_LedgeGrab();
			}
			else /*if (!CharacterOwner_Steikemann->bFoundLedge)*/
			{
				PRINT("WallJump / WallSliding - CharacterMovementComponent");
				if (CharacterOwner_Steikemann->bFoundStickableWall /* && CharacterOwner_Steikemann->bCanStickToWall*/) {
					bStickingToWall = StickToWall(DeltaTime);
				}
				else /*(!CharacterOwner_Steikemann->bFoundStickableWall && !CharacterOwner_Steikemann->bCanStickToWall)*/ {
					bWallSlowDown = false;
				}
			}
		}
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
	PRINTLONG("WallJump");

	FVector InputDirection{ CharacterOwner_Steikemann->InputVector };
	float InputToForwardAngle{ 0.f };
	if (InputDirection.SizeSquared() > 0.5f)
	{
		InputToForwardAngle = FMath::RadiansToDegrees(acosf(FVector::DotProduct(GetCharacterOwner()->GetActorForwardVector(), InputDirection)));
		float InputAngleDirection{ FVector::DotProduct(GetCharacterOwner()->GetActorRightVector(), InputDirection) };
		if (InputAngleDirection > 0.f) { InputToForwardAngle *= -1.f; }
		//PRINTPARLONG("ANGLE FROM FORWARD: %f", InputToForwardAngle);
	}

	FVector OrthoVector = FVector::CrossProduct(ImpactNormal, FVector::CrossProduct(FVector::UpVector, ImpactNormal));
	OrthoVector.Normalize();

	float radians = WallJump_JumpAngle * (PI / 180);
	WallJump_VelocityDirection = (cosf(radians) * ImpactNormal) + (sinf(radians) * OrthoVector);
	WallJump_VelocityDirection.Normalize();
	
	if (InputToForwardAngle > 20.f || InputToForwardAngle < -20.f)
	{
		if (InputToForwardAngle > 45.f) {
			WallJump_VelocityDirection = WallJump_VelocityDirection.RotateAngleAxis(WallJump_SidewaysJumpAngle, OrthoVector);
		}
		else if (InputToForwardAngle < -45.f) {
			WallJump_VelocityDirection = WallJump_VelocityDirection.RotateAngleAxis(-WallJump_SidewaysJumpAngle, OrthoVector);
		}
	}

	WallJump_VelocityDirection *= JumpZVelocity;

		DrawDebugLine(GetWorld(), CharacterOwner_Steikemann->GetActorLocation(), CharacterOwner_Steikemann->GetActorLocation() + (ImpactNormal * 300.f), FColor::Blue, false, 2.f, 0, 4.f);
		DrawDebugLine(GetWorld(), CharacterOwner_Steikemann->GetActorLocation(), CharacterOwner_Steikemann->GetActorLocation() + WallJump_VelocityDirection, FColor::Yellow, false, 2.f, 0, 4.f);

	bWallJump = true;
	bStickingToWall = false;
	bWallSlowDown = false;
	//CharacterOwner_Steikemann->bCanStickToWall = true;
	CharacterOwner_Steikemann->WallJump_NonStickTimer = 0.f;
	return true;
}

bool USteikemannCharMovementComponent::StickToWall(float DeltaTime)
{
	if (Velocity.Z > 0.f) { bWallSlowDown = false;  return false; }

	if (Velocity.Size() < WallJump_MaxStickingSpeed)
	{
		Velocity *= 0;
		GravityScale = 0;
		bWallSlowDown = false;
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

bool USteikemannCharMovementComponent::ReleaseFromWall(const FVector& ImpactNormal)
{
	float angle = FMath::DegreesToRadians(30.f);
	FVector ReleaseVector = (cosf(angle) * FVector::DownVector) + (sinf(angle) * ImpactNormal);

		DrawDebugLine(GetWorld(), CharacterOwner_Steikemann->GetActorLocation(), CharacterOwner_Steikemann->GetActorLocation() + (ReleaseVector * 300.f), FColor::Green, false, 2.f, 0, 4.f);

	bStickingToWall = false;
	bWallSlowDown = false;

	AddImpulse(ReleaseVector * 200.f, true);

	return false;
}

void USteikemannCharMovementComponent::Start_LedgeGrab()
{
	//PRINTLONG("START LEDGEGRAB");
	bLedgeGrab = true;
}

void USteikemannCharMovementComponent::Update_LedgeGrab()
{
	Velocity *= 0;
	GravityScale = 0;
	PRINT("Is LedgeGrabbing");
}

bool USteikemannCharMovementComponent::LedgeJump(const FVector& LedgeLocation)
{
	//LedgeJumpBoost = CharacterOwner_Steikemann->LengthToLedge * LedgeJumpBoost_Multiplier;
	//LedgeJumpBoost = 100.f;
	PRINTPARLONG("LedgeJumpLength: %f", JumpZVelocity + LedgeJumpBoost);
	
	bLedgeJump = true;

	bLedgeGrab = false;
	//CharacterOwner_Steikemann->ResetWallJumpAndLedgeGrab();
	return true;
}
