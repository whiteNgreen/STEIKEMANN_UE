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

	//GravityScale = GravityScale
}

void USteikemannCharMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	/* Friction */
	{
		//GroundFriction = CharacterFriction * Traced_GroundFriction;
		GroundFriction = CharacterFriction;
	}

	/* -- Gravity -- */
	SetGravityScale(DeltaTime);
	PRINTPAR("GravityScale %f", GravityScale);

	/* Crouch */
	if (GetCharOwner()->IsCrouchSliding()) {
		Do_CrouchSlide(DeltaTime);
		GroundFriction = 0.f;
	}

	/* Jump velocity */
	if (bIsJumping || bIsDoubleJumping)
	{
		DetermineJump(DeltaTime);

		SetMovementMode(MOVE_Falling);
	}
	if (bJumpPrematureSlowdown)
	{
		SlowdownJumpSpeed(DeltaTime);
	}

	if (bJumpHeightHold){
		AddForce(FVector(0, 0, 981.f * Mass * GravityScale));
		if (bIsJumping && bIsDoubleJumping && Velocity.Z > 0.f) {
			Velocity.Z = FMath::FInterpTo(Velocity.Z, 0.f, DeltaTime, GetCharOwner()->JumpHeightHold_VelocityInterpSpeed);
		}
		
	}

	/* Wall Jump / Sticking to wall */
	switch (m_WallState)
	{
	case EOnWallState::WALL_None:
		break;
	case EOnWallState::WALL_Hang:
	{
		OnWallHang_IMPL();
		break;
	}
	case EOnWallState::WALL_Drag:
	{
		OnWallDrag_IMPL(DeltaTime);
		break;
	}
	case EOnWallState::WALL_Leave:
		break;
	default:
		break;
	}

}


void USteikemannCharMovementComponent::SetGravityScale(float deltatime)
{

	switch (m_GravityMode)
	{
	case EGravityMode::Default:
		GravityScale = m_GravityScaleOverride;
		break;
	case EGravityMode::LerpToDefault:
		GravityScale = FMath::FInterpTo(GravityScale, m_GravityScaleOverride, deltatime, m_GravityScaleOverride_InterpSpeed);
		if (GravityScale < m_GravityScaleOverride - 0.01f || GravityScale > m_GravityScaleOverride + 0.05f)
			m_GravityMode = EGravityMode::Default;
		break;
	case EGravityMode::None:
		GravityScale = 0.f;
		break;
	case EGravityMode::LerpToNone:
		GravityScale = FMath::FInterpTo(GravityScale, 0.f, deltatime, m_GravityScaleOverride_InterpSpeed);
		if (GravityScale < m_GravityScaleOverride - 0.01f || GravityScale > m_GravityScaleOverride + 0.05f)
			m_GravityMode = EGravityMode::None;
		break;
	case EGravityMode::ForcedNone:
		GravityScale = 0.f;
		Velocity *= 0.f;
		break;
	default:
		break;
	}
}

void USteikemannCharMovementComponent::Initiate_CrouchSlide(const FVector& SlideDirection)
{
	CrouchSlideDirection = SlideDirection;
	CrouchSlideSpeed = GetCharOwner()->CrouchSlideSpeed;
}

