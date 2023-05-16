// Fill out your copyright notice in the Description page of Project Settings.


#include "SmallEnemy.h"
#include "EnemyAIController.h"
#include "EnemyAnimInstance.h"
#include "../Spawner/EnemySpawner.h"
#include "../CommonFunctions.h"
#include "EnemyAnimInstance.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "Gameframework/CharacterMovementComponent.h"
#include "../GameplayTags.h"
#include "../Components/BouncyShroomActorComponent.h"
#include "NiagaraComponent.h"

#include "GameplayTagAssetInterface.h"
#include "../WallDetection/WallDetectionComponent.h"
#include "Components/TimelineComponent.h"

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
	TlComp_LaunchedCollision = CreateDefaultSubobject<UTimelineComponent>("Timeline LaunchedCollision");
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
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ASmallEnemy::OnCapsuleComponentLaunchHit);

	// Timeline Components
	FOnTimelineFloatStatic FloatBind;
	FloatBind.BindUObject(this, &ASmallEnemy::Tl_Smacked);
	TlComp_Smacked->AddInterpFloat(Curve_NSTrail, FloatBind);

	FOnTimelineVectorStatic VectorBind;
	VectorBind.BindUObject(this, &ASmallEnemy::Tl_LaunchedCollisionShake);
	TlComp_LaunchedCollision->AddInterpVector(Curve_LaunchedCollisionShake, VectorBind);
	FOnTimelineEventStatic TimelineFinished;
	TimelineFinished.BindUObject(this, &ASmallEnemy::Tl_LaunchedCollision_End);
	TlComp_LaunchedCollision->SetTimelineFinishedFunc(TimelineFinished);

	MeshInitialPosition = GetMesh()->GetRelativeLocation();

	// Special Start Conditions
	if (bStartStuckToWall) 
	{
		m_State = EEnemyState::STATE_OnWall;
		m_WallState = EWall::WALL_Stuck;
		PlayerPogoDetection->SetSphereRadius(PB_SphereRadius_Stuck);

		// AI
		Incapacitate(EAIIncapacitatedType::StuckToWall);

		StickToWall();
		Gravity_Tick();
		SpecialStartWorldLocation = GetActorLocation();
		TimerManager.SetTimer(TH_SpecialStartWorldLocation, [this]() {
			SetActorLocation(SpecialStartWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
			}, 0.2f, true);
	}
}

void ASmallEnemy::BeginDestroy()
{
	if (GetWorld())
		GetWorldTimerManager().ClearTimer(TH_IAttack_LeewayPause);
	Super::BeginDestroy();
}

// Called every frame
void ASmallEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector playerLoc = SteikeWorldStatics::PlayerLocation;

	if (bIsActive)
	{
		SetDefaultState();
		const bool wall = WallDetector->DetectStickyWall(this, GetActorLocation(), GetActorForwardVector(), m_WallData, ECC_EnemyWallDetection);
		if (wall && (m_State == EEnemyState::STATE_InAir || m_State == EEnemyState::STATE_Launched) && m_WallState != EWall::WALL_Leaving)
		{
			m_State = EEnemyState::STATE_OnWall;
			m_WallState = EWall::WALL_Stuck;
			PlayerPogoDetection->SetSphereRadius(PB_SphereRadius_Stuck);
			// AI
			Incapacitate(EAIIncapacitatedType::StuckToWall);
			StickToWall();
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
			break;
		default:
			break;
		}
		Gravity_Tick();
		EndTick(DeltaTime);

		//PrintState();
	}
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


void ASmallEnemy::DisableCollisions(float time)
{
	ABaseCharacter::DisableCollisions();
	TimerManager.SetTimer(TH_EnableCollision_PlacementCheck, this, &ASmallEnemy::ActorInvalidPlacement, time);
	TimerManager.SetTimer(TH_DisabledCollision, this, &ASmallEnemy::EnableCollisions, time);
}

