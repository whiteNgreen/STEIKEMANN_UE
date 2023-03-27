// Fill out your copyright notice in the Description page of Project Settings.


#include "../Enemies/SmallEnemy.h"
#include "EnemyAIController.h"
#include "EnemyAnimInstance.h"
#include "../Spawner/EnemySpawner.h"
#include "../CommonFunctions.h"
#include "EnemyAnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Gameframework/CharacterMovementComponent.h"
#include "../GameplayTags.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "../Components/BouncyShroomActorComponent.h"

#include "../WorldStatics/SteikeWorldStatics.h"

// Sets default values
ASmallEnemy::ASmallEnemy()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	WallDetector = CreateDefaultSubobject<UWallDetectionComponent>(TEXT("Wall Detection Component"));
	PlayerPogoDetection = CreateDefaultSubobject<USphereComponent>(TEXT("PlayerPogoDetection"));
	PlayerPogoDetection->SetupAttachment(RootComponent);

	TlComp_Smacked = CreateDefaultSubobject<UTimelineComponent>("Timeline_Smacked");

	BoxComp_Chomp = CreateDefaultSubobject<UBoxComponent>("Attack Collider");
	BoxComp_Chomp->SetupAttachment(GetMesh(), FName("SOCKET_Head"));

	BounceComp = CreateDefaultSubobject<UBouncyShroomActorComponent>("Bounce Component");
}

// Called when the game starts or when spawned
void ASmallEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	WallDetector->SetCapsuleSize(WDC_Capsule_Radius, WDC_Capsule_Halfheight);
	WallDetector->SetHeight(WDC_MinHeight, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	WallDetector->SetDebugStatus(bWDC_Debug);

	ChompColliderScale = BoxComp_Chomp->GetRelativeScale3D();
	Chomp_DisableCollision();

	// Adding gameplay tags
	GameplayTags.AddTag(Tag::AubergineDoggo());
	GameplayTags.AddTag(Tag::GrappleTarget_Dynamic());
	GameplayTags.AddTag(Tag::PogoTarget());

	PlayerPogoDetection->SetSphereRadius(PB_SphereRadius);

	// Niagara
	NComp_AirTrailing = CreateNiagaraComponent("TrailinigParticle", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
	NComp_AirTrailing->SetAsset(NS_Trail);
	NComp_AirTrailing->Deactivate();
	Delegate_ParticleUpdate.AddUObject(this, &ASmallEnemy::NS_Update_Trail);

	// Delegates
	BoxComp_Chomp->OnComponentBeginOverlap.AddDynamic(this, &ASmallEnemy::ChompCollisionOverlap);

	// Timeline Components
	FOnTimelineFloatStatic FloatBind;
	FloatBind.BindUObject(this, &ASmallEnemy::Tl_Smacked);
	TlComp_Smacked->AddInterpFloat(Curve_NSTrail, FloatBind);

	// Special Start Conditions
	if (bStartStuckToWall) 
	{
		m_State = EEnemyState::STATE_OnWall;
		m_WallState = EWall::WALL_Stuck;
		PlayerPogoDetection->SetSphereRadius(PB_SphereRadius_Stuck);

		// AI
		Incapacitate(EAIIncapacitatedType::StuckToWall);

		StickToWall();
		Gravity_Tick(0.f);
	}
}

// Called every frame
void ASmallEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector playerLoc = SteikeWorldStatics::PlayerLocation;
	
	//static TIMER tim;
	//tim.Start();

	if (FVector::DistSquared(GetActorLocation(), playerLoc) < _Statics_PlayerDistaceToActive)
	{
		//PRINT("Dog active");
		SetDefaultState();

		const bool wall = WallDetector->DetectStickyWall(this, GetActorLocation(), GetActorForwardVector(), m_WallData, ECC_EnemyWallDetection);
		if (wall && (m_State == EEnemyState::STATE_InAir || m_State == EEnemyState::STATE_Launched) && m_WallState != EWall::WALL_Leaving)
		{
			m_State = EEnemyState::STATE_OnWall;
			m_WallState = EWall::WALL_Stuck;
			PlayerPogoDetection->SetSphereRadius(PB_SphereRadius_Stuck);

			// AI
			Incapacitate(EAIIncapacitatedType::StuckToWall);
		}

		// State 
		switch (m_State)
		{
		case EEnemyState::STATE_None:
			break;
		case EEnemyState::STATE_OnGround:
			break;
		case EEnemyState::STATE_InAir:
			break;
		case EEnemyState::STATE_Launched:
			RotateActorYawToVector(GetVelocity() * -1.f);
			break;
		case EEnemyState::STATE_OnWall:
			StickToWall();
			break;
		default:
			break;
		}

		Gravity_Tick(DeltaTime);
		//// Gravity State
		//auto i = GetCharacterMovement();
		//switch (m_GravityState)
		//{
		//case EGravityState::Default:
		//	i->GravityScale = m_GravityScale;
		//	break;
		//case EGravityState::LerpToDefault:
		//	break;
		//case EGravityState::None:
		//	i->GravityScale = 0.f;
		//	break;
		//case EGravityState::LerpToNone:
		//	break;
		//case EGravityState::ForcedNone:
		//	i->GravityScale = 0.f;
		//	i->Velocity *= 0.f;
		//	break;
		//default:
		//	break;
		//}

		EndTick(DeltaTime);
	}
	//else
		//PRINT("InActive");

	// TAKING TIME
	//tim.Print("Doggo tick single");
	//tim.Print_Average("Doggo tick average");
}