void USteikemannCharMovementComponent::Do_CrouchSlide(float DeltaTime)
{
	float InterpSpeed = 1.f / GetCharOwner()->CrouchSlide_Time;
	CrouchSlideSpeed = FMath::FInterpTo(CrouchSlideSpeed, CrouchSlideSpeed * GetCharOwner()->EndCrouchSlideSpeedMultiplier, DeltaTime, InterpSpeed);

	FVector Slide = CrouchSlideDirection * CrouchSlideSpeed;

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
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (RightOrtho * 300.f), FColor::Green, false, 1.f, 0, 4.f);
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (GetOwner()->GetActorForwardVector() * 300.f), FColor::Red, false, 1.f, 0, 4.f);

		float AngleDirection = FVector::DotProduct(RightOrtho, Input);
		if (AngleDirection < 0.f) { AngleBetween *= -1.f; }

		//PRINTPARLONG("-- anglebetween -------: %f", FMath::RadiansToDegrees(AngleBetween));

		float JumpAngle = FMath::ClampAngle(FMath::RadiansToDegrees(AngleBetween), -CSJ_MaxInputAngleAdjustment, CSJ_MaxInputAngleAdjustment);
		//float JumpAngle = FMath::Clamp(AngleBetween, -CSJ_MaxInputAngleAdjustment, CSJ_MaxInputAngleAdjustment);

		//PRINTPARLONG("-- anglebetween CLAMPED: %f", JumpAngle);

		FinalSlideDirection = SlideDirection2D.RotateAngleAxis(JumpAngle, FVector::UpVector);
		//FinalSlideDirection = SlideDirection2D.RotateAngleAxis(JumpAngle, FVector::UpVector);
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (FinalSlideDirection * 300.f), FColor::Orange, false, 1.f, 0, 5.f);
	}
	/* Rotate the direction upwards toward the engines UpVector CrouchSlideJumpAngle amount of degrees */
	FVector OrthoUp		{ FVector::CrossProduct(FinalSlideDirection, FVector::CrossProduct(FVector::UpVector, FinalSlideDirection)) };
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (OrthoUp * 300.f), FColor::Blue, false, 1.f, 0, 4.f);

	FinalSlideDirection = (FinalSlideDirection * cosf(FMath::DegreesToRadians(CrouchSlideJumpAngle))) + (OrthoUp * sinf(FMath::DegreesToRadians(CrouchSlideJumpAngle)));
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (FinalSlideDirection * CrouchJumpSpeed), FColor::Orange, false, 1.f, 0, 5.f);

	CrouchSlideJump_Vector = FinalSlideDirection * CrouchJumpSpeed;
	GetCharOwner()->Stop_CrouchSliding();
	return bCrouchSlideJump;
}

bool USteikemannCharMovementComponent::DoJump(bool bReplayingMoves)
{
	if (!GetCharOwner()) { return false; }

	if (GetCharOwner()->bCanPostEdgeRegularJump)	{ return true; }
	if (GetCharOwner()->bCanEdgeJump)		{ return true; }

	if (GetCharOwner()->CanJump() || GetCharOwner()->CanDoubleJump())
	{
		// Don't jump if we can't move up/down.
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			return true;
		}
	}

	return false;
}

void USteikemannCharMovementComponent::Jump(const float& JumpStrength)
{
	Jump(FVector::UpVector, JumpStrength);
}

void USteikemannCharMovementComponent::Jump(const FVector& direction, const float& JumpStrength)
{
	bIsJumping = true;

	Velocity.Z = 0.f;
	AddImpulse(direction * JumpStrength, true);
	InitialJumpVelocity = JumpStrength;
}

void USteikemannCharMovementComponent::DoubleJump(const FVector& Direction, const float& JumpStrength)
{
	/* Direction is input (2D) */

	//FVector Dir = Direction; Dir.Normalize();

	/* If input is nearly Zero do regular jump */
	if (Direction.IsNearlyZero()) 
	{
		//PRINTLONG("ZERO : DOUBLE JUMP");
		Jump(JumpStrength);
		return;
	}
	if (FVector::DotProduct(Direction, Velocity.GetSafeNormal2D()) > 0.8)
	{
		//PRINTLONG("SIMILAR : DOUBLE JUMP");
		Jump(JumpStrength);
		return;
	}

	bIsJumping = true;
	bIsDoubleJumping = true;

	Velocity *= 0.f;
	float UpAngle = GetCharOwner()->DoubleJump_AngleFromUp;
	FVector JumpDirection = (cosf(FMath::DegreesToRadians(UpAngle)) * FVector::UpVector) + (sinf(FMath::DegreesToRadians(UpAngle)) * Direction);
	JumpDirection *= JumpStrength;

	AddImpulse(JumpDirection, true);
	InitialJumpVelocity = JumpDirection.Z;
}

void USteikemannCharMovementComponent::DetermineJump(float DeltaTime)
{
	if (bIsDoubleJumping)
	{
		if (Velocity.Z <= 0.f){
			StopJump();
			//StartJumpHeightHold();
		}
		return;
	}

	JumpPercentage = Velocity.Z / InitialJumpVelocity;
	if (JumpPercentage < (1 - GetCharOwner()->JumpFullPercentage))
	{
		StopJump();
	}
}