/* Unfinished function to properly place the actor outside of an object 
*	Meant to be called when collision is enabled after a mechanic required it to be disabled for a period of time */
void ASmallEnemy::ActorInvalidPlacement()
{
	if (!GetWorld()) return;
	FHitResult Hit;
	FCollisionQueryParams Params("", false, this);
	FCollisionShape capsule = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (GetWorld()->SweepSingleByChannel(Hit, GetActorLocation(), GetActorLocation(), FQuat(1, 0, 0, 0), ECC_StaticWorldChannel, capsule, Params))
	{
		// SKRIV MER

		// Her antar vi at hunden er inni et objekt. 
		//		Fra sweep hit, dot produktet mellom hit.normal og hit.impact->actorlocation
		//		Hvis (dot > 0) sjekk om det er korrekt avstand til capsule og hit.impactpoint

		// Kan ogs� sjekke en linetrace fra StartLaunchLocation->CurrentLocation for � se om det er noen objekter mellom. 
		//		Hvis det er det s� gj�r en st�rre capsule sweep enn den forrige for � sjekke etter overflater
		//			Hvis den �kte capsule sweep'en ikke finner en overflate s� gj�r en til sweep som er enda st�rre 
		//			og sett posisjonen basert p� det
		//		Hvis ikke s� er actor sikkert i lufta

	}
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
	bool alerted{};
	if (m_AI)
		alerted = m_AI->AlertedByPack();
	
	// Anim
	if (alerted)
	{
		SleepingEnd();
		Anim_Startled();
	}
}

void ASmallEnemy::SleepingBegin()
{
	m_Anim->bIsSleeping = true;
}

void ASmallEnemy::SleepingEnd()
{
	m_Anim->bIsSleeping = false;
}

void ASmallEnemy::AttackJump()
{
	FVector JumpDirection = 
		(GetActorForwardVector() * (1.f - FMath::Abs(AttackJumpAngle)))
		+ (FVector::UpVector * AttackJumpAngle);

	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(JumpDirection * AttackJumpStrength, true);
	Delegate_StunnedLand.BindUObject(this, &ASmallEnemy::PostChompLand);
}

void ASmallEnemy::CHOMP_Pure()
{
	Anim_CHOMP();
}

void ASmallEnemy::Cancel_CHOMP()
{
	Deactivate_AttackCollider();
	Anim_Attacked();
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

	IGameplayTagAssetInterface* itag = Cast<IGameplayTagAssetInterface>(Hit.GetActor());
	m_Anim->m_AnimState = EEnemyAnimState::Idle_Run;
	if (!Delegate_LaunchedLand.IsBound())
	{
		bool InvalidStunnedLand{};
		if (itag)
			InvalidStunnedLand = 
				itag->HasMatchingGameplayTag(Tag::BouncyShroom()) ||
				itag->HasMatchingGameplayTag(Tag::Player()) ||
				itag->HasMatchingGameplayTag(Tag::Enemy());

		if (Delegate_StunnedLand.ExecuteIfBound() && !InvalidStunnedLand)
			Delegate_StunnedLand.Unbind();
	}	
	if (Delegate_LaunchedLand.ExecuteIfBound(Hit))
		Delegate_LaunchedLand.Unbind();

	m_State = EEnemyState::STATE_OnGround;
	m_Anim->bIsLaunchedInAir = false;

	NS_Stop_Trail();
}

void ASmallEnemy::Gravity_Tick()
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

void ASmallEnemy::EnableGravity_Force()
{
	EnableGravity();
	Gravity_Tick();
}

void ASmallEnemy::DisableGravity_Force()
{
	DisableGravity();
	Gravity_Tick();
}