void ASmallEnemy::Anim_Attacked_Pure(FVector direction)
{
	Anim_Attacked();
	m_Anim->SetLaunchedInAir(direction);
}

void ASmallEnemy::SetSpawnPointData(TSharedPtr<SpawnPointData> spawn)
{
	m_SpawnPointData = spawn;
	m_AI->m_SpawnPointData = spawn;
}

FVector ASmallEnemy::GetRandomLocationNearSpawn()
{
	FVector location = m_SpawnPointData->Location;
	FVector direction = FVector::ForwardVector.RotateAngleAxis(RandomFloat(0.f, 360.f), FVector::UpVector);

	location += direction * RandomFloat(m_SpawnPointData->Radius_Min, m_SpawnPointData->Radius_Max);

	return location;
}

void ASmallEnemy::DisableCollisions()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ASmallEnemy::EnableCollisions()
{
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ASmallEnemy::SetDogType(enum EDogType type)
{
	m_DogType = type;

	AEnemyAIController* con = Cast<AEnemyAIController>(GetController());
	if (con) {
		con->m_DogType = type;
		con->m_DogPack = m_DogPack.Get();
	}
}

FGameplayTag ASmallEnemy::SensingPawn(APawn* pawn)
{
	m_SensedPawn = pawn;
	bSensingPawn = true;
	
	auto ITag = Cast<IGameplayTagAssetInterface>(pawn);
	FGameplayTagContainer ContainerTags;
	ITag->GetOwnedGameplayTags(ContainerTags);
	if (ContainerTags.HasTag(Tag::Player()))
	{
		return Tag::Player();
	}
	else if (ContainerTags.HasTag(Tag::AubergineDoggo()))
	{
		return Tag::AubergineDoggo();
	}
	return FGameplayTag();
}

void ASmallEnemy::Alert(const APawn& instigator)
{
	SleepingEnd();
	m_AI->SetState(ESmallEnemyAIState::ChasingTarget);
	Anim_Startled();
}

void ASmallEnemy::SleepingBegin()
{
	m_Anim->bIsSleeping = true;
}

void ASmallEnemy::SleepingEnd()
{
	m_Anim->bIsSleeping = false;
}

void ASmallEnemy::CHOMP_Pure()
{


	Anim_CHOMP();
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
	case EEnemyState::STATE_Launched: return;
	case EEnemyState::STATE_OnWall: return;
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

void ASmallEnemy::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (IncapacitatedLandDelegation.ExecuteIfBound())
		IncapacitatedLandDelegation.Unbind();

	if (IncapacitatedCollisionDelegate.ExecuteIfBound())	// HA CAPSULE_HIT DELEGATION FOR DENNE
		IncapacitatedCollisionDelegate.Unbind();

	m_State = EEnemyState::STATE_OnGround;
	m_Anim->bIsLaunchedInAir = false;
	NS_Stop_Trail();
}

void ASmallEnemy::Gravity_Tick(float DeltaTime)
{
	// Gravity State
	auto i = GetCharacterMovement();
	switch (m_GravityState)
	{
	case EGravityState::Default:
		i->GravityScale = m_GravityScale;
		break;
	case EGravityState::LerpToDefault:
		break;
	case EGravityState::None:
		i->GravityScale = 0.f;
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

void ASmallEnemy::EnableGravity()
{
	m_GravityState = EGravityState::Default;
}
void ASmallEnemy::DisableGravity()
{
	m_GravityState = EGravityState::ForcedNone;
}

void ASmallEnemy::ChompCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == this) return;

	auto iTag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!iTag) return;

	if (iTag->HasMatchingGameplayTag(Tag::Player()) && OtherComp->IsA(UCapsuleComponent::StaticClass()))
	{
		auto IAttack = Cast<IAttackInterface>(OtherActor);
		IAttack->Receive_SmackAttack_Pure(FVector(OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal(), 1000.f);

		AttackContactDelegate.Broadcast(OtherActor);
	}
}

void ASmallEnemy::Activate_AttackCollider()
{
	Chomp_EnableCollision();
}
void ASmallEnemy::Deactivate_AttackCollider()
{
	Chomp_DisableCollision();
}

void ASmallEnemy::Chomp_EnableCollision()
{
	BoxComp_Chomp->SetGenerateOverlapEvents(true);
	BoxComp_Chomp->SetRelativeScale3D(ChompColliderScale);
}

void ASmallEnemy::Chomp_DisableCollision()
{
	BoxComp_Chomp->SetGenerateOverlapEvents(false);
	BoxComp_Chomp->SetRelativeScale3D(FVector(0,0,0));
}

void ASmallEnemy::Incapacitate(const EAIIncapacitatedType& IncapacitateType, float Time/*, const ESmallEnemyAIState& NextState*/)
{
	m_AI->IncapacitateAI(IncapacitateType, Time/*, NextState*/);
	Chomp_DisableCollision();
}

void ASmallEnemy::IncapacitateUndeterminedTime(const EAIIncapacitatedType& IncapacitateType, void(ASmallEnemy::* function)())
{
	Incapacitate(IncapacitateType);

	if (!function) return;
	IncapacitatedCollisionDelegate.BindUObject(this, function);
	IncapacitatedLandDelegation.BindUObject(this, &ASmallEnemy::IncapacitatedLand);
}

void ASmallEnemy::IncapacitatedLand()
{
	Incapacitate(EAIIncapacitatedType::Stunned, Incapacitated_LandedStunTime);
}

bool ASmallEnemy::IsIncapacitated() const
{
	AEnemyAIController* AI = Cast<AEnemyAIController>(GetController());
	return AI->m_AIState == ESmallEnemyAIState::Incapacitated;
}

void ASmallEnemy::CollisionDelegate()
{

}

bool ASmallEnemy::IsTargetWithinSpawn(const FVector& target, const float& radiusmulti) const
{
	return FVector::Dist(m_SpawnPointData->Location, target) < m_SpawnPointData->Radius_Max * radiusmulti;
}

void ASmallEnemy::Capacitate_Grappled()
{
	Incapacitate(EAIIncapacitatedType::Grappled, 1.f/*Post grappled stun timer*/);
}

void ASmallEnemy::RotateActorYawToVector(FVector AimVector, float DeltaTime)
{
	FVector AimXY = AimVector;
	AimXY.Z = 0.f;
	AimXY.Normalize();

	float YawDotProduct = FVector::DotProduct(AimXY, GetActorForwardVector());
	float Yaw = FMath::RadiansToDegrees(acosf(YawDotProduct));

	/*		Check if yaw is to the right or left		*/
	float RightDotProduct = FVector::DotProduct(AimXY, GetActorRightVector());
	if (RightDotProduct < 0.f) { Yaw *= -1.f; }

	if (DeltaTime > 0.f) {
		Yaw *= DeltaTime;
	}

	AddActorWorldRotation(FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);

}

void ASmallEnemy::StickToWall()
{
	m_GravityState = EGravityState::ForcedNone;
	//AI
	IncapacitateUndeterminedTime(EAIIncapacitatedType::StuckToWall);
}

void ASmallEnemy::LeaveWall()
{
	PlayerPogoDetection->SetSphereRadius(PB_SphereRadius);
	m_WallState = EWall::WALL_Leaving;
	TimerManager.SetTimer(TH_LeavingWall, [this]() { m_WallState = EWall::WALL_None; }, WDC_LeavingWallTimer, false);

	//AI - Register collision delegate
	IncapacitateUndeterminedTime(EAIIncapacitatedType::Grappled, &ASmallEnemy::CollisionDelegate);
}

void ASmallEnemy::SpottingPlayer_Begin()
{
	m_Anim->bSpottingPlayer = true;
}
void ASmallEnemy::SpottingPlayer_End()
{
	m_Anim->bSpottingPlayer = false;
}

void ASmallEnemy::TargetedPure()
{
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
		//AI
		IncapacitateUndeterminedTime(EAIIncapacitatedType::Grappled, &ASmallEnemy::Capacitate_Grappled);

		FVector Direction = InstigatorLocation - GetActorLocation();
		RotateActorYawToVector(Direction.GetSafeNormal());
		m_GravityState = EGravityState::ForcedNone;
		return;
	}

	if (bCanBeGrappleHooked)
	{
		GetCharacterMovement()->Velocity *= 0.f;

		/* Rotate again towards Instigator - Yaw*/
		FVector InstigatorDirection = (InstigatorLocation - FVector(0,0,GrappledInstigatorOffset)) - GetActorLocation();
		FVector GoToLocation = InstigatorLocation + (InstigatorDirection.GetSafeNormal2D() * -GrappledInstigatorOffset);
		FVector Direction = GoToLocation - GetActorLocation();
		RotateActorYawToVector(InstigatorDirection);

		// Enable Gravity and Disable Collisions
		m_GravityState = EGravityState::Default;
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		FVector Velocity{};
		OnGround ?
			Velocity = ((Direction) / GrappledLaunchTime) + (0.5f * FVector(0, 0, -m_BaseGravityZ) * GrappledLaunchTime) :	// On Ground
			Velocity = ((InstigatorDirection) / GrappledLaunchTime);														// In Air

		GetCharacterMovement()->AddImpulse(Velocity, true);

		FTimerHandle h;
		TimerManager.SetTimer(h, [this](){ GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }, FMath::Clamp(GrappledLaunchTime - GrappledLaunchTime_CollisionActivation, 0.f, GrappledLaunchTime), false);

		bCanBeGrappleHooked = false;
		TimerManager.SetTimer(Handle_GrappledCooldown, this, &ASmallEnemy::ResetCanBeGrappleHooked, GrappleHookedInternalCooldown);
	}
}