void USteikemannCharMovementComponent::SlowdownJumpSpeed(float DeltaTime)
{
	Velocity.Z = FMath::FInterpTo(Velocity.Z, 0.f, DeltaTime, 1 / JumpPrematureSlowDownTime);
	if (Velocity.Z < 0.f)
	{
		bJumpPrematureSlowdown = false;
	}
}

void USteikemannCharMovementComponent::StopJump()
{
	bIsJumping = false;
	bIsDoubleJumping = false;
	GetCharOwner()->bJumping = false;

	if (JumpPercentage > (1 - GetCharOwner()->JumpFullPercentage))
	{
		bJumpPrematureSlowdown = true;
	}
	JumpPercentage = 0.f;
}

void USteikemannCharMovementComponent::StartJumpHeightHold()
{
	bJumpHeightHold = true;
	GetCharOwner()->GetWorldTimerManager().SetTimer(TH_JumpHold, this, &USteikemannCharMovementComponent::StopJumpHeightHold, GetCharOwner()->Jump_HeightHoldTimer);
}

void USteikemannCharMovementComponent::StopJumpHeightHold()
{
	bJumpHeightHold = false;
}

void USteikemannCharMovementComponent::DeactivateJumpMechanics()
{
	bIsJumping = false;
	bIsDoubleJumping = false;
	GetCharOwner()->bJumping = false;
	bJumpPrematureSlowdown = false;
}

void USteikemannCharMovementComponent::PB_Launch_Active(FVector direction, float strength)
{
	//m_GravityMode = EGravityMode::Default;
	Velocity *= 0.f;
	AddImpulse(direction * strength, true);
}



void USteikemannCharMovementComponent::Initial_OnWall_Hang(const Wall::WallData& wall, float time)
{
	PRINTLONG("ON WALL HANG");

	m_WallJumpData = wall;
	m_WallState = EOnWallState::WALL_Hang;

	Velocity *= 0.f;
	m_GravityScaleOverride_InterpSpeed = 1.f / time;
	m_GravityMode = EGravityMode::LerpToNone;

	//FTimerHandle h;
	GetCharOwner()->GetWorldTimerManager().SetTimer(TH_WallHang, [this]()
		{
			m_GravityMode = EGravityMode::LerpToDefault;
			m_WallState = EOnWallState::WALL_Drag;
			GetCharOwner()->m_WallState = EOnWallState::WALL_Drag;
		}, time, false);
}

void USteikemannCharMovementComponent::WallJump(FVector input, float JumpStrength)
{
	ExitWall_Air();

	FVector Input = input;
	FVector normal = m_WallJumpData.Normal;
	FVector Right, OrthoUp;
	FVector direction = GetInputDirectionToNormal(Input, normal, Right, OrthoUp);		

	direction = ClampDirectionToAngleFromVector(direction, normal, WallJump_SidewaysAngleLimit, Right, OrthoUp);

	direction = (cosf(FMath::DegreesToRadians(WallJump_UpwardAngle)) * direction) + (sinf(FMath::DegreesToRadians(WallJump_UpwardAngle)) * FVector::UpVector);
	direction.Normalize();
	AddImpulse(direction * JumpStrength * WallJump_StrenghtMulti, true);
}