void ASmallEnemy::ChompCollisionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == this) return;

	auto iTag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!iTag) return;

	if (OtherComp->IsA(UCapsuleComponent::StaticClass()))
	{
		auto IAttack = Cast<IAttackInterface>(OtherActor);
		if (iTag->HasMatchingGameplayTag(Tag::Player())) {
			IAttack->Receive_SmackAttack_Pure(FVector(OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D(), 0.f);
		}
		if (iTag->HasMatchingGameplayTag(Tag::Enemy())) {
			ChompingAnotherEnemy(IAttack, OtherActor);
		}
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

void ASmallEnemy::Bark_Pure()
{
	RotateActorYawToVector(SteikeWorldStatics::PlayerLocation - GetActorLocation());

	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(FVector::UpVector * Bark_JumpStrenght, true);

	Bark_BPEvent();
}

void ASmallEnemy::CorgiJump_Pure()
{
	RotateActorYawToVector(SteikeWorldStatics::PlayerLocation - GetActorLocation());
	FVector JumpDirection =
		(GetActorForwardVector() * (1.f - FMath::Abs(AttackJumpAngle)))
		+ (FVector::UpVector * AttackJumpAngle);
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(JumpDirection * FMath::Max(AttackJumpStrength * 0.5f, CorgiJump_MinJumpStrength), true);

	CorgiJump_BPEvent();
}

void ASmallEnemy::Incapacitate(const EAIIncapacitatedType& IncapacitateType, float Time)
{
	if (m_AI) {
		m_AI->IncapacitateAI(IncapacitateType, Time);
		m_AI->Incapacitate_GettingSmacked();
	}
	Chomp_DisableCollision();
}

void ASmallEnemy::IncapacitateUndeterminedTime(const EAIIncapacitatedType& IncapacitateType, void(ASmallEnemy::* function)())
{
	Incapacitate(IncapacitateType);

	if (!function) return;
	IncapacitatedLandDelegation.BindUObject(this, &ASmallEnemy::IncapacitatedLand);
}

void ASmallEnemy::Post_IncapacitateDetermineState()
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params("", false, this);
	FCollisionShape Capsule = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	FVector SweepLocation = GetActorLocation() - (FVector::UpVector * 20.f);
	if (GetWorld()->SweepMultiByChannel(Hits, SweepLocation, SweepLocation, FQuat(1.f, 0, 0, 0), ECC_WorldStatic, Capsule, Params))
	{
		if (IncapacitatedLandDelegation.ExecuteIfBound())
			IncapacitatedLandDelegation.Unbind();
	}
}

void ASmallEnemy::RedetermineIncapacitateState()
{
	if (GetCharacterMovement()->IsWalking() || m_State == EEnemyState::STATE_OnGround)
	{
		m_AI->RedetermineIncapacitateState();
		IncapacitatedLandDelegation.Unbind();
		m_Anim->m_AnimState = EEnemyAnimState::Idle_Run;
	}
}

void ASmallEnemy::IncapacitatedLand()
{
	Incapacitate(EAIIncapacitatedType::Stunned, Incapacitated_LandedStunTime);
}

void ASmallEnemy::StunnedLand()
{
	Incapacitate(EAIIncapacitatedType::Stunned, Incapacitated_LandedStunTime);
	TimerManager.SetTimer(TH_RedetermineIncapacitate, this, &ASmallEnemy::RedetermineIncapacitateState, Incapacitated_LandedStunTime);
	m_Anim->m_AnimState = EEnemyAnimState::Stunned;
	Anim_Attacked();
	SpawnStunnedEffect(Incapacitated_LandedStunTime);
}

void ASmallEnemy::PostChompLand()
{
	Incapacitate(EAIIncapacitatedType::Stunned, PostChomp_StunTime);
	TimerManager.SetTimer(TH_PostChompStun, this, &ASmallEnemy::RedetermineIncapacitateState, PostChomp_StunTime);
}

bool ASmallEnemy::IsIncapacitated() const
{
	if (!m_AI) return false;
	AEnemyAIController* AI = Cast<AEnemyAIController>(GetController());
	return AI->IsIncapacitated();
}

void ASmallEnemy::CollisionDelegate()
{

}

bool ASmallEnemy::IsTargetWithinSpawn(const FVector& target, const float& radiusmulti) const
{
	return FVector::Dist(m_SpawnPointData->Location, target) < m_SpawnPointData->GuardRadius * radiusmulti;
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

bool ASmallEnemy::ShroomBounce(FVector direction, float strength)
{
	const bool b = (Super::ShroomBounce(direction, strength));
	if (b)
	{
		// Incapacitate Launch
		Incapacitate(EAIIncapacitatedType::Stunned);
		// land delegation
		Launched();
		Delegate_StunnedLand.BindUObject(this, &ASmallEnemy::StunnedLand);
		Delegate_LaunchedLand.BindUObject(this, &ASmallEnemy::LandedLaunched);
		DeleteStunnedEffect();
		TimerManager.ClearTimer(TH_RedetermineIncapacitate);
	}
	return b;
}

void ASmallEnemy::StickToWall()
{
	m_GravityState = EGravityState::ForcedNone;
	//AI
	IncapacitateUndeterminedTime(EAIIncapacitatedType::StuckToWall);

	m_Anim->m_AnimState = EEnemyAnimState::StuckToThornbush;
	Effect_StuckToThornwall_Start();
}

void ASmallEnemy::LeaveWall()
{
	PlayerPogoDetection->SetSphereRadius(PB_SphereRadius);
	m_WallState = EWall::WALL_Leaving;
	TimerManager.SetTimer(TH_LeavingWall, [this]() { m_WallState = EWall::WALL_None; }, WDC_LeavingWallTimer, false);

	//AI - Register collision delegate
	IncapacitateUndeterminedTime(EAIIncapacitatedType::Grappled, &ASmallEnemy::CollisionDelegate);

	m_Anim->m_AnimState = EEnemyAnimState::Idle_Run;
	Effect_StuckToThornwall_End();
}

void ASmallEnemy::Tl_LaunchedCollision_End()
{
	GetMesh()->SetWorldLocation(GetActorLocation() + MeshInitialPosition);
}

void ASmallEnemy::Tl_LaunchedCollisionShake(FVector vector)
{
	GetMesh()->SetWorldLocation(GetActorLocation() + MeshInitialPosition + vector);
}

void ASmallEnemy::OnCapsuleComponentLaunchHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& SweepHit)
{
	if (OtherActor == this) return;

	if (DogEnvironmentCollision(SweepHit)) return;

	if (CanReflectCollisionLaunch())
		ReflectedCollisionLaunch_PreLaunch(SweepHit.ImpactNormal, SweepHit.ImpactPoint);
}
bool ASmallEnemy::CanReflectCollisionLaunch() const
{
	if (TimerManager.IsTimerActive(TH_CollisionLaunchFreeze)) 
		return false;
	if (m_State != EEnemyState::STATE_Launched)
		return false;
	return true;
}
void ASmallEnemy::ReflectedCollisionLaunch_PreLaunch(FVector SurfaceNormal, FVector SurfaceLocation, bool bAlwaysFreeze)
{
	if (!GetWorld()) return;
	m_GravityState = EGravityState::ForcedNone;
	m_State = EEnemyState::STATE_PreLaunched;

	CollisionLaunchDirection = GetCollisionDirection(SurfaceNormal);
	float time{};
	float Multiplier{};
	GetCollisionTime(time, Multiplier);
	TlComp_LaunchedCollision->PlayFromStart();

	if (TimerManager.IsTimerActive(TH_FreezeCollisionLaunchCooldown) && !bAlwaysFreeze)
		ReflectedCollisionLaunch_Launch();
	else
	{
		TimerManager.SetTimer(TH_CollisionLaunchFreeze, this, &ASmallEnemy::ReflectedCollisionLaunch_Launch, time);
		TimerManager.SetTimer(TH_FreezeCollisionLaunchCooldown, LaunchedCollision_FreezeCooldown + time, false);
	}
	LC_SpawnEffect_Pure(time, FMath::Max(Multiplier, 0.4f), SurfaceNormal, SurfaceLocation);
}

void ASmallEnemy::ReflectedCollisionLaunch_Launch()
{
	m_GravityState = EGravityState::Default;
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(CollisionLaunchDirection * LaunchedCollision_VelocityMultiplier + (FVector(0, 0, GetCharacterMovement()->GetGravityZ() * GetWorld()->GetDeltaSeconds())), true);
	Launched(CollisionLaunchDirection.GetSafeNormal());
}

void ASmallEnemy::LaunchedLandCollision_PreLaunch(FVector SurfaceNormal, FVector SurfaceLocation)
{
	if (!GetWorld()) return;
	m_GravityState = EGravityState::ForcedNone;
	m_State = EEnemyState::STATE_PreLaunched;

	CollisionLaunchDirection = GetCollisionDirection(SurfaceNormal);
	float time{};
	float Multiplier{};
	GetCollisionTime(time, Multiplier);
	TlComp_LaunchedCollision->PlayFromStart();

	TimerManager.SetTimer(TH_CollisionLaunchFreeze, this, &ASmallEnemy::LaunchedLandCollision_Launch, time);
	TimerManager.SetTimer(TH_FreezeCollisionLaunchCooldown, LaunchedCollision_FreezeCooldown + time, false);

	LC_SpawnEffect_Pure(time, FMath::Max(Multiplier, 0.4f), SurfaceNormal, SurfaceLocation);
}

void ASmallEnemy::LaunchedLandCollision_Launch()
{
	m_GravityState = EGravityState::Default;
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(CollisionLaunchDirection * LaunchedGroundCollision_VelocityMultiplier + (FVector(0, 0, GetCharacterMovement()->GetGravityZ() * GetWorld()->GetDeltaSeconds())), true);
	Launched(CollisionLaunchDirection.GetSafeNormal());
}

FVector ASmallEnemy::GetCollisionDirection(const FVector& SurfaceNormal)
{
	FVector Velocity = -GetCharacterMovement()->GetLastUpdateVelocity().GetSafeNormal();
	FVector Proj = Velocity - (FVector::DotProduct(SurfaceNormal, Velocity) * SurfaceNormal);
	float CurrentVelocitySize = GetCharacterMovement()->GetLastUpdateVelocity().Length();
	return (Velocity + (Proj * -2.f)) * CurrentVelocitySize;
}

void ASmallEnemy::GetCollisionTime(float& OUT_Time, float& OUT_Multiplier)
{
	OUT_Time = LaunchedCollision_FreezeBaseTime;
	OUT_Multiplier = (FMath::Min(GetCharacterMovement()->GetLastUpdateVelocity().SquaredLength(), FMath::Square(LaunchedCollision_FreezeVelocityMultiplier)) / FMath::Square(LaunchedCollision_FreezeVelocityMultiplier));
	OUT_Multiplier = SMath::InvertedGaussian(OUT_Multiplier, 5.3f, 3.5f);
	OUT_Time *= OUT_Multiplier;
}

void ASmallEnemy::DogToDogCollision(const FHitResult& SweepHit, ASmallEnemy* OtherDog)
{
	ReflectedCollisionLaunch_PreLaunch(SweepHit.ImpactNormal, SweepHit.ImpactPoint, true);
	OtherDog->ReflectedCollisionLaunch_PreLaunch(-SweepHit.ImpactNormal, SweepHit.ImpactPoint, true);
}

void ASmallEnemy::GettingDogCollision(FVector SurfaceNormal, FVector SurfaceLocation)
{
}

bool ASmallEnemy::DogEnvironmentCollision(const FHitResult& SweepHit)
{
	AActor* OtherActor = SweepHit.GetActor();
	if (OtherActor == this) return true;
	if (IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor))
	{
		if (ITag->HasMatchingGameplayTag(Tag::Player()))				return true;
		if (ITag->HasMatchingGameplayTag(Tag::BouncyShroom()))			return true;
		if (ITag->HasMatchingGameplayTag(Tag::EnvironmentHazard()))		return true;
		if (ITag->HasMatchingGameplayTag(Tag::Enemy()))
		{
			if (CanReflectCollisionLaunch())
				if (ASmallEnemy* Dog = Cast<ASmallEnemy>(OtherActor))
					DogToDogCollision(SweepHit, Dog);
			return true;
		}
		if (ITag->HasMatchingGameplayTag(Tag::CorruptionCore()) || ITag->HasMatchingGameplayTag(Tag::InkFlower()) || ITag->HasMatchingGameplayTag(Tag::EnemySpawner()))
		{
			if (CanReflectCollisionLaunch()) {
				float Time{};
				float m{};
				GetCollisionTime(Time, m);
				if (IAttackInterface* IAttack = Cast<IAttackInterface>(OtherActor)) {
					IAttack->Gen_ReceiveAttack(SweepHit.ImpactNormal, 1000.f, EAttackType::Environmental, Time);
				}
			}
		}
	}
	return false;
}

