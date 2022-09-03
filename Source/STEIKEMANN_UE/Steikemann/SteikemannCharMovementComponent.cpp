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
		GroundFriction = CharacterFriction;
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
	else if (bGP_PreLaunch)
	{
		GravityScale = 0.f;
		Velocity *= 0.f;
	}
	else if (bStickingToWall || bWallSlowDown)
	{
		GravityScale = FMath::FInterpTo(GravityScale, 1.f, GetWorld()->GetDeltaSeconds(), GravityScaleOverride_InterpSpeed);
	}

	else{
		GravityScale = FMath::FInterpTo(GravityScale, GravityScaleOverride_Freefall, DeltaTime, GravityScaleOverride_InterpSpeed);
	}
	PRINTPAR("GravityScale: %f", GravityScale);


	/* Crouch */
	if (CharacterOwner_Steikemann->IsCrouchSliding()) {
		Do_CrouchSlide(DeltaTime);
		GroundFriction = 0.f;
	}
	PRINTPAR("Velocity: %f", Velocity.Size());

	/* Jump velocity */
	if (CharacterOwner_Steikemann->IsJumping())
	{
		if (bWallJump) {
			Velocity = WallJump_VelocityDirection;
		}
		else if (bLedgeJump) {
			Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity * (1.f + LedgeJumpBoost_Multiplier));
			//Velocity = LedgeJumpDirection;
		}
		else if (bCrouchJump) {
			Velocity.Z = FMath::Max(Velocity.Z, CrouchJumpSpeed);
		}
		else if (bCrouchSlideJump) {
			Velocity = CrouchSlideJump_Vector;
		}
		else {
			Velocity.Z = FMath::Max(Velocity.Z, JumpZVelocity);
		}

		SetMovementMode(MOVE_Falling);
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
				Update_LedgeGrab();
			}
			else
			{
				if (CharacterOwner_Steikemann->bFoundStickableWall) {
					bStickingToWall = StickToWall(DeltaTime);
				}
				else {
					bWallSlowDown = false;
				}
			}
		}
	}
}


void USteikemannCharMovementComponent::Initiate_CrouchSlide(const FVector& InputDirection)
{
	CrouchSlideDirection = InputDirection;
	CrouchSlideSpeed = CharacterOwner_Steikemann->CrouchSlideSpeed;
}

void USteikemannCharMovementComponent::Do_CrouchSlide(float DeltaTime)
{
	float InterpSpeed = 1.f / CharacterOwner_Steikemann->CrouchSlide_Time;
	//CrouchSlideSpeed = FMath::InterpEaseIn()
	CrouchSlideSpeed = FMath::FInterpTo(CrouchSlideSpeed, CrouchSlideSpeed * CharacterOwner_Steikemann->EndCrouchSlideSpeedMultiplier, DeltaTime, InterpSpeed);

	FVector Slide = CrouchSlideDirection * CrouchSlideSpeed;
	PRINTPAR("CrouchSlideSpeed: %f", CrouchSlideSpeed);

	/* Setting Velocity in only X and Y to still make gravity have an effect */
	{
		Velocity.X = Slide.X;
		Velocity.Y = Slide.Y;
	}
}

