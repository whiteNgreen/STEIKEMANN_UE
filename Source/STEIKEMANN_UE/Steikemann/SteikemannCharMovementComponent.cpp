// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "../Steikemann/SteikemannCharacter.h"
//#include "Engine.h"
#include "BaseClasses/StaticVariables.h"
//#include "../WallDetection/WallDetectionComponent.h"


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
	GroundFriction = CharacterFriction;

	/* -- Gravity -- */
	SetGravityScale(DeltaTime);
	PRINTPAR("GravityScale %f", GravityScale);
	PRINTPAR("Friction %f", GroundFriction);
	

	/* Jump velocity */
	if (bIsJumping || bIsDoubleJumping){
		DetermineJump(DeltaTime);
		SetMovementMode(MOVE_Falling);
	}
	if (bJumpPrematureSlowdown){
		SlowdownJumpSpeed(DeltaTime);
	}

	if (bJumpHeightHold){
		AddForce(FVector(0, 0, 981.f * Mass * GravityScale));
		if (bIsJumping && bIsDoubleJumping && Velocity.Z > 0.f) {
			Velocity.Z = FMath::FInterpTo(Velocity.Z, 0.f, DeltaTime, GetCharOwner()->JumpHeightHold_VelocityInterpSpeed /* Get from a function argument and set class variable */);
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


void USteikemannCharMovementComponent::EnableGravity()
{
	m_GravityMode = EGravityMode::Default;
}

void USteikemannCharMovementComponent::DisableGravity()
{
	m_GravityMode = EGravityMode::ForcedNone;
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



bool USteikemannCharMovementComponent::DoJump(bool bReplayingMoves)
{
	//if (!GetCharOwner()) { return false; }

	//if (GetCharOwner()->bCanPostEdgeRegularJump)	{ return true; }
	//if (GetCharOwner()->bCanEdgeJump)		{ return true; }

	//if (GetCharOwner()->CanJump() || GetCharOwner()->CanDoubleJump())
	//{
	//	// Don't jump if we can't move up/down.
	//	if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
	//	{
	//		return true;
	//	}
	//}

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
	/* If input is nearly Zero do regular jump */
	if (Direction.IsNearlyZero()) 
	{
		Jump(JumpStrength);
		return;
	}

	bIsJumping = true;
	bIsDoubleJumping = true;

	Velocity *= 0.f;
	float UpAngle = GetCharOwner()->DoubleJump_AngleFromUp;	// Should be Function Argument
	FVector JumpDirection = (cosf(FMath::DegreesToRadians(UpAngle)) * FVector::UpVector) + (sinf(FMath::DegreesToRadians(UpAngle)) * Direction);
	JumpDirection *= JumpStrength;

	AddImpulse(JumpDirection, true);
	InitialJumpVelocity = JumpDirection.Z;
}

void USteikemannCharMovementComponent::JumpHeight(const float Height, const float time)
{
	Velocity *= 0.f;
	float t = time;

	float gravZ = GetGravityZ();
	float forceZ = (Height / t) + (0.5f * -gravZ * t);

	AddImpulse(FVector(0, 0, forceZ), true);
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
	if (JumpPercentage < (1 - GetCharOwner()->JumpFullPercentage/* Could get this from function argument */))
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
	GetCharOwner()->bJumping = false;	// CharOwners bJumping is NOT used anywhere

	if (JumpPercentage > (1 - GetCharOwner()->JumpFullPercentage/* Could get this from function argument */))
	{
		bJumpPrematureSlowdown = true;
	}
	JumpPercentage = 0.f;
}

void USteikemannCharMovementComponent::StartJumpHeightHold()
{
	bJumpHeightHold = true;
	GetCharOwner()->TimerManager.SetTimer(TH_JumpHold, this, &USteikemannCharMovementComponent::StopJumpHeightHold, GetCharOwner()->Jump_HeightHoldTimer);
}

void USteikemannCharMovementComponent::StopJumpHeightHold()
{
	bJumpHeightHold = false;
}

void USteikemannCharMovementComponent::DeactivateJumpMechanics()
{
	bIsJumping = false;
	bIsDoubleJumping = false;
	GetCharOwner()->bJumping = false;	// CharOwners bJumping is NOT used anywhere
	bJumpPrematureSlowdown = false;
}

void USteikemannCharMovementComponent::AirFriction2D(FVector input)
{
	// Passive AirFriction
	float in = input.Length();
	float x = SMath::Gaussian(in, 5.f, 4.f, 0.f, AirFriction2D_NoInputStrength);
	FVector vel2D = FVector(Velocity.X, Velocity.Y, 0.f);
	FVector PassiveForce = -vel2D * AirFriction2D_Strength * AirFriction2D_Multiplier * Mass * x;

	// Active AirFriction -- "Brakes"
	float dot = FVector::DotProduct(input, Velocity.GetSafeNormal2D());
	float negativeInputDirectionStrength = SMath::Gaussian(dot, 6.f, 2, -1.f);
	FVector ActiveForce = -vel2D * negativeInputDirectionStrength * NegativeAirFriction2D_Strength * Mass;

	AddForce(PassiveForce + ActiveForce);
}

void USteikemannCharMovementComponent::AirFrictionMultiplier(float value)
{
	AirFriction2D_Multiplier = value;
}

void USteikemannCharMovementComponent::PB_Launch_Active(FVector direction, float strength)
{
	//m_GravityMode = EGravityMode::Default;
	Velocity *= 0.f;
	AddImpulse(direction * strength, true);
}



void USteikemannCharMovementComponent::Initial_OnWall_Hang(const Wall::WallData& wall, float time)
{
	m_WallJumpData = wall;
	m_WallState = EOnWallState::WALL_Hang;

	Velocity *= 0.f;
	m_GravityScaleOverride_InterpSpeed = 1.f / time;
	m_GravityMode = EGravityMode::LerpToNone;

	GetCharOwner()->TimerManager.SetTimer(TH_WallHang, [this]()
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
	GetCharOwner()->TimerManager.ClearTimer(TH_WallHang);
}

void USteikemannCharMovementComponent::ExitWall_Air()
{
	m_GravityMode = EGravityMode::LerpToDefault;
	m_WallState = EOnWallState::WALL_None;
	GetCharOwner()->TimerManager.ClearTimer(TH_WallHang);
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
	if (input.SizeSquared() < 0.5f) {
		Input = normal;
		return Input;
	}
	
	up = FVector::CrossProduct(normal, FVector::CrossProduct(FVector::UpVector, normal));
	right = FVector::CrossProduct(up, normal);

	FVector direction = FVector::VectorPlaneProject(Input, up);
	direction.Normalize();
	return direction;
}

FVector USteikemannCharMovementComponent::ClampDirectionToAngleFromVector(const FVector& direction, const FVector& clampVector, const float angle, const FVector& right, const FVector& up)
{
	float dot = FVector::DotProduct(clampVector, direction);
	
	FVector d;
	float r = FMath::Clamp(FVector::DotProduct(right, direction), -angle, angle);
	float a = FMath::RadiansToDegrees(asinf(r));
	d = clampVector.RotateAngleAxis(a, up);
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