void ASmallEnemy::CancelCollisionLaunch()
{
	TimerManager.ClearTimer(TH_CollisionLaunchFreeze);
	TimerManager.ClearTimer(TH_FreezeCollisionLaunchCooldown);
}

void ASmallEnemy::LC_SpawnEffect_Pure(float Time, float VelocityMultiplier, FVector SurfaceNormal, FVector SurfaceLocation)
{
	LC_GetEffectLocation(SurfaceLocation, SurfaceNormal);
	LC_SpawnEffect(Time, VelocityMultiplier, SurfaceNormal, SurfaceLocation);
}

void ASmallEnemy::LC_GetEffectLocation(FVector& SurfaceLocation, FVector& SurfaceNormal)
{
	float length = 100.f;
	FHitResult Hit;
	FCollisionQueryParams Params("", true, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, SurfaceLocation + (SurfaceNormal * length), SurfaceLocation + (SurfaceNormal * -length), ECC_Visibility, Params))
	{
		SurfaceLocation = Hit.ImpactPoint;
		SurfaceNormal = Hit.ImpactNormal;
	}	
}

void ASmallEnemy::SpottingPlayer_Begin()
{
	m_Anim->bSpottingPlayer = true;
}
void ASmallEnemy::SpottingPlayer_End()
{
	m_Anim->bSpottingPlayer = false;
}