void ASmallEnemy::UnHookedPure()
{
	Execute_UnHooked(this);
}

void ASmallEnemy::PullFree_Pure(const FVector InstigatorLocation)
{
	m_State = EEnemyState::STATE_InAir;
	
	// Add Impulse towards instigator - 2D direction
	FVector Direction = InstigatorLocation - GetActorLocation();
	RotateActorYawToVector(Direction.GetSafeNormal2D());
	GetCharacterMovement()->AddImpulse(Direction.GetSafeNormal2D() * Direction.Size() * Grappled_PulledFreeStrengthMultiplier, true);

	// Enable Gravity and Disable Collisions
	m_GravityState = EGravityState::Default;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }, Grappled_PulledFreeNoCollisionTimer, false);

	// Leave wall
	LeaveWall();

	// Internal cooldown to getting grapplehooked
	bCanBeGrappleHooked = false;
	TimerManager.SetTimer(Handle_GrappledCooldown, this, &ASmallEnemy::ResetCanBeGrappleHooked, GrappleHookedInternalCooldown);
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
		/* Sets a timer before character can be damaged by the same attack */
		TimerManager.SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, SmackAttack_InternalTimer, false);

		EnableGravity();

		/* Weaker smack attack if actor on ground than in air */
		float s;
		GetMovementComponent()->IsFalling() ? s = Strength : s = Strength * SmackAttack_OnGroundMultiplication;

		bCanBeSmackAttacked = false;
		SetActorRotation(FVector(Direction.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
		GetCharacterMovement()->Velocity *= 0.f;
		GetCharacterMovement()->AddImpulse(Direction * s, true);

		Incapacitate(EAIIncapacitatedType::Stunned, 1.5f/* Stun timer */);

		Launched(Direction);
	}
}
void ASmallEnemy::Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength)
{
	SetActorRotation(FVector(PoundDirection.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(PoundDirection * GP_Strength, true);

	/* Sets a timer before character can be damaged by the same attack */
	
	Launched(PoundDirection);
}

void ASmallEnemy::NS_Start_Trail(FVector direction)
{
	bTrailingParticles = true;
	NComp_AirTrailing->Activate(true);
	TlComp_Smacked->PlayFromStart();
}

void ASmallEnemy::NS_Update_Trail(float DeltaTime)
{
	if (!bTrailingParticles) return;

	FVector vel = GetVelocity();
	float fVel = GetVelocity().Length();
	FRotator dir = vel.Rotation();
	NComp_AirTrailing->SetNiagaraVariableVec3("Direction", FVector(dir.Yaw, dir.Pitch, dir.Roll));
	NComp_AirTrailing->SetNiagaraVariableFloat("SpawnRate", fVel * DeltaTime * NS_Trail_SpawnRate * NS_Trail_SpawnRate_Internal);
	NComp_AirTrailing->SetNiagaraVariableFloat("Speed_Min", fVel * NS_Trail_SpeedMin);
	NComp_AirTrailing->SetNiagaraVariableFloat("Speed_Max", fVel * NS_Trail_SpeedMax);
}

void ASmallEnemy::NS_Stop_Trail()
{
	bTrailingParticles = false;
	NComp_AirTrailing->Deactivate();
}

void ASmallEnemy::Tl_Smacked(float value)
{
	NS_Trail_SpawnRate_Internal = value;
}

void ASmallEnemy::Receive_Pogo_GroundPound_Pure()
{
	FVector Direction = FVector::DownVector;
	if (IsStuck_Pure())
		Direction = (FVector::DownVector * (1.f - PB_Groundpound_LaunchWallNormal)) + (m_WallData.Normal * PB_Groundpound_LaunchWallNormal);
	LeaveWall();
	m_GravityState = EGravityState::Default;
	m_State = EEnemyState::STATE_None;

	GetCharacterMovement()->AddImpulse(Direction * PB_Groundpound_LaunchStrength, true);

	// Particles
	//UNiagaraFunctionLibrary::SpawnSystemAtLocation()
}

void ASmallEnemy::Launched()
{
	Launched(GetVelocity().GetSafeNormal());
}

void ASmallEnemy::Launched(FVector direction)
{
	m_State = EEnemyState::STATE_Launched;
	// Animation
	Anim_Attacked_Pure(direction * -1.f);
	RotateActorYawToVector(direction * -1.f);
	// Particles
	NS_Start_Trail(direction);
}