bool USteikemannCharMovementComponent::CrouchSlideJump(const FVector& SlideDirection, const FVector& Input)
{
	bCrouchSlideJump = true;

	FVector FinalSlideDirection{ SlideDirection };

	/* Check Input direction relative to the SlideDirection in 2D space 
	*  If there is no Input, the jump will simply move forward along the SlideDirection */
	if (Input.Size() > 0.f)
	{
		FVector SlideDirection2D	{ FVector(FVector2D(SlideDirection), 0.f) };
		//float AngleBetween = acosf(SlideDirection.CosineAngle2D(Input));
		float AngleBetween = acosf(FVector::DotProduct(SlideDirection, Input));
		
		/* Find the left/right direction of the input relative to the SlideDirection */
		FVector RightOrtho			{ FVector::CrossProduct(SlideDirection2D, FVector::CrossProduct(GetOwner()->GetActorRightVector(), SlideDirection2D)) };
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (RightOrtho * 300.f), FColor::Green, false, 1.f, 0, 4.f);
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (GetOwner()->GetActorForwardVector() * 300.f), FColor::Red, false, 1.f, 0, 4.f);

		float AngleDirection { FVector::DotProduct(RightOrtho, Input) };
		if (AngleDirection < 0.f) { AngleBetween *= -1.f; }

		PRINTPARLONG("-- anglebetween -------: %f", FMath::RadiansToDegrees(AngleBetween));

		float JumpAngle = FMath::ClampAngle(FMath::RadiansToDegrees(AngleBetween), -CSJ_MaxInputAngleAdjustment, CSJ_MaxInputAngleAdjustment);
		//float JumpAngle = FMath::Clamp(AngleBetween, -CSJ_MaxInputAngleAdjustment, CSJ_MaxInputAngleAdjustment);

		PRINTPARLONG("-- anglebetween CLAMPED: %f", JumpAngle);

		FinalSlideDirection = SlideDirection2D.RotateAngleAxis(JumpAngle, FVector::UpVector);
		//FinalSlideDirection = SlideDirection2D.RotateAngleAxis(JumpAngle, FVector::UpVector);
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (FinalSlideDirection * 300.f), FColor::Orange, false, 1.f, 0, 5.f);
	}
	/* Rotate the direction upwards toward the engines UpVector CrouchSlideJumpAngle amount of degrees */
	FVector OrthoUp		{ FVector::CrossProduct(FinalSlideDirection, FVector::CrossProduct(FVector::UpVector, FinalSlideDirection)) };
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (OrthoUp * 300.f), FColor::Blue, false, 1.f, 0, 4.f);

	FinalSlideDirection = (FinalSlideDirection * cosf(FMath::DegreesToRadians(CrouchSlideJumpAngle))) + (OrthoUp * sinf(FMath::DegreesToRadians(CrouchSlideJumpAngle)));
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (FinalSlideDirection * CrouchJumpSpeed), FColor::Orange, false, 1.f, 0, 5.f);

	CrouchSlideJump_Vector = FinalSlideDirection * CrouchJumpSpeed;
	CharacterOwner_Steikemann->Stop_CrouchSliding();
	return bCrouchSlideJump;
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

	//float radians = WallJump_JumpAngle * (PI / 180);
	float radians = FMath::DegreesToRadians(WallJump_JumpAngle);
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
}

bool USteikemannCharMovementComponent::LedgeJump(const FVector& LedgeLocation)
{
	float InputAngle = CharacterOwner_Steikemann->InputAngleToForward;
	//float Angle = 45.f;

	//float ImpulseStrength = 300.f;

	LedgeJumpDirection = CharacterOwner_Steikemann->GetActorForwardVector();
	float ClampedAngle = FMath::Clamp(InputAngle, -LedgeJump_AngleClamp, LedgeJump_AngleClamp);
	LedgeJumpDirection = LedgeJumpDirection.RotateAngleAxis(-ClampedAngle, FVector::UpVector);
	
	AddImpulse(LedgeJumpDirection * LedgeJump_ImpulseStrength, true);
		DrawDebugLine(GetWorld(), CharacterOwner_Steikemann->GetActorLocation(), CharacterOwner_Steikemann->GetActorLocation() + (LedgeJumpDirection * LedgeJump_ImpulseStrength), FColor::Orange, false, 1.f, 0, 4.f);


	bLedgeJump = true;
	bLedgeGrab = false;
	return true;
}

void USteikemannCharMovementComponent::GP_PreLaunch()
{
	bGP_PreLaunch = true;
}

void USteikemannCharMovementComponent::GP_Launch()
{
	bGP_PreLaunch = false;
	const float LaunchStrength = CharacterOwner_Steikemann->GP_LaunchStrength;
	AddImpulse(FVector(0, 0, -LaunchStrength), true);
}