bool ASmallEnemy::CanAttack_Pawn() const
{
	if (m_State != EEnemyState::STATE_OnGround) return false;
	if (bIsHooked) return false;
	return true;
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
	Anim_Attacked();
	Execute_Hooked(this);
}

void ASmallEnemy::HookedPure(const FVector InstigatorLocation, bool OnGround, bool PreAction /*=false*/)
{
	/* During Pre Action, Rotate Actor towards instigator - Yaw */
	if (PreAction)
	{
		bIsHooked = true;
		//AI
		IncapacitateUndeterminedTime(EAIIncapacitatedType::Grappled, &ASmallEnemy::Capacitate_Grappled);
		Cancel_CHOMP();
		CancelCollisionLaunch();
		FVector Direction = InstigatorLocation - GetActorLocation();
		RotateActorYawToVector(Direction.GetSafeNormal());
		m_State = EEnemyState::STATE_Grappled;
		m_GravityState = EGravityState::ForcedNone;
		Anim_Attacked();
		// If stunned, delete the stunned effect
		if (TimerManager.IsTimerActive(TH_RedetermineIncapacitate)) {
			DeleteStunnedEffect();
		}
		return;
	}

	if (bCanBeGrappleHooked)
	{
		// Enable Gravity and Disable Collisions
		m_State = EEnemyState::STATE_Launched;
		m_GravityState = EGravityState::Default;
		Cancel_AttackContact();
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		DisableCollisions(FMath::Clamp(GrappledLaunchTime - GrappledLaunchTime_CollisionActivation, 0.f, GrappledLaunchTime));
		IncapacitateUndeterminedTime(EAIIncapacitatedType::Grappled, &ASmallEnemy::Capacitate_Grappled);

		/* Rotate again towards Instigator - Yaw*/
		GrappleLaunchToInstigator(InstigatorLocation, GrappledLaunchTime, OnGround);

		bCanBeGrappleHooked = false;
		TimerManager.SetTimer(Handle_GrappledCooldown, this, &ASmallEnemy::ResetCanBeGrappleHooked, GrappleHookedInternalCooldown);
		TimerManager.SetTimer(TH_UncheckIsHooked, [this]() { bIsHooked = false; }, 1.f, false);

		//
		if (m_DogPack)
			m_DogPack->AlertPack(this);
		SleepingEnd();

		Delegate_StunnedLand.BindUObject(this, &ASmallEnemy::StunnedLand);
		if (!OnGround)
			Delegate_LaunchedLand.BindUObject(this, &ASmallEnemy::LandedLaunched);
	}
}