void USteikemannCharMovementComponent::LedgeJump(const FVector input, float JumpStrength)
{
	FVector mInput = input;
	if (mInput.SizeSquared() < 0.35f) {
		Jump(JumpStrength * (1.f + LedgeJumpBoost_Multiplier));
		return;
	}

	// Clamp direction to not go towards vector (wall.normal)
	FVector i = mInput.GetSafeNormal();
	float dot = FVector::DotProduct(m_WallJumpData.Normal * -1.f, i);
	float alpha = mInput.Size();
	if (dot <= 1.f && dot >= 0.95f) {
		Jump(JumpStrength * (1.f + LedgeJumpBoost_Multiplier));
		return;
	}
	if (dot > 0.f)	// Clamp direction
	{
		FVector p = i.ProjectOnTo(m_WallJumpData.Normal * -1);
		i -= p;
		i.Normalize();
		i *= input.Size();

		alpha *= 1.f - dot;

		mInput = i;
	}
	
	// Angle direction towards input
	FVector ortho = FVector::CrossProduct(FVector::UpVector, FVector::CrossProduct(mInput.GetSafeNormal(), FVector::UpVector));
	float angle = FMath::Clamp(LedgeJump_AngleLimit * ((LedgeJump_AngleLimit * alpha) / LedgeJump_AngleLimit), 0.f, LedgeJump_AngleLimit);
	float r = FMath::DegreesToRadians(angle);
	FVector direction = (cosf(r) * FVector::UpVector) + (sinf(r) * ortho);

	Jump(direction, JumpStrength * (1.f + LedgeJumpBoost_Multiplier));
}

void USteikemannCharMovementComponent::ExitWall()
{
	m_WallState = EOnWallState::WALL_None;
	m_GravityMode = EGravityMode::Default;
	GetCharOwner()->ExitOnWall(EState::STATE_OnGround);
}

void USteikemannCharMovementComponent::CancelOnWall()
{
	m_WallState = EOnWallState::WALL_None;
	m_GravityMode = EGravityMode::Default;
	GetCharOwner()->GetWorldTimerManager().ClearTimer(TH_WallHang);
}

void USteikemannCharMovementComponent::ExitWall_Air()
{
	m_GravityMode = EGravityMode::LerpToDefault;
	m_WallState = EOnWallState::WALL_None;
	GetCharacterOwner()->GetWorldTimerManager().ClearTimer(TH_WallHang);
	Velocity *= 0.f;
}


void USteikemannCharMovementComponent::OnWallHang_IMPL()
{
	Velocity *= 0.f;	// TODO: Lerp to zero velocity in all directions?
}

void USteikemannCharMovementComponent::OnWallDrag_IMPL(float deltatime)
{
	Velocity.X = 0.f;
	Velocity.Y = 0.f;
	if (Velocity.Z < -WJ_DragSpeed)
		Velocity.Z = -WJ_DragSpeed;
	if (IsWalking())
		ExitWall();
}

FVector USteikemannCharMovementComponent::GetInputDirectionToNormal(FVector& input, const FVector& normal)
{
	FVector right, up;
	return GetInputDirectionToNormal(input, normal, right, up);
}

FVector USteikemannCharMovementComponent::GetInputDirectionToNormal(FVector& input, const FVector& normal, FVector& right, FVector& up)
{
	FVector Input = input;
	if (input.SizeSquared() < 0.5f)
		Input = normal;
	
	up = FVector::CrossProduct(normal, FVector::CrossProduct(FVector::UpVector, normal));
	right = FVector::CrossProduct(up, normal);

	FVector direction = FVector::VectorPlaneProject(Input, up);
	direction.Normalize();
	return direction;
}

FVector USteikemannCharMovementComponent::ClampDirectionToAngleFromVector(const FVector& direction, const FVector& clampVector, const float angle, const FVector& right, const FVector& up)
{
	float dot = FVector::DotProduct(clampVector, direction);
	if (dot > WallJump_SidewaysAngleLimit)
		return direction;
	
	FVector d;
	float r = FVector::DotProduct(right, direction);
	float a = FMath::RadiansToDegrees(acosf(angle));
	if (r < 0) a *= -1.f;
	d = clampVector.RotateAngleAxis(a, up);
		DrawDebugLine(GetWorld(), m_WallJumpData.Location, m_WallJumpData.Location + (direction * 100.f), FColor::Black, false, 2.f, 0, 5.f);
	return d;
}


void USteikemannCharMovementComponent::GP_PreLaunch()
{
	//bGP_PreLaunch = true;
	m_GravityMode = EGravityMode::ForcedNone;
}

void USteikemannCharMovementComponent::GP_Launch(float strength)
{
	//bGP_PreLaunch = false;
	//const float LaunchStrength = GetCharOwner()->GP_LaunchStrength;
	m_GravityMode = EGravityMode::Default;
	AddImpulse(FVector(0, 0, -strength), true);
}