void ASmallEnemy::UnHookedPure()
{
	Execute_UnHooked(this);
	Incapacitate(EAIIncapacitatedType::Stunned, 0.1f);
	m_GravityState = EGravityState::Default;
}

void ASmallEnemy::PullFree_Pure(const FVector InstigatorLocation)
{
	m_State = EEnemyState::STATE_InAir;
	
	if (bStartStuckToWall) {
		TimerManager.ClearTimer(TH_SpecialStartWorldLocation);
		bStartStuckToWall = false;
	}

	PullFree_Launch(InstigatorLocation);
	LeaveWall();

	// Enable Gravity and Disable Collisions
	m_GravityState = EGravityState::Default;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); }, Grappled_PulledFreeNoCollisionTimer, false);

	// Internal cooldown to getting grapplehooked
	bCanBeGrappleHooked = false;
	TimerManager.SetTimer(Handle_GrappledCooldown, this, &ASmallEnemy::ResetCanBeGrappleHooked, GrappleHookedInternalCooldown);

	Delegate_StunnedLand.BindUObject(this, &ASmallEnemy::StunnedLand);
	Delegate_LaunchedLand.BindUObject(this, &ASmallEnemy::LandedLaunched);
	Launched(FVector(InstigatorLocation - GetActorLocation()).GetSafeNormal());
}

void ASmallEnemy::PullFree_Launch(const FVector& InstigatorLocation)
{
	FVector Direction = InstigatorLocation - GetActorLocation();
	float Dotproduct_Up = FVector::DotProduct(FVector::UpVector, Direction.GetSafeNormal());
	GrappleLaunchToInstigator(InstigatorLocation, GrappledLaunchTime * 4.f, true);
}

void ASmallEnemy::GrappleLaunchToInstigator(FVector InstigatorLocation, float Time, bool OnGround)
{
	/* Rotate again towards Instigator - Yaw*/
	FVector InstigatorDirection = (InstigatorLocation - FVector(0, 0, GrappledInstigatorOffset)) - GetActorLocation();
	FVector GoToLocation = InstigatorLocation + (InstigatorDirection.GetSafeNormal2D() * -GrappledInstigatorOffset);
	FVector Direction = GoToLocation - GetActorLocation();
	RotateActorYawToVector(InstigatorDirection);

	FVector Velocity{};
	OnGround ?
		Velocity = ((Direction) / Time) + (0.5f * FVector(0, 0, -m_BaseGravityZ) * Time) :		// On Ground
		Velocity = ((InstigatorDirection) / Time);												// In Air

	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(Velocity, true);
}

bool ASmallEnemy::CanBeAttacked()
{
	return false;
}

void ASmallEnemy::Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	/**
	** Check if actor has component
	 *	is this the best method of creating modular game mechanics?
	 *  Don't know the cost of actually checking for and actor component vs casting to an interface. 
	** Or is Interface better? 
	 *  Issue with interfaces would be the multiple inheritance on the classes, which is bad for reasons beyond me atm
	 */
}

void ASmallEnemy::Receive_SmackAttack_Pure(const FVector Direction, const float Strength, const bool bOverrideStrength)
{
	if (!GetCanBeSmackAttacked()) return;
	/* Sets a timer before character can be damaged by the same attack */
	TimerManager.SetTimer(THandle_GotSmackAttacked, this, &ASmallEnemy::ResetCanBeSmackAttacked, SmackAttack_InternalTimer, false);
	DisableCollisions(0.1f);
	EnableGravity();
	/* Weaker smack attack if actor on ground than in air */
	float s = Strength;
	if (!bOverrideStrength)
		if (GetCharacterMovement()->IsWalking()) 
			s = Strength * SmackAttack_OnGroundMultiplication;

	bCanBeSmackAttacked = false;
	SetActorRotation(FVector(Direction.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(Direction * s, true);

	Incapacitate(EAIIncapacitatedType::Stunned);
	Launched(Direction);

	if (m_DogPack)
		m_DogPack->AlertPack(this);
	SleepingEnd();

	Delegate_StunnedLand.BindUObject(this, &ASmallEnemy::StunnedLand);
	Delegate_LaunchedLand.BindUObject(this, &ASmallEnemy::LandedLaunched);

	// If stunned, delete the stunned effect
	if (TimerManager.IsTimerActive(TH_RedetermineIncapacitate)) {
		DeleteStunnedEffect();
	}
}
void ASmallEnemy::Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength)
{
	EnableGravity();

	SetActorRotation(FVector(PoundDirection.GetSafeNormal2D() * -1.f).Rotation(), ETeleportType::TeleportPhysics);
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse(PoundDirection * GP_Strength, true);

	/* Sets a timer before character can be damaged by the same attack */
	Launched(PoundDirection);

	if (m_DogPack)
		m_DogPack->AlertPack(this);
	SleepingEnd();

	Delegate_StunnedLand.BindUObject(this, &ASmallEnemy::StunnedLand);
	Delegate_LaunchedLand.BindUObject(this, &ASmallEnemy::LandedLaunched);

	// If stunned, delete the stunned effect
	if (TimerManager.IsTimerActive(TH_RedetermineIncapacitate)) {
		DeleteStunnedEffect();
	}
}

void ASmallEnemy::Receive_LeewayPause_Pure(float Pausetime)
{
	if (m_State == EEnemyState::STATE_Grappled) return;
	CustomTimeDilation = AttackInterface_LeewayPause_Timedilation;
	if (GetWorld())
		GetWorldTimerManager().SetTimer(TH_IAttack_LeewayPause, [this]() { if(GetWorld() && this)CustomTimeDilation = 1.f; }, Pausetime, false);
}

void ASmallEnemy::ChompingAnotherEnemy(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	FVector Direction2D = FVector(OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	FVector NewDirection = (FVector::UpVector * ChompOther_Angle) + (Direction2D * (1.f - FMath::Abs(ChompOther_Angle)));
	OtherInterface->Receive_SmackAttack_Pure(NewDirection, ChompOther_Strength, true);
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
	// Don't detach from the wall if it detects a sticky wall below
	if (WallDetector->DetectStickyWallOnNormalWithinAngle(GetActorLocation(), 0.9f, FVector::UpVector))
		return;

	FVector Direction = FVector::DownVector;
	if (IsStuck_Pure())
		Direction = (FVector::DownVector * (1.f - PB_Groundpound_LaunchWallNormal)) + (m_WallData.Normal * PB_Groundpound_LaunchWallNormal);
	LeaveWall();
	m_GravityState = EGravityState::Default;
	m_State = EEnemyState::STATE_None;

	GetCharacterMovement()->AddImpulse(Direction * PB_Groundpound_LaunchStrength, true);
	Incapacitate(EAIIncapacitatedType::Stunned, PB_Pogo_Groundpound_Stunduration);
	Execute_Receive_Pogo_GroundPound(this);
	m_Anim->bPogoedOn = true;
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { m_Anim->bPogoedOn = false; }, 0.5f, false);
}

void ASmallEnemy::IA_Receive_Pogo_Pure()
{
	Incapacitate(EAIIncapacitatedType::Stunned, PB_Pogo_Passive_Stunduration);
	Execute_IA_Receive_Pogo(this);
	m_Anim->bPogoedOn = true;
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { m_Anim->bPogoedOn = false; }, 0.5f, false);
}

void ASmallEnemy::PrintState()
{
	FString s = "State: ";
	switch (m_State)
	{
	case EEnemyState::STATE_None:
		s += "None";
		break;
	case EEnemyState::STATE_OnGround:
		s += "On Ground";
		break;
	case EEnemyState::STATE_InAir:
		s += "In Air";
		break;
	case EEnemyState::STATE_Launched:
		s += "Launched";
		break;
	case EEnemyState::STATE_OnWall:
		s += "On Wall";
		break;
	default:
		break;
	}
	PRINTPAR("%s", *s);
}

void ASmallEnemy::Launched()
{
	EnableGravity();
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

void ASmallEnemy::LandedLaunched(const FHitResult& LandHit)
{
	if (DogEnvironmentCollision(LandHit)) return;
	LaunchedLandCollision_PreLaunch(LandHit.ImpactNormal, LandHit.ImpactPoint);
}
