// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharacter.h"
#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "../GameplayTags.h"
#include "Components/SplineComponent.h"
#include "../StaticActors/Collectible.h"
#include "../StaticActors/PlayerRespawn.h"


// Sets default values
ASteikemannCharacter::ASteikemannCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<USteikemannCharMovementComponent>(ACharacter::CharacterMovementComponentName))
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	GrappleHookMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("GrappleHook Mesh"));
	GrappleHookMesh->SetupAttachment(RootComponent);

	GrappleTargetingDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Grapple Targeting Detection Sphere"));
	GrappleTargetingDetectionSphere->SetupAttachment(GetCapsuleComponent());
	GrappleTargetingDetectionSphere->SetGenerateOverlapEvents(false);
	GrappleTargetingDetectionSphere->SetSphereRadius(0.1f);

	Component_Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	Component_Audio->SetupAttachment(RootComponent);

	Component_Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	Component_Niagara->SetupAttachment(RootComponent);


	AttackCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollider"));
	AttackCollider->SetupAttachment(GetMesh(), "Stav_CollisionSocket");
	AttackCollider->SetGenerateOverlapEvents(false);

	GroundPoundCollider = CreateDefaultSubobject<USphereComponent>(TEXT("GroundPoundCollider"));
	GroundPoundCollider->SetupAttachment(RootComponent);
	GroundPoundCollider->SetGenerateOverlapEvents(false);
	GroundPoundCollider->SetSphereRadius(0.1f);

	WallDetector = CreateDefaultSubobject<UWallDetectionComponent>(TEXT("WallDetector"));
}

// Called when the game starts or when spawned
void ASteikemannCharacter::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = Cast<USteikemannCharMovementComponent>(GetCharacterMovement());
	PlayerController = Cast<APlayerController>(GetController());
	Base_CameraBoomLength = CameraBoom->TargetArmLength;
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnCapsuleComponentBeginOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ASteikemannCharacter::OnCapsuleComponentEndOverlap);
	MaxHealth = Health;
	StartTransform = GetActorTransform();
	m_BaseGravity = MovementComponent->GetGravityZ();
	ResetState();

	/* Creating Niagara Compnents */
	NiComp_CrouchSlide = CreateNiagaraComponent("Niagara_CrouchSlide", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
	if (NiComp_CrouchSlide) {
		NiComp_CrouchSlide->bAutoActivate = false;
		if (NS_CrouchSlide) { NiComp_CrouchSlide->SetAsset(NS_CrouchSlide); }
	}
	
	
	/* Attack Collider */
	AttackCollider->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnAttackColliderBeginOverlap);
	AttackColliderScale = AttackCollider->GetRelativeScale3D();
	AttackCollider->SetRelativeScale3D(FVector(0));
		

	GroundPoundCollider->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnAttackColliderBeginOverlap);
	//GroundPoundColliderScale = GroundPoundCollider->GetRelativeScale3D();
	
	/* Grapple Targeting Detection Sphere */
	GrappleTargetingDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnGrappleTargetDetectionBeginOverlap);
	GrappleTargetingDetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ASteikemannCharacter::OnGrappleTargetDetectionEndOverlap);

	GrappleTargetingDetectionSphere->SetGenerateOverlapEvents(true);
	GrappleTargetingDetectionSphere->SetSphereRadius(GrappleHookRange);

	WallDetector->SetCapsuleSize(WDC_Capsule_Radius, WDC_Capsule_Halfheight);
	WallDetector->SetDebugStatus(bWDC_Debug);
	WallDetector->SetHeight(Wall_HeightCriteria, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	WallDetector->SetMinLengthToWall(WDC_Length);
	/*
	* Adding GameplayTags to the GameplayTagsContainer
	*/
	GameplayTags.AddTag(Tag::Player());
	mFocusPoints.Empty();
}

UNiagaraComponent* ASteikemannCharacter::CreateNiagaraComponent(FName Name, USceneComponent* Parent, FAttachmentTransformRules AttachmentRule, bool bTemp /*= false*/)
{
	UNiagaraComponent* TempNiagaraComp = NewObject<UNiagaraComponent>(this, Name);
	TempNiagaraComp->AttachToComponent(Parent, AttachmentRule);
	TempNiagaraComp->RegisterComponent();

	if (bTemp) { TempNiagaraComponents.Add(TempNiagaraComp); } // Adding as temp comp

	return TempNiagaraComp;
}

void ASteikemannCharacter::NS_Land_Implementation(const FHitResult& Hit)
{
	//bool groundpound{};
	//if (IsGroundPounding()) {
	//	GroundPoundLand(Hit);
	//	m_EAttackState = EAttackState::None;
	//	ResetState();
	//	groundpound = true;
	//}

	/* Play Landing particle effect */
	float Velocity = GetVelocity().Size();
	UNiagaraComponent* NiagaraPlayer{ nullptr };

	if (Component_Niagara->IsComplete())
	{
		NiagaraPlayer = Component_Niagara;
	}
	else
	{
		//PRINTLONG("Niagara Component not yet completed with its task. Creating temp Niagara Component.");

		UNiagaraComponent* TempNiagaraLand = CreateNiagaraComponent("Niagara_Land", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, true);
		NiagaraPlayer = TempNiagaraLand;
	}

	NiagaraPlayer->SetAsset(NS_Land);
	NiagaraPlayer->SetNiagaraVariableInt("User.SpawnAmount", static_cast<int>(Velocity * NSM_Land_ParticleAmount));
	NiagaraPlayer->SetNiagaraVariableFloat("User.Velocity", IsGroundPounding() ? Velocity * 3.f * NSM_Land_ParticleSpeed : Velocity * NSM_Land_ParticleSpeed);	// Change pending on character is groundpounding or not
	NiagaraPlayer->SetWorldLocationAndRotation(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	NiagaraPlayer->Activate(true);
}


// Called every frame
void ASteikemannCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/* SHOW COLLECTIBLES */
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, FString::Printf(TEXT("Common : %i"), CollectibleCommon));
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, FString::Printf(TEXT("CorruptionCore : %i"), CollectibleCorruptionCore));
	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Red, FString::Printf(TEXT("Health : %i"), Health), true, FVector2D(3));

	if (IsDead()) { return; }

	/* Rotate Inputvector to match the playercontroller */
	{
		InputVector = InputVectorRaw;
		FRotator Rot = GetControlRotation();
		InputVector = InputVector.RotateAngleAxis(Rot.Yaw, FVector::UpVector);

		if (InputVectorRaw.Size() > 1.f || InputVector.Size() > 1.f)
			InputVector.Normalize();
	}
	switch (m_EState)
	{
	case EState::STATE_OnGround:
		PRINT("STATE_OnGround");
		break;
	case EState::STATE_InAir:
		PRINT("STATE_InAir");
		break;
	case EState::STATE_OnWall:
		PRINT("STATE_OnWall");
		break;
	case EState::STATE_Attacking:
		PRINT("STATE_Attacking");
		break;
	case EState::STATE_Grappling:
		PRINT("STATE_Grappling");
		break;
	default:
		break;
	}

	PRINTPAR("Attack State :: %i", m_EAttackState);
	PRINTPAR("Air State :: %i", m_EAirState);
	PRINTPAR("Pogo Type :: %i", m_EPogoType);

	/*		Resets Rotation Pitch and Roll		*/
	if (IsFalling() || GetMoveComponent()->IsWalking()) {
		ResetActorRotationPitchAndRoll(DeltaTime);
	}

	
	/*		Jump		*/
	PostEdge_JumpTimer += DeltaTime;
	if (GetMoveComponent()->IsFalling() && (PostEdge_JumpTimer < PostEdge_JumpTimer_Length))
	{
		bCanPostEdgeRegularJump = true;
	}
	else if (GetMoveComponent()->IsFalling() && (PostEdge_JumpTimer > PostEdge_JumpTimer_Length))
	{
		bCanPostEdgeRegularJump = false;
	}
	if (GetCharacterMovement()->IsWalking()) { PostEdge_JumpTimer = 0.f; }


	/*------------ BASIC STATES ---------------*/
	SetDefaultState();

	/*------------ STATES ---------------*/
	const bool wall = WallDetector->DetectWall(this, GetActorLocation(), GetActorForwardVector(), m_Walldata, m_WallJumpData);
	switch (m_EState)
	{
	case EState::STATE_OnGround:
	{
		if (m_EAttackState == EAttackState::GroundPound)
			PB_Exit();
		break;
	}
	case EState::STATE_InAir:
	{
		switch (m_EPogoType)
		{
		case EPogoType::POGO_None:
			break;
		case EPogoType::POGO_Passive:
			PB_Passive_IMPL(m_PogoTarget);
			break;
		case EPogoType::POGO_Active:
			PRINTLONG("Tick: POGO ACTIVE");
			PB_Active_IMPL();
			break;
		default:
			break;
		}

		if (m_EAirState == EAirState::AIR_Pogo) break;
		if (wall && m_WallState == EOnWallState::WALL_None)
		{
			// Detect ledge & Ledge Grab
			if (WallDetector->DetectLedge(m_Ledgedata, this, GetActorLocation(), GetActorUpVector(), m_Walldata, LedgeGrab_Height, LedgeGrab_Inwards))
			{
				//if (Validate_Ledge()) break;
				GetMoveComponent()->m_WallJumpData = m_WallJumpData;
				Initial_LedgeGrab();
				break;	
			}

			// Hang on wall -- Default
			if (ValidateWall() && m_WallJumpData.valid)
			{
				GetMoveComponent()->Initial_OnWall_Hang(m_WallJumpData, OnWall_HangTime);
				m_EState = EState::STATE_OnWall;
				m_WallState = EOnWallState::WALL_Hang;
			}
		}
		break;
	}
	case EState::STATE_OnWall:
	{
		if (wall || WallDetector->DetectLedge(m_Ledgedata, this, GetActorLocation(), GetActorUpVector(), m_Walldata, LedgeGrab_Height, LedgeGrab_Inwards))
			OnWall_IMPL(DeltaTime);
		else {
			ExitOnWall(EState::STATE_InAir);
			GetMoveComponent()->m_WallState = EOnWallState::WALL_None;
			GetMoveComponent()->m_GravityMode = EGravityMode::LerpToDefault;
		}

		break;
	}
	case EState::STATE_Attacking:
		
		switch (m_EAttackState)
		{
		case EAttackState::None:
			break;
		case EAttackState::Smack:
			break;
		case EAttackState::Scoop:
			break;
		case EAttackState::GroundPound:
			if (m_EPogoType == EPogoType::POGO_Groundpound) 
			{
				PRINTLONG("Tick: POGO GROUND POUND");
				PB_Groundpound_IMPL(m_PogoTarget);
				break;
			}
			break;
		default:
			break;
		}
		//if (m_EAttackState == EAttackState::GroundPound)
			//bPB_Groundpound_PredeterminedPogoHit = PB_Groundpound_Predeterminehit();
		break;
	case EState::STATE_Grappling:
		if (m_EPogoType == EPogoType::POGO_Passive)
		{
			PB_Passive_IMPL(m_PogoTarget);
			break;
		}
		break;
	default:
		break;
	}


	/* --------- GRAPPLE TARGETING -------- */
	GH_GrappleAiming();
	
	/* Checking TempNiagaraComponents array if they are completed, then deleting them if they are */
	if (TempNiagaraComponents.Num() > 0)
	{
		for (int i = 0; i < TempNiagaraComponents.Num(); i++)
		{
			if (TempNiagaraComponents[i]->IsComplete())
			{
				//PRINTLONG("Removing completed TempNiagaraComponent");
				TempNiagaraComponents[i]->DestroyComponent();
				TempNiagaraComponents.RemoveAt(i);
				i--;
			}
		}
	}

	/*	
	*	POGO BOUNCE
	* 
	* Raytrace beneath player to see if they hit an enemy
	*/
	//if ((GetMoveComponent()->IsFalling() || IsJumping()) && GetMoveComponent()->Velocity.Z < 0.f)
	//{
	//	FHitResult Hit{};
	//	FCollisionQueryParams Params{ "", false, this };
	//	const bool b = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + FVector::DownVector * 500.f, ECC_PogoCollision, Params);
	//	if (b)
	//	{
	//		PB_TargetBeneath(Hit);
	//	}
	//}

	if (m_EState == EState::STATE_Grappling && m_EGrappleType == EGrappleType::Dynamic_Ground) {
		GrappleDynamicGuideCamera(DeltaTime);
	}

	GuideCamera(DeltaTime);

	/* ----------------------- COMBAT TICKS ------------------------------ */
	PreBasicAttackMoveCharacter(DeltaTime);
	SmackAttackMoveCharacter(DeltaTime);
	ScoopAttackMoveCharacter(DeltaTime);
	// Hack method of forcing camera to not freak out during attack movements
	//if (bPreBasicAttackMoveCharacter || bSmackAttackMoveCharacter || bScoopAttackMoveCharacter) {	
		//CameraBoom->bDoCollisionTest = false;
	//} else { CameraBoom->bDoCollisionTest = true; }

}

// Called to bind functionality to input
void ASteikemannCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	

	/* Basic Movement */
		/* Movement control */
			/* Gamepad and Keyboard */
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ASteikemannCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ASteikemannCharacter::MoveRight);
		/* Looking control */
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this,		&APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this,		&APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this,	&ASteikemannCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up/Down Gamepad", this,		&ASteikemannCharacter::LookUpAtRate);
			/* Dualshock */

		/* Jump */
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASteikemannCharacter::Jump).bConsumeInput = true;
	//PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::StopJumping).bConsumeInput = true;
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::JumpRelease).bConsumeInput = true;

	/* -- SLIDE -- */
	PlayerInputComponent->BindAction("Slide", IE_Pressed, this, &ASteikemannCharacter::Click_Slide);
	PlayerInputComponent->BindAction("Slide", IE_Released, this, &ASteikemannCharacter::UnClick_Slide);
	

	/* -- GRAPPLEHOOK -- */
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Pressed, this,	  &ASteikemannCharacter::RightTriggerClick);
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Released, this,	  &ASteikemannCharacter::RightTriggerUn_Click);


	/* -- SMACK ATTACK -- */
	PlayerInputComponent->BindAction("SmackAttack", IE_Pressed, this, &ASteikemannCharacter::Click_Attack);
	PlayerInputComponent->BindAction("SmackAttack", IE_Released, this, &ASteikemannCharacter::UnClick_Attack);

	/* -- SCOOP ATTACK -- */
	PlayerInputComponent->BindAction("ScoopAttack", IE_Pressed, this, &ASteikemannCharacter::Click_ScoopAttack);
	PlayerInputComponent->BindAction("ScoopAttack", IE_Released, this, &ASteikemannCharacter::UnClick_ScoopAttack);

	/* Attack - GroundPound */
	PlayerInputComponent->BindAction("GroundPound", IE_Pressed, this, &ASteikemannCharacter::Click_GroundPound);
	PlayerInputComponent->BindAction("GroundPound", IE_Released, this, &ASteikemannCharacter::UnClick_GroundPound);
}


void ASteikemannCharacter::GH_GrappleAiming()
{
	FVector Forward = GetControlRotation().Vector();
	float TargetDotProd{ -1.f };
	AActor* Target{ nullptr };

	for (auto it : InReachGrappleTargets)
	{
		/* Skip iteration if it == Active_GrappledActor, to not interrupt the grapplehook post_launch period */
		if (it == Active_GrappledActor) { continue; }

		FVector ToActor = it->GetActorLocation() - CameraBoom->GetComponentLocation();
		float DotProd = FVector::DotProduct(Forward, ToActor.GetSafeNormal());
		if (DotProd > TargetDotProd)
		{
			TargetDotProd = DotProd;
			Target = it;
		}

		IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(it);
		if (GrappleInterface)
		{
			GrappleInterface->InReach_Pure();
		}
	}

	if ((Target == Active_GrappledActor || GrappledActor == Active_GrappledActor) && Active_GrappledActor.IsValid())
	{
		return;
	}

	/* If the target is behind the player, return and Untarget GrappledActor */
	if (TargetDotProd < 0)
	{
		/* If there is a target already, untarget */
		if (GrappledActor.IsValid())
		{
			IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
			if (GrappleInterface)
			{
				/* If still within range, set to InReach */
				GrappleInterface->InReach_Pure();
			}
		}

		GrappledActor = nullptr;
		return;
	}


	/* Else if target is in front of the player */
	if (!Target) { return; }

	/* If target is not GrappledActor. UnTarget GrappledActor */
	if (Target != GrappledActor)
	{
		IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
		if (GrappleInterface)
		{
			/* If still within range, set to InReach */	// Denne tingen trengs kanskje ikke 
			if (InReachGrappleTargets.Contains(GrappledActor))
			{
				GrappleInterface->InReach_Pure();
			}
			else
			{
				GrappleInterface->OutofReach_Pure();
			}
		}

		GrappleInterface = Cast<IGrappleTargetInterface>(Target);
		if (GrappleInterface && m_EState != EState::STATE_Grappling)
		{
			GrappleInterface->TargetedPure();
		}

		GrappledActor = Target;
	}
	else
	{
		IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
		if (GrappleInterface && m_EState != EState::STATE_Grappling)
		{
			GrappleInterface->TargetedPure();
		}
		return;
	}
}

void ASteikemannCharacter::RightTriggerClick()
{
	switch (m_EState)
	{
	case EState::STATE_None:		break;
	case EState::STATE_OnGround:	break;
	case EState::STATE_InAir:		break;
	case EState::STATE_OnWall:		
		CancelOnWall();
		break;
	case EState::STATE_Attacking:	return;
	case EState::STATE_Grappling:	return;
	default:
		break;
	}

	if (!GrappledActor.IsValid()) return;
	Active_GrappledActor = GrappledActor;
	Active_GrappledActor_Location = Active_GrappledActor->GetActorLocation();

	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(Active_GrappledActor.Get());
	IGrappleTargetInterface* IGrapple = Cast<IGrappleTargetInterface>(Active_GrappledActor.Get());
	if (!ITag || !IGrapple) return;

	JumpCurrentCount = 1;	// Reset DoubleJump

	GH_SetGrappleType(ITag, IGrapple);

	//---------------------------------------------------------
	
	
	//if (IsGrappling() || IsPostGrapple()) { return; }

	//if (GrappledActor.IsValid() && !Active_GrappledActor.IsValid())
	//{
	//	IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(GrappledActor.Get());
	//	IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
	//	if (TagInterface && GrappleInterface)
	//	{
	//		Active_GrappledActor = GrappledActor;
	//
	//		/* Checking tags to determine if the grapple target is dynamic or static */
	//		FGameplayTagContainer GrappledTags;
	//		TagInterface->GetOwnedGameplayTags(GrappledTags);
	//
	//
	//		/* Grappling to Enemy/Dynamic Target */
	//		//if (GrappledTags.HasTag(Tag_GrappleTarget_Dynamic))	// Burde bruke denne taggen på fiendene
	//		if (GrappledTags.HasTag(Tag::AubergineDoggo()))		// Også spesifisere videre med fiende type
	//		{
	//			if (GrappleInterface->IsStuck_Pure()) {
	//				bGrapplingStaticTarget = true;
	//			}
	//			else {
	//				bGrapplingDynamicTarget = true;
	//				InitialGrappleDynamicZ = GrappledActor->GetActorLocation().Z;
	//				if (IsOnGround())
	//					bGrapplingDynamicTarget_CameraGuide = true;
	//			}
	//		}
	//		/* Grappling to Static Target */
	//		else if (GrappledTags.HasTag(Tag::GrappleTarget_Static()))
	//		{
	//			bGrapplingStaticTarget = true;
	//		}
	//
	//		Initial_GrappleHook();	// TODO: Forandre til to ulike grappling funksjoner, basert på Static og Dynamisk Target
	//	}
	//}
}

void ASteikemannCharacter::RightTriggerUn_Click()
{
	// Noe angående slapp grapple knappen
	//bGrapplingDynamicTarget = false;
}

void ASteikemannCharacter::GH_SetGrappleType(IGameplayTagAssetInterface* ITag, IGrappleTargetInterface* IGrapple)
{
	FGameplayTagContainer tags;
	ITag->GetOwnedGameplayTags(tags);

	// Static Target
	if (tags.HasTag(Tag::GrappleTarget_Static()))
	{
		m_EGrappleType = EGrappleType::Static;
		GH_PreLaunch_Static(&ASteikemannCharacter::GH_Launch_Static, IGrapple);
		return;
	}

	// Dynamic Target
	if (!tags.HasTag(Tag::GrappleTarget_Dynamic()))
		return;

		// Dynamic Target Stuck
	if (IGrapple->IsStuck_Pure())
	{
		ResetState();
		if (m_EState == EState::STATE_OnGround)	m_EGrappleType = EGrappleType::Static_StuckEnemy_Ground;
		if (m_EState == EState::STATE_InAir)		m_EGrappleType = EGrappleType::Static_StuckEnemy_Air;
		GH_PreLaunch_Static(&ASteikemannCharacter::GH_Launch_Static_StuckEnemy, IGrapple);
		return;
	}

		// Player in Air
	if (m_EState == EState::STATE_InAir)
	{
		m_EGrappleType = EGrappleType::Dynamic_Air;
		GH_PreLaunch_Dynamic(IGrapple, false);
		return;
	}

		// Player on Ground
	if (m_EState == EState::STATE_OnGround)
	{
		m_EGrappleType = EGrappleType::Dynamic_Ground;
		GH_PreLaunch_Dynamic(IGrapple, true);
		return;
	}
}

void ASteikemannCharacter::GH_PreLaunch_Static(void(ASteikemannCharacter::* LaunchFunction)(), IGrappleTargetInterface* IGrapple)
{
	GH_PreLaunch();
	IGrapple->HookedPure();

	//FTimerHandle h;
	GetWorldTimerManager().SetTimer(TH_Grapplehook_Pre_Launch,
		[this]()
		{
			m_EGrappleState = EGrappleState::Post_Launch;
			GetMoveComponent()->m_GravityMode = EGravityMode::Default;
		},
		GrappleDrag_PreLaunch_Timer_Length, false);
	GetWorldTimerManager().SetTimer(TH_Grapplehook_Start, this, LaunchFunction, GrappleDrag_PreLaunch_Timer_Length);

	// End GrappleHook Timer
	GetWorldTimerManager().SetTimer(TH_Grapplehook_End_Launch, this, &ASteikemannCharacter::GH_Stop, GrappleDrag_PreLaunch_Timer_Length + GrappleHook_PostLaunchTimer);
}

void ASteikemannCharacter::GH_PreLaunch_Dynamic(IGrappleTargetInterface* IGrapple, bool OnGround)
{
	GH_PreLaunch();
	IGrapple->HookedPure();
	IGrapple->HookedPure(GetActorLocation(), OnGround, true);

	//FTimerHandle h;
	GetWorldTimerManager().SetTimer(TH_Grapplehook_Pre_Launch,
		[this, IGrapple, OnGround]()
		{
			m_EGrappleState = EGrappleState::Post_Launch;
			GetMoveComponent()->m_GravityMode = EGravityMode::Default;
			IGrapple->HookedPure(GetActorLocation(), OnGround);
		},
		GrappleDrag_PreLaunch_Timer_Length, false);

	// End GrappleHook Timer
	GetWorldTimerManager().SetTimer(TH_Grapplehook_End_Launch, this, &ASteikemannCharacter::GH_Stop, GrappleDrag_PreLaunch_Timer_Length + GrappleHook_PostLaunchTimer);
}

void ASteikemannCharacter::GH_PreLaunch()
{
	m_EState = EState::STATE_Grappling;
	m_EGrappleState = EGrappleState::Pre_Launch;
	GetMoveComponent()->m_GravityMode = EGravityMode::ForcedNone;
	GetMoveComponent()->DeactivateJumpMechanics();
	RotateActorYawToVector(Active_GrappledActor->GetActorLocation() - GetActorLocation());
}

void ASteikemannCharacter::GH_Launch_Static()
{
	/* Grapple Launch */
	FVector LaunchDirection = Active_GrappledActor->GetActorLocation() - GetActorLocation();

	float length = LaunchDirection.Size();

	float LaunchStrength{};
	/* Set launch strength based on length and threshhold */
	length < GrappleHook_Threshhold ?
		LaunchStrength = GrappleHook_LaunchSpeed :
		LaunchStrength = GrappleHook_LaunchSpeed + (GrappleHook_LaunchSpeed * ((length - GrappleHook_Threshhold) / (GrappleHookRange - GrappleHook_Threshhold)) / GrappleHook_DividingFactor);

	FVector Direction = LaunchDirection.GetSafeNormal() + FVector::UpVector;
	Direction.Normalize();

	GetMoveComponent()->AddImpulse(Direction * LaunchStrength, true);
}


void ASteikemannCharacter::GH_Launch_Static_StuckEnemy()
{
	GetMoveComponent()->DeactivateJumpMechanics();
	FVector GrappledLocation = GrappledActor->GetActorLocation();
	FVector Direction = GrappledLocation - GetActorLocation();
	FVector Direction2D = FVector(Direction.X, Direction.Y, 0);

	FVector Velocity = Direction2D / GrappleHook_Time_ToStuckEnemy;

	float z{};
	z = ((GrappledLocation.Z + GrappleHook_AboveStuckEnemy) - GetActorLocation().Z);

	Velocity.Z = (z / GrappleHook_Time_ToStuckEnemy) + (0.5 * m_BaseGravity * GrappleHook_Time_ToStuckEnemy * -1.f);
	GetMoveComponent()->AddImpulse(Velocity, true);

	// When grapple hooking to stuck enemy, set a wall jump activation timer 
	m_WallState = EOnWallState::WALL_Leave;
	FTimerHandle h;
	GetWorldTimerManager().SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, GrappleHook_Time_ToStuckEnemy + OnWallActivation_PostStuckEnemyGrappled, false);
}

void ASteikemannCharacter::GH_Stop()
{
	Active_GrappledActor = nullptr;

	m_EState = EState::STATE_None;
	m_EGrappleState = EGrappleState::None;
	m_EGrappleType = EGrappleType::None;
	SetDefaultState();
}


void ASteikemannCharacter::DetectPhysMaterial()
{
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() - FVector(0, 0, 100);
	FHitResult Hit;

	FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);
	Params.bReturnPhysicalMaterial = true;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (bHit)
	{
		GetMoveComponent()->Traced_GroundFriction = Hit.PhysMaterial->Friction;
		TEnumAsByte<EPhysicalSurface> surface = Hit.PhysMaterial->SurfaceType;

		FString string;
		switch (surface)
		{
		case SurfaceType_Default:
			string = "Default";
			bSlipping = false;
			break;

		case SurfaceType1:
			string = "Slippery";
			bSlipping = true;
			break;
		default:
			break;
		}
	}
}

void ASteikemannCharacter::PlayCameraShake(TSubclassOf<UCameraShakeBase> shake, float falloff)
{
	/* Skal regne ut Velocity.Z også basere falloff på hvor stor den er */
	UGameplayStatics::PlayWorldCameraShake(GetWorld(), shake, Camera->GetComponentLocation() + FVector(0, 0, 1), 0.f, 10.f, falloff);
}

/* ----------------- Read this function from the bottom->up after POINT ----------------- */
void ASteikemannCharacter::GuideCamera(float DeltaTime)
{
	/* Lambda function for SLerp Playercontroller Quaternion to target Quat */	// Kunne bare vært en funksjon som tar inn en enum verdi
	auto SLerpToQuat = [&](FQuat& Target, float alpha, APlayerController* Con) {
		FQuat Rot{ Con->GetControlRotation() };
		FQuat New{ FQuat::Slerp(Rot, Target, alpha) };
		FRotator Rot1 = New.Rotator();
		Rot1.Roll = 0.f;
		Con->SetControlRotation(Rot1);
	};

	/* Lerp CameraBoom TargetArmLength back to it's original length, after the changes caused by CAMERA_Absolute */
	if (bCamLerpBackToPosition) {
		float Current = CameraBoom->TargetArmLength;
		float L = FMath::FInterpTo(Current, Base_CameraBoomLength, DeltaTime, 2.f);
		CameraBoom->TargetArmLength = L;

		if (CameraBoom->TargetArmLength == Base_CameraBoomLength) {
			bCamLerpBackToPosition = false;
		}
	}

	/* IF NOT IN CAMERA VOLUME */
	if (mFocusPoints.Num() == 0) { 
		//CameraGuideAlpha <= 0.f ? CameraGuideAlpha = 0.f : CameraGuideAlpha -= DeltaTime * 3.f;
		CameraGuideAlpha = 0.f;

		/* Reset camera TargetArmLength */
		if (CurrentCameraGuide == EPointType::CAMERA_Absolute) {
			bCamLerpBackToPosition = true;
			CurrentCameraGuide = EPointType::NONE;
		}
		return; 
	}

	/* This sets the camera directly to the FocusPoint */
	FocusPoint FP = mFocusPoints[0];	// Hardkoder for første array punkt nå, Så vil ikke håndterer flere volumes og prioriteringene mellom dem
	/* Get location used for */
	switch (FP.ComponentType)
	{
	case EFocusType::FOCUS_Point:
		FP.Location = FP.ComponentLocation;
		break;
	case EFocusType::FOCUS_Spline:
		// Bruk Internal_key til å finne lokasjonen
		// Finn ny Spline key
		// Lerp internal_key til ny splinekey
		FP.Location = FP.FocusSpline->GetLocationAtSplineInputKey(Internal_SplineInputkey, ESplineCoordinateSpace::World);
		FP.SplineInputKey = FP.FocusSpline->FindInputKeyClosestToWorldLocation(CameraBoom->GetComponentLocation());
		Internal_SplineInputkey = FMath::FInterpTo(Internal_SplineInputkey, FP.SplineInputKey, DeltaTime, SplineLerpSpeed);
		break;
	default:
		break;
	}

	/* ----------------- POINT ------------------- */

	/* Pitch Adjustment  
	* Based on distance to object */
	auto PitchAdjust = [&](float& PitchCurrent, float ZAtMin, float ZAtMax) {
		float DistMin = CameraGuide_Pitch_DistanceMIN;
		float DistMax = CameraGuide_Pitch_DistanceMAX;
		FVector VtoP = FP.Location - CameraBoom->GetComponentLocation();
		float Distance = FMath::Clamp(VtoP.Length(), DistMin, DistMax);
		
		float Z;
		float N = Distance / DistMax;	// Prosenten
		Z = (N * ZAtMax) + (1-N)*ZAtMin;

		/* Juster for høyden */
		float Zdiff = FP.Location.Z - GetActorLocation().Z;
		if (Zdiff < 0){ Zdiff *= -1.f; }
		Z += Zdiff * CameraGuide_ZdiffMultiplier;
		
		return Z;
	};



	auto LA_Absolute = [&](FocusPoint& P, float& alpha, APlayerController* Con) {
		alpha >= 1.f ? alpha = 1.f : alpha += DeltaTime * P.LerpSpeed;
		FQuat VToP{ ((P.Location - FVector(0, 0, PitchAdjust(CameraGuide_Pitch, CameraGuide_Pitch_MIN, CameraGuide_Pitch_MAX)/*Pitch Adjustment*/)) - CameraBoom->GetComponentLocation()).Rotation() };	// Juster pitch adjustment basert på z til VToP uten adjustment

		SLerpToQuat(VToP, alpha, Con);
	};

	auto LA_Lean = [&](FocusPoint& P, float& alpha, APlayerController* Con) {
		FVector VToP_vec = (P.Location - FVector(0, 0, PitchAdjust(CameraGuide_Pitch, CameraGuide_Pitch_MIN, CameraGuide_Pitch_MAX)/*Pitch Adjustment*/)) - CameraBoom->GetComponentLocation();
		FQuat VToP_quat{ (VToP_vec).Rotation() };	
		
		float DotProd = FVector::DotProduct(VToP_vec.GetSafeNormal(), Con->GetControlRotation().Vector());

		if (DotProd >= ((2 * P.LeanRelax) - 1)){
			alpha = FMath::FInterpTo(alpha, (P.LeanSpeed / 100.f) * (P.LeanMultiplier - ((DotProd / 2.f + 0.5f) * (P.LeanMultiplier - 1))), DeltaTime, 2.f);
		}
		else {
			alpha = FMath::FInterpTo(alpha, (P.LeanSpeed / 50.f) * (P.LeanMultiplier - ((DotProd / 2.f + 0.5f) * (P.LeanMultiplier - 1))), DeltaTime, 2.f);
		}
		if (DotProd >= ((2 * P.LeanAmount) - 1)) {
			return;
		}

		SLerpToQuat(VToP_quat, alpha, Con);
	};

	auto CAM_LerpToPosition = [&](FQuat& Target, float& alpha, float& length, float lerpSpeed, APlayerController* Con) {
		SLerpToQuat(Target, alpha, Con);

		float Current = CameraBoom->TargetArmLength;
		float L = FMath::FInterpTo(Current, length, DeltaTime, lerpSpeed);
		CameraBoom->TargetArmLength = L;
	};


	auto CAM_Absolute = [&](FocusPoint& P, float& alpha, APlayerController* Con) {
		alpha >= 1.f ? alpha = 1.f : alpha += DeltaTime * P.LerpSpeed;
		FVector VtoP_vec = CameraBoom->GetComponentLocation() - P.Location;
		FQuat VtoP_quat{ (VtoP_vec).Rotation() };
		float length = VtoP_vec.Size();

		// SLerp CameraBoom til quat
		// Set Target Arm length slik at kamera sitter på target
		CAM_LerpToPosition(VtoP_quat, alpha, length, 1.f/P.LerpSpeed, Con);
	};

	auto CAM_Lean = [&](FocusPoint& P, float& alpha, APlayerController* Con) {
		//alpha >= 1.f ? alpha = 1.f : alpha += DeltaTime * P.LerpSpeed;
		FVector VtoP_vec = CameraBoom->GetComponentLocation() - P.Location;
		FQuat VtoP_quat{ (VtoP_vec).Rotation() };

		float DotProd = FVector::DotProduct(VtoP_vec.GetSafeNormal(), Con->GetControlRotation().Vector());

		if (DotProd >= ((2 * P.LeanRelax) - 1)) {
			alpha = FMath::FInterpTo(alpha, (P.LeanSpeed / 100.f) * (P.LeanMultiplier - ((DotProd / 2.f + 0.5f) * (P.LeanMultiplier - 1))), DeltaTime, 2.f);
		}
		else {
			alpha = FMath::FInterpTo(alpha, (P.LeanSpeed / 50.f) * (P.LeanMultiplier - ((DotProd / 2.f + 0.5f) * (P.LeanMultiplier - 1))), DeltaTime, 2.f);
		}
		if (DotProd >= ((2 * P.LeanAmount) - 1)) {
			return;
		}

		SLerpToQuat(VtoP_quat, alpha, Con);
	};


	/* ----- SWITCH TO DETERMINE TYPE OF CAMERA VOLUME ----- */
	switch (FP.Type)
	{
	case EPointType::LOOKAT_Absolute:
		CurrentCameraGuide = EPointType::LOOKAT_Absolute;
		if (CurrentCameraGuide != PreviousCameraGuide) { CameraGuideAlpha = 0.f; }
		LA_Absolute(FP, CameraGuideAlpha, GetPlayerController());
		PreviousCameraGuide = EPointType::LOOKAT_Absolute;
		break;

	case EPointType::LOOKAT_Lean:
		CurrentCameraGuide = EPointType::LOOKAT_Lean;
		if (CurrentCameraGuide != PreviousCameraGuide) { CameraGuideAlpha = 0.f; }
		LA_Lean(FP, CameraGuideAlpha, GetPlayerController());
		PreviousCameraGuide = EPointType::LOOKAT_Lean;
		break;

	case EPointType::CAMERA_Absolute:
		CurrentCameraGuide = EPointType::CAMERA_Absolute;
		if (CurrentCameraGuide != PreviousCameraGuide) { CameraGuideAlpha = 0.f; }
		CAM_Absolute(FP, CameraGuideAlpha, GetPlayerController());
		PreviousCameraGuide = EPointType::CAMERA_Absolute;
		break;

	case EPointType::CAMERA_Lean:
		CurrentCameraGuide = EPointType::CAMERA_Lean;
		if (CurrentCameraGuide != PreviousCameraGuide) { CameraGuideAlpha = 0.f; }
		CAM_Lean(FP, CameraGuideAlpha, GetPlayerController());
		PreviousCameraGuide = EPointType::CAMERA_Lean;
		break;

	default:
		break;
	}

	//PRINTPAR("Alpha : %f", CameraGuideAlpha);
	//PRINTPAR("Fokuspoints: %i", mFocusPoints.Num());
}

void ASteikemannCharacter::GuideCameraTowardsVector(FVector vector, float alpha)
{
	FQuat target{ vector.Rotation() };

	FQuat Rot{ GetPlayerController()->GetControlRotation()};
	FQuat New{ FQuat::Slerp(Rot, target, alpha) };
	FRotator Rot1 = New.Rotator();
	Rot1.Roll = 0.f;
	GetPlayerController()->SetControlRotation(Rot1);
}

void ASteikemannCharacter::GuideCameraPitch(float z, float alpha)
{
	FVector old = GetControlRotation().Vector();
	old.Z = z;
	float angle = acosf(z);
	old = (cosf((PI/2) * z) * old.GetSafeNormal2D()) + (sinf((PI / 2) * z) * FVector::UpVector);
	FQuat target{ old.Rotation() };

	FQuat Rot{ GetPlayerController()->GetControlRotation() };
	FQuat New{ FQuat::Slerp(Rot, target, alpha) };
	FRotator Rot1 = New.Rotator();
	Rot1.Roll = 0.f;
	GetPlayerController()->SetControlRotation(Rot1);
}

float ASteikemannCharacter::GuideCameraPitchAdjustmentLookAt(FVector LookatLocation, float MinDistance, float MaxDistance, float PitchAtMin, float PitchAtMax, float ZdiffMultiplier)
{
	// Pitch adjustment, based on distance to object
	FVector BoomToLookat = LookatLocation - CameraBoom->GetComponentLocation();
	float Distance = FMath::Clamp(BoomToLookat.Size(), MinDistance, MaxDistance);

	// Find percentage, get Z with linear parametrisation
	float N = Distance / MaxDistance;
	float Z = (N * PitchAtMax) + (1 - N) * PitchAtMin;

	// Adjust for height
	float Zdiff = LookatLocation.Z - CameraBoom->GetComponentLocation().Z;
	if (Zdiff < 0.f) Zdiff *= -1.f;
	Z += Zdiff * ZdiffMultiplier;

	return Z;
}

void ASteikemannCharacter::GrappleDynamicGuideCamera(float deltatime)
{
	if (!GrappledActor.Get()) return;

	FVector input = InputVectorRaw;

	FVector grappled = GrappledActor->GetActorLocation();
	grappled.Z = GetActorLocation().Z;
	//grappled.Z = InitialGrappleDynamicZ;
	//FVector toGrapple = (grappled - GetActorLocation()) - FVector(0, 0, GuideCameraPitchAdjustmentLookAt(grappled, GrappleDynamic_Pitch_DistanceMIN, GrappleHookRange, GrappleDynamic_Pitch_MIN, GrappleDynamic_Pitch_MAX, GrappleDynamic_ZdiffMultiplier));
	FVector toGrapple = (grappled - GetActorLocation());
	toGrapple.Normalize();
	FVector toGrapple2D = toGrapple.GetSafeNormal2D();
	
	// Yaw
	float y = input.Y;
	if (y < 0.f) y *= -1.f;
	float alphaY = 1.f - y;
		// Creates left and right vector to SLerp towards 
	FVector cameraYawRight = toGrapple.RotateAngleAxis( GrappleDynamic_MaxYaw, FVector::UpVector);
	FVector cameraYawLeft  = toGrapple.RotateAngleAxis(-GrappleDynamic_MaxYaw, FVector::UpVector);
	FVector cameraYaw = cameraYawRight;
	if (input.Y < 0.f) cameraYaw = cameraYawLeft;
	else if (input.Y <= 0.05f) cameraYaw = toGrapple2D;
	GuideCameraTowardsVector(cameraYaw, y * GrappleDynamic_YawAlpha);

	// Pitch
	float x = input.X;
	if (x < 0.f) x *= -1.f;
	float alphaX = 1.f - x;
	GuideCameraPitch(FMath::Clamp(input.X, -GrappleDynamic_MaxPitch, GrappleDynamic_MaxPitch), x * GrappleDynamic_PitchAlpha);

	// Default Guide towards grappled target
	GuideCameraTowardsVector(toGrapple, alphaY * GrappleDynamic_DefaultAlpha);
	GuideCameraPitch(GrappleDynamic_DefaultPitch, alphaX * GrappleDynamic_DefaultAlpha);
}



void ASteikemannCharacter::ResetState()
{
	if (IsOnGround()) {
		m_EState = EState::STATE_OnGround;
		return;
	}
	if (GetMoveComponent()->IsFalling()) {
		m_EState = EState::STATE_InAir;
		m_EAirState = EAirState::AIR_Freefall;
		return;
	}
}

void ASteikemannCharacter::SetDefaultState()
{
	switch (m_EState)
	{
	case EState::STATE_None:
		break;
	case EState::STATE_OnGround:
		break;
	case EState::STATE_InAir:
		if (m_EAirState == EAirState::AIR_Pogo) return;
		break;
	case EState::STATE_OnWall: return;
	case EState::STATE_Attacking: 
		if (m_EAttackState == EAttackState::GroundPound)
			if (GetCharacterMovement()->IsWalking())
				m_EState = EState::STATE_OnGround;
		return;
	case EState::STATE_Grappling: return;
	default:
		break;
	}

	ResetState();
}

void ASteikemannCharacter::AllowActionCancelationWithInput()
{
	//m_State = EState::STATE_None;
	//SetDefaultState();
	m_EMovementInputState = EMovementInput::Open;

}

bool ASteikemannCharacter::BreakMovementInput(float value)
{
	if (m_EMovementInputState == EMovementInput::Locked) return true;
	switch (m_EState)
	{
	case EState::STATE_OnGround:
		break;
	case EState::STATE_InAir:
		break;
	case EState::STATE_OnWall: return true;
	case EState::STATE_Attacking:
	{
		if (m_EAttackState == EAttackState::Smack && m_EMovementInputState == EMovementInput::Open && value > 0.1f)
		{
			ResetState();
			StopAnimMontage();
			Deactivate_AttackCollider();
			return false;
		}
		return true;
		break;
	}
	case EState::STATE_Grappling: return true;
	default:
		break;
	}
	return false;
}

void ASteikemannCharacter::MoveForward(float value)
{
	InputVectorRaw.X = value;

	if (BreakMovementInput(value)) return;

	float movement = value;
	if (bSlipping)
		movement *= 0.1;

	if ((Controller != nullptr) && (value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, movement);
	}
}



void ASteikemannCharacter::MoveRight(float value)
{
	InputVectorRaw.Y = value;

	if (BreakMovementInput(value)) return;

	float movement = value;
	if (bSlipping)
		movement *= 0.1;

	if ((Controller != nullptr) && (value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, movement);
	}

}


void ASteikemannCharacter::TurnAtRate(float rate)
{
	AddControllerYawInput(rate * TurnRate * GetWorld()->GetDeltaSeconds());
}

void ASteikemannCharacter::LookUpAtRate(float rate)
{
	AddControllerPitchInput(rate * TurnRate * GetWorld()->GetDeltaSeconds());
}

void ASteikemannCharacter::Landed(const FHitResult& Hit)
{
	PRINTLONG("Landed");
	UE_LOG(LogTemp, Display, TEXT("Landed"));
	switch (m_EState)
	{
	case EState::STATE_None:
		break;
	case EState::STATE_OnGround:
		break;
	case EState::STATE_InAir:
		switch (m_EAirState)
		{
		case EAirState::AIR_None:		break;
		case EAirState::AIR_Freefall:	break;
		case EAirState::AIR_Jump:		break;
		case EAirState::AIR_Pogo:		return;
		default:						break;
		}
		break;
	case EState::STATE_OnWall:
		break;
	case EState::STATE_Attacking:
		if (bPB_Groundpound_PredeterminedPogoHit)
			return;
		if (m_EAttackState == EAttackState::GroundPound)
		{
			bool groundpound{};
			if (IsGroundPounding()) {
				GroundPoundLand(Hit);
				//m_EAttackState = EAttackState::None;
				//ResetState();
				groundpound = true;
			}
		}
		//if (m_EPogoType == EPogoType::POGO_Groundpound)
			//return;
		break;
	case EState::STATE_Grappling:
		break;
	default:
		break;
	}



	//if (bPB_Groundpound_PredeterminedPogoHit)
		//return;

	OnLanded(Hit);

	LandedDelegate.Broadcast(Hit);


	bPB_Groundpound_PredeterminedPogoHit = false;
	//m_EPogoTickCheck = EPogoTickCheck::PB_Tick_None;
}

void ASteikemannCharacter::Jump()
{
	/* Don't Jump if player is Grappling */
	switch (m_EState)
	{
	case EState::STATE_None:
		break;
	case EState::STATE_OnGround:
		break;
	case EState::STATE_InAir:
		break;
	case EState::STATE_OnWall:
		break;
	case EState::STATE_Attacking: return;
	case EState::STATE_Grappling: return;
	default:
		break;
	}

	if (!bJumpClick)
	{
		bJumpClick = true;
		bJumping = true;

		FTimerHandle h;
		switch (m_EState)	// TODO: Go to Sub-States eg: OnGround->Idle|Running|Slide
		{
		case EState::STATE_OnGround:	// Regular Jump
		{
			JumpCurrentCount++;
			GetMoveComponent()->Jump(JumpStrength);
			Anim_Activate_Jump();

			GetWorldTimerManager().SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, OnWallActivation_PostJumpingOnGround, false);
			m_WallState = EOnWallState::WALL_Leave;
			break;
		}
		case EState::STATE_InAir:	// Double Jump
		{
			if (m_EPogoType != EPogoType::POGO_Leave)
				if (PB_Active_TargetDetection())
				{
					UE_LOG(LogTemp, Warning, TEXT("----------------"));

					m_EPogoType = EPogoType::POGO_Active;
					//PB_Active_IMPL();
					//Anim_Activate_Jump();
					break;
				}

			if (GetMoveComponent()->IsFalling() && JumpCurrentCount == 0)
			{
				/* and the post edge timer is valid */
				if (bCanPostEdgeRegularJump)
				{
					JumpCurrentCount++;
				}
				/* after post edge timer is valid */
				else 
				{
					JumpCurrentCount = 2;
				}
				GetMoveComponent()->Jump(JumpStrength);
				Anim_Activate_Jump();
				break;
			}
			if (CanDoubleJump())
			{
				JumpCurrentCount++;
				GetMoveComponent()->DoubleJump(InputVector.GetSafeNormal(), JumpStrength * DoubleJump_MultiplicationFactor);
				GetMoveComponent()->StartJumpHeightHold();
				Anim_Activate_Jump();//Anim DoubleJump
			}
			break;
		}
		case EState::STATE_OnWall:	// Wall Jump
		{
			// Ledgejump
			if (m_WallState == EOnWallState::WALL_Ledgegrab)
			{
				m_EState = EState::STATE_InAir;
				m_WallState = EOnWallState::WALL_Leave;
				GetMoveComponent()->LedgeJump(InputVector, JumpStrength);
				GetMoveComponent()->m_GravityMode = EGravityMode::LerpToDefault;
				GetWorldTimerManager().SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, 0.5f, false);
				break;
			}

			// Walljump
			m_EState = EState::STATE_InAir;
			m_WallState = EOnWallState::WALL_Leave;
			
			
			if (InputVector.SizeSquared() > 0.5f)
				RotateActorYawToVector(InputVector);
			else
				RotateActorYawToVector(m_WallJumpData.Normal);

			GetWorldTimerManager().SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, OnWall_Reset_OnWallJump_Timer, false);
			GetMoveComponent()->WallJump(InputVector, JumpStrength);
			Anim_Activate_Jump();//Anim_Activate_WallJump
			break;
		}
		case EState::STATE_Attacking:
			break;
		case EState::STATE_Grappling:
			break;
		default:
			break;
		}
	}
}


void ASteikemannCharacter::StopJumping()
{
	//PRINTLONG("STOP JUMP");
	//PRINTPARLONG("JumpHeight: %f", GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	//GetMoveComponent()->StopJump();
	//bJumpClick = false;

	/* Old Jump */
	//bPressedJump = false;
	//bActivateJump = false;
	//bJumping = false;
	//bAddJumpVelocity = false;
	//bCanEdgeJump = false;
	//bCanPostEdgeJump = false;
	//
	//if (GetMoveComponent().IsValid())
	//{
	//	GetMoveComponent()->bWallJump = false;
	//	GetMoveComponent()->bLedgeJump = false;
	//	GetMoveComponent()->EndCrouchJump();
	//	GetMoveComponent()->EndCrouchSlideJump();	// Kan KANSKJE sette inn at CrouchSlide Jump må kjøre x antall frames før det kan stoppes, og at det ikke tvangsstoppes når man slepper jump knappen. En TIMER basically
	//}

	//ResetJumpState();
}

void ASteikemannCharacter::JumpRelease()
{
	bJumpClick = false;
	bJumping = false;
	GetMoveComponent()->StopJump();
}

void ASteikemannCharacter::CheckJumpInput(float DeltaTime)
{
	JumpCurrentCountPreJump = JumpCurrentCount;

	if (GetCharacterMovement())
	{
		if (bPressedJump)
		{
			/* Crouch- and Slide- Jump */
			if (IsCrouching() && GetMoveComponent()->IsWalking())
			{
				/* Crouch Jump */
				if (IsCrouchWalking())
				{
					//PRINTLONG("--CrouchJump--");
					JumpCurrentCount++;
					/*bAddJumpVelocity = */CanCrouchJump();
					GetMoveComponent()->StartCrouchJump();
					Anim_Activate_Jump();	// Anim_Activate_CrouchJump()
					return;
				}

				/* CrouchSlide Jump */
				if (IsCrouchSliding())
				{
					JumpCurrentCount++;
					{
						FVector CrouchSlideDirection = GetVelocity();
						CrouchSlideDirection.Normalize();
						/*bAddJumpVelocity = */GetMoveComponent()->CrouchSlideJump(CrouchSlideDirection, InputVector);
					}
					Anim_Activate_Jump();	// Anim_Activate_CrouchSlideJump()
					return;
				}
			}

			/* If player is LedgeGrabbing */
			//if ((GetMoveComponent()->bLedgeGrab) && GetMoveComponent()->IsFalling())
			//{
			//	JumpCurrentCount = 1;
			//	/*bAddJumpVelocity = */GetMoveComponent()->LedgeJump(LedgeLocation, JumpStrength);
			//	ResetWallJumpAndLedgeGrab();
			//	Anim_Activate_Jump();	//	Anim_Activate_LedgeGrabJump
			//	return;
			//}

			///* If player is sticking to a wall */
			//if ((GetMoveComponent()->bStickingToWall || bFoundStickableWall ) && GetMoveComponent()->IsFalling())
			//{	
			//	if (JumpCurrentCount == 2)
			//	{
			//		//JumpCurrentCount++;
			//	}
			//	Anim_Activate_Jump();	//	Anim_Activate_WallJump
			//	return;
			//}


			/* If player walks off edge with no jumpcount and the post edge timer is valid */
			if (GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0 && PostEdge_JumpTimer < PostEdge_JumpTimer_Length) 
			{
				//PRINTLONG("POST EDGE JUMP");
				bCanPostEdgeRegularJump = true;
				JumpCurrentCount++;
				Anim_Activate_Jump();
				//bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
				return;
			}
			/* If player walks off edge with no jumpcount after post edge timer is valid */
			if (GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0)
			{
				bCanEdgeJump = true;
				JumpCurrentCount += 2;
				Anim_Activate_Jump();
				//bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
				return;
			}

			// If this is the first jump and we're already falling,
			// then increment the JumpCount to compensate.
			const bool bFirstJump = JumpCurrentCount == 0;
			if (bFirstJump && GetCharacterMovement()->IsFalling())
			{
				Anim_Activate_Jump();
				JumpCurrentCount++;
			}
			if (CanDoubleJump() && GetCharacterMovement()->IsFalling())
			{
				Anim_Activate_Jump();
				GetCharacterMovement()->DoJump(bClientUpdating);
				JumpCurrentCount++;
			}

			const bool bDidJump = CanJump() && GetCharacterMovement()->DoJump(bClientUpdating);
			if (bDidJump)
			{
				Anim_Activate_Jump();
				// Transition from not (actively) jumping to jumping.
				if (!bWasJumping)
				{
					JumpCurrentCount++;
					JumpForceTimeRemaining = GetJumpMaxHoldTime();
					OnJumped();
				}
			}
			bWasJumping = bDidJump;
		}
	}
}


bool ASteikemannCharacter::CanDoubleJump() const
{
	return JumpCurrentCount == 1;
}

bool ASteikemannCharacter::IsJumping() const
{
	return bJumping;
}

bool ASteikemannCharacter::IsFalling() const
{
	if (!GetMoveComponent().IsValid()) { return false; }
	return  GetMoveComponent()->MovementMode == MOVE_Falling 
		&& m_EState == EState::STATE_InAir 
		&& m_EAirState == EAirState::AIR_Freefall;
}

bool ASteikemannCharacter::IsOnGround() const
{
	if (!GetMoveComponent().IsValid()) { return false; }
	return GetMoveComponent()->MovementMode == MOVE_Walking;
}

bool ASteikemannCharacter::PB_TargetBeneath()
{
	FHitResult Hit{};
	FCollisionQueryParams Params{ "", false, this };
	const bool b = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + FVector::DownVector * 700.f, ECC_PogoCollision, Params);
	if (!b) return false;

	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(Hit.GetActor());
	if (!ITag) return false;

	FGameplayTagContainer tags;
	ITag->GetOwnedGameplayTags(tags);
	if (tags.HasTag(Tag::PogoTarget())) {
		m_PogoTarget = Hit.GetActor();
		return true;
	}

	return false;
}

bool ASteikemannCharacter::PB_ValidTargetDistance(const FVector OtherActorLocation)
{
	float PlayerZ = (GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (PlayerZ < OtherActorLocation.Z)
		return false;

	FVector Direction2D = FVector(OtherActorLocation - GetActorLocation());
	Direction2D.Z = 0.f;
	const bool Valid2D = Direction2D.Size() <= PB_Max2DTargetDistance;

	float LengthToEnemy = GetActorLocation().Z - OtherActorLocation.Z;
	float Difference = (GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) + (PB_TargetLengthContingency);
	/* If Player is close to the enemy */
	if (LengthToEnemy - Difference < 0.f && Valid2D)
		return true;

	return false;
}

bool ASteikemannCharacter::PB_Active_TargetDetection()
{
	FCollisionShape capsule = FCollisionShape::MakeCapsule(PB_ActiveDetection_CapsuleRadius, PB_ActiveDetection_CapsuleHalfHeight);

	FVector location = GetActorLocation() - FVector(0, 0, PB_ActiveDetection_CapsuleZLocation);
	FHitResult Hit;
	FCollisionQueryParams Params("", false, this);
	bool b = GetWorld()->SweepSingleByChannel(Hit, location, location, FQuat(1, 0, 0, 0), ECC_PogoCollision, capsule, Params);
	return b;
}

bool ASteikemannCharacter::PB_Passive_IMPL(AActor* OtherActor)
{
	// Validate Distance
	if (!PB_ValidTargetDistance(OtherActor->GetActorLocation()))	
		return false;
	if (GetCharacterMovement()->Velocity.Z > 0.f)
		return false;

	// Launch Passive Pogo
	PB_EnterPogoState(PB_StateTimer_Passive);
	PB_Launch_Passive();

	// Affect Pogo Target

	return true;
}

void ASteikemannCharacter::PB_Launch_Passive()
{
	FVector Direction = GetCharacterMovement()->Velocity.GetSafeNormal2D();
	Direction = (Direction + InputVector) / 2.f;

	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse((FVector::UpVector * PB_LaunchStrength_Z_Passive) + (Direction * PB_LaunchStrength_MultiXY_Passive), true);

	Anim_Activate_Jump();	// Anim Pogo
}

void ASteikemannCharacter::PB_Active_IMPL()
{
	m_EAirState = EAirState::AIR_Pogo;
	m_EPogoType = EPogoType::POGO_Leave;
	//GetWorldTimerManager().ClearTimer(TH_Pogo);

	PB_Launch_Active();
	PB_EnterPogoState(PB_StateTimer_Active);

	GetWorldTimerManager().SetTimer(TH_PB_ExitHandle, [this](){ m_EPogoType = EPogoType::POGO_None; }, PB_StateTimer_Active, false);

	// Visual Effects
	Anim_Activate_Jump();
}

void ASteikemannCharacter::PB_Launch_Active()
{
	FVector direction = FVector((FVector::UpVector * (1.f - (InputVector.Size() * PB_InputMulti_Active))) + (InputVector * PB_InputMulti_Active)).GetSafeNormal();
	GetMoveComponent()->PB_Launch_Active(direction, PB_LaunchStrength_Active);
}

bool ASteikemannCharacter::PB_Groundpound_IMPL(AActor* OtherActor)
{
	if (!PB_ValidTargetDistance(OtherActor->GetActorLocation()))
		return false;

	if (IAttackInterface* IAttack = Cast<IAttackInterface>(OtherActor))
		IAttack->Receive_Pogo_GroundPound_Pure();

	JumpCurrentCount = 1;	// Resets double jump
	PB_EnterPogoState(PB_StateTimer_Groundpound);
	PB_Launch_Groundpound();
	ResetState();
	m_EAttackState = EAttackState::None;
	m_EPogoType = EPogoType::POGO_None;
	return true;
}

void ASteikemannCharacter::PB_Launch_Groundpound()
{
	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);

	FVector Direction = ((FVector::UpVector * (1.f - PB_InputMulti_Groundpound)) + (InputVector * PB_InputMulti_Groundpound)).GetSafeNormal();
	GetCharacterMovement()->AddImpulse(Direction * PB_LaunchStrength_Groundpound, true);	// Simple method of bouncing player atm
	Anim_Activate_Jump();	// Anim_Pogo-Groundpound 
}

bool ASteikemannCharacter::PB_Groundpound_Predeterminehit()
{
	FCollisionQueryParams Params("", false, this);
	// Ground
	FHitResult GroundHit;
	bool ground = GetWorld()->LineTraceSingleByChannel(GroundHit, GetActorLocation(), GetActorLocation() - FVector(0, 0, 1000.f), ECC_WorldStatic, Params);
	if (!ground)
	{
		ground = GetWorld()->LineTraceSingleByChannel(GroundHit, GetActorLocation(), GetActorLocation() - FVector(0, 0, 1000.f), ECC_PogoCollision, Params);
		if (!ground)
			return false;
	}
	if (IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(GroundHit.GetActor()))
	{
		FGameplayTagContainer tags;
		ITag->GetOwnedGameplayTags(tags);
		if (tags.HasTag(Tag::PogoTarget())) {
			m_PogoTarget = GroundHit.GetActor();
			return true;
		}
	}

	// Pogo targets near ground location
	FCollisionShape capsule = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	TArray<FHitResult> CapHits;
	const bool pogotarget = GetWorld()->SweepMultiByChannel(CapHits, GroundHit.ImpactPoint, GetActorForwardVector(), FQuat(1, 0, 0, 0), ECC_PogoCollision, capsule, Params);
	if (!pogotarget) return false;


	for (const auto& it : CapHits)
		if (ValidLengthToCapsule(it.ImpactPoint, GroundHit.ImpactPoint, capsule.GetCapsuleHalfHeight(), capsule.GetCapsuleRadius())) {
			m_PogoTarget = it.GetActor();
			return true;
		}

	return false;
}

void ASteikemannCharacter::PB_Exit()
{
	m_EAttackState = EAttackState::None;
	m_EPogoType = EPogoType::POGO_None;
	//m_EPogoTickCheck = EPogoTickCheck::PB_Tick_None;
	//m_PogoTarget = nullptr;
}

bool ASteikemannCharacter::ValidLengthToCapsule(FVector HitLocation, FVector capsuleLocation, float CapsuleHeight, float CapsuleRadius)
{
	// Get point along capsule line
	float zCapMin = capsuleLocation.Z;
	float zCapMax = CapsuleHeight;

	float zLine = FMath::Clamp(HitLocation.Z - zCapMin, 0.f, zCapMax);
	float alpha = zLine / zCapMax;
	FVector Point = capsuleLocation + (FVector::UpVector * (CapsuleHeight * alpha));

	float length = FVector(HitLocation - Point).Size();

	if (length < CapsuleRadius - 10.f)
		return true;

	return false;
}

void ASteikemannCharacter::PB_Pogo()
{
	if (m_EAttackState == EAttackState::GroundPound) {
		PRINTLONG("Collision: ENTER POGO GROUNDPOUND");
		m_EPogoType = EPogoType::POGO_Groundpound;
		return;
	}
	m_EPogoType = EPogoType::POGO_Passive;
}

void ASteikemannCharacter::PB_EnterPogoState(float time)
{
	m_EAirState = EAirState::AIR_Pogo;
	GetWorldTimerManager().SetTimer(TH_Pogo, [this](){ m_EAirState = EAirState::AIR_Freefall;  }, time, false);
}

void ASteikemannCharacter::Start_Crouch()
{
	bPressedCrouch = true;

	if (GetMoveComponent()->IsWalking())
	{
		/* Crouch Slide */
		if (GetVelocity().Size() > Crouch_WalkToSlideSpeed && bCanCrouchSlide && !IsCrouchSliding() && !IsCrouchWalking())	// Start Crouch Slide
		{
			Start_CrouchSliding();
			return;
		}

		/* Crouch */
		
		bIsCrouchWalking = true;
		GetCapsuleComponent()->SetCapsuleRadius(25.f);	// Temporary solution for cube to crouch

		Crouch();
	}
}

void ASteikemannCharacter::Stop_Crouch()
{
	bPressedCrouch = false;

	if (bIsCrouchWalking)
	{
		bIsCrouchWalking = false;

		UnCrouch();
	}
}

void ASteikemannCharacter::Click_Slide()
{
	switch (m_EState)
	{
	case EState::STATE_None:
		break;
	case EState::STATE_OnGround:
		break;
	case EState::STATE_InAir:
		break;
	case EState::STATE_OnWall:
		break;
	case EState::STATE_Attacking: 
		return;
	case EState::STATE_Grappling:
	{
		// Pull dynamic target free from being stuck
		if (m_EGrappleState == EGrappleState::Pre_Launch && m_EGrappleType == EGrappleType::Static_StuckEnemy_Ground)
		{
			if (!Active_GrappledActor.IsValid()) return;
			IGrappleTargetInterface* IGrapple = Cast<IGrappleTargetInterface>(Active_GrappledActor);
			if (!IGrapple) return;
			IGrapple->PullFree_Pure(GetActorLocation());

			GetWorldTimerManager().ClearTimer(TH_Grapplehook_Start);
			GetWorldTimerManager().ClearTimer(TH_Grapplehook_Pre_Launch);
			GetWorldTimerManager().ClearTimer(TH_Grapplehook_End_Launch);

			GetMoveComponent()->m_GravityMode = EGravityMode::Default;
			GH_Stop();

			m_EMovementInputState = EMovementInput::Locked;
			FTimerHandle h;
			GetWorldTimerManager().SetTimer(h, [this]() { m_EMovementInputState = EMovementInput::Open; }, GH_PostPullingTargetFreeTime, false);
			return;
		}
		return;
	}
	default:
		break;
	}

	if (bPressedSlide) { return; }
	if (IsCrouchSliding()) { return; }

	bPressedSlide = true;

	if (GetMoveComponent()->IsWalking())
	{
		/* Crouch Slide */
		if (GetVelocity().Size() >= Crouch_WalkToSlideSpeed && bCanCrouchSlide && !IsCrouchSliding() && !IsCrouchWalking())	// Start Crouch Slide
		{
			Start_CrouchSliding();
			return;
		}
	}
}

void ASteikemannCharacter::UnClick_Slide()
{
	bPressedSlide = false;
}

void ASteikemannCharacter::Start_CrouchSliding()
{
	//if (!IsCrouchSliding())
	{
		bCrouchSliding = true;

		/* Adjust capsule collider */
		// code here

		NiComp_CrouchSlide->SetNiagaraVariableVec3("User.M_Velocity", GetVelocity() * -1.f);
		NiComp_CrouchSlide->Activate();

		FVector SlideDirection{ InputVector };
		if (InputVector.IsZero())
		{
			SlideDirection = GetActorForwardVector();
		}
		GetMoveComponent()->Initiate_CrouchSlide(SlideDirection);

		GetWorldTimerManager().SetTimer(CrouchSlide_TimerHandle, this, &ASteikemannCharacter::Stop_CrouchSliding, CrouchSlide_Time);
	}
}

void ASteikemannCharacter::Stop_CrouchSliding()
{
	bCrouchSliding = false;
	bCanCrouchSlide = false;
	NiComp_CrouchSlide->Deactivate();
	GetWorldTimerManager().SetTimer(Post_CrouchSlide_TimerHandle, this, &ASteikemannCharacter::Reset_CrouchSliding, Post_CrouchSlide_Time);

	if (bPressedCrouch) { Start_Crouch(); return; }	// If player is still holding the crouch button, move over to crouch movement

	UnCrouch();
}

void ASteikemannCharacter::Reset_CrouchSliding()
{
	bCanCrouchSlide = true;
}

void ASteikemannCharacter::ReceiveCollectible(ECollectibleType type)
{
	switch (type)
	{
	case ECollectibleType::Common:
		CollectibleCommon++;
		break;
	case ECollectibleType::Health:
		GainHealth(1);
		break;
	case ECollectibleType::CorruptionCore:
		CollectibleCorruptionCore++;
		break;
	default:
		break;
	}
}

void ASteikemannCharacter::GainHealth(int amount)
{
	Health = FMath::Clamp(Health += amount, 0, MaxHealth);
}

//void ASteikemannCharacter::PTakeDamage(int damage, FVector launchdirection)
void ASteikemannCharacter::PTakeDamage(int damage, AActor* otheractor, int i/* = 0*/)
{
	if (IsDead()) { return; }

	bPlayerCanTakeDamage = false;
	Health = FMath::Clamp(Health -= damage, 0, MaxHealth);
	if (Health == 0) {
		Death();
	}

	/* Launch player */
	FVector direction = (GetActorLocation() - otheractor->GetActorLocation()).GetSafeNormal();
	direction = (direction + FVector::UpVector).GetSafeNormal();
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (direction * 1000.f), FColor::Red, true, 0, 0, 5.f);

	FTimerHandle TH;
	GetWorldTimerManager().SetTimer(TH, 
		[this, otheractor, i]()
		{ 
		for (auto& it : CloseHazards) {
			PTakeDamage(1, otheractor, i+1);
			return;
		}
		bPlayerCanTakeDamage = true; 
		}, 
		DamageInvincibilityTime, false);

	/* Damage launch */
	GetMoveComponent()->Velocity *= 0.f;
	GetMoveComponent()->AddImpulse(direction * SelfDamageLaunchStrength, true);
}

void ASteikemannCharacter::Death()
{
	PRINTLONG("POTTITT IS DEAD");

	bIsDead = true;
	DisableInput(GetPlayerController());

	/* Set respawn timer */
	FTimerHandle h;
	GetWorldTimerManager().SetTimer(h, this, &ASteikemannCharacter::Respawn, RespawnTimer);
	// Do death related stuff here
}

void ASteikemannCharacter::Respawn()
{
	PRINTLONG("RESPAWN PLAYER");

	/* Reset player */
	Health = MaxHealth;
	bIsDead = false;
	bPlayerCanTakeDamage = true;
	CloseHazards.Empty();
	EnableInput(GetPlayerController());
	GetMoveComponent()->Velocity *= 0;

	if (Checkpoint) {
		FTransform T = Checkpoint->GetSpawnTransform();
		SetActorTransform(T, false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void ASteikemannCharacter::ExitOnWall(EState state)
{
	// TODO: Reevaluate m_State EState
	m_EState = state;
	m_WallState = EOnWallState::WALL_Leave;
	FTimerHandle h;
	GetWorldTimerManager().SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, 0.5f, false);
}

bool ASteikemannCharacter::IsOnWall() const
{
	return m_EState == EState::STATE_OnWall && m_WallState == EOnWallState::WALL_Drag;
}

bool ASteikemannCharacter::IsLedgeGrabbing() const
{
	return m_EState == EState::STATE_OnWall && m_WallState == EOnWallState::WALL_Ledgegrab;
}

bool ASteikemannCharacter::Validate_Ledge(FHitResult& hit)
{
	FHitResult h;
	FCollisionQueryParams param("", false, this);
	const bool b1 = GetWorld()->LineTraceSingleByChannel(h, m_Ledgedata.ActorLocation, m_Ledgedata.ActorLocation + (FVector::DownVector * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f)), ECC_WallDetection, param);
	if (b1)
		return false;
	const bool b2 = GetWorld()->LineTraceSingleByChannel(hit, m_Ledgedata.TraceLocation, m_Ledgedata.TraceLocation - (m_Walldata.Normal * 100.f), ECC_WallDetection, param);
	if (!b2)
		return false;

	return true;
}

void ASteikemannCharacter::Initial_LedgeGrab()
{
	FHitResult hit;
	const bool b = Validate_Ledge(hit);
	if (!b)
		return;

	m_EState = EState::STATE_OnWall;
	m_WallState = EOnWallState::WALL_Ledgegrab;

	// adjust to valid location
	FVector location = m_Ledgedata.ActorLocation;

	float length = FVector(m_Ledgedata.TraceLocation - hit.ImpactPoint).Size();
	float radius = GetCapsuleComponent()->GetScaledCapsuleRadius() + 3.f;
	if (length < radius || length > radius + 2.f)
	{
		float l = radius - length;
		location += m_Walldata.Normal * l;
	}


	// Set Transforms
	SetActorLocation(location, false, nullptr, ETeleportType::TeleportPhysics);
	RotateActorYawToVector(m_Walldata.Normal * -1.f);

	GetMoveComponent()->m_GravityMode = EGravityMode::ForcedNone;
}

void ASteikemannCharacter::LedgeGrab()
{
}

bool ASteikemannCharacter::ValidateWall()
{
	FVector start = GetActorLocation() + FVector(0, 0, Wall_HeightCriteria);
	FHitResult hit;
	FCollisionQueryParams Params("", false, this);
	const bool b = GetWorld()->LineTraceSingleByChannel(hit, start, start + (m_WallJumpData.Normal * 200.f * -1.f), ECC_WallDetection, Params);
	return b;
}

void ASteikemannCharacter::OnWall_IMPL(float deltatime)
{
	switch (m_WallState)
	{
	case EOnWallState::WALL_None:
		break;
	case EOnWallState::WALL_Hang:
		RotateActorYawToVector(m_WallJumpData.Normal * -1.f);
		break;
	case EOnWallState::WALL_Drag:
		GetMoveComponent()->m_WallJumpData = m_WallJumpData;	// TODO: Change when this happens
		OnWall_Drag_IMPL(deltatime, (GetMoveComponent()->Velocity.Z/GetMoveComponent()->WJ_DragSpeed) * -1.f);	// VelocityZ scale from 0->1
		break;
	case EOnWallState::WALL_Ledgegrab:
		DrawDebugArms(0.f);
		break;
	case EOnWallState::WALL_Leave:
		break;
	default:
		break;
	}
}

void ASteikemannCharacter::OnWall_Drag_IMPL(float deltatime, float velocityZ)
{
	// Play particle effects
	if (NS_WallSlide) {
		Component_Niagara->SetAsset(NS_WallSlide);
		Component_Niagara->SetNiagaraVariableInt("User.SpawnAmount", NS_WallSlide_ParticleAmount * deltatime * velocityZ);
		Component_Niagara->SetWorldLocationAndRotation(GetMesh()->GetSocketLocation("Front"), GetMesh()->GetSocketRotation("Front"));
		Component_Niagara->Activate(true);
	}
}

void ASteikemannCharacter::ExitOnWall_GROUND()	// Player drags along the wall and hits the ground
{
	// Play animation, maybe sound -- EState being 'idle'
}

void ASteikemannCharacter::CancelOnWall()
{
	ResetState();
	m_WallState = EOnWallState::WALL_Leave;
	GetMoveComponent()->CancelOnWall();
	GetWorldTimerManager().SetTimer(TH_OnWall_Cancel, [this]() { m_WallState = EOnWallState::WALL_None; }, OnWall_CancelTimer, false);
}

void ASteikemannCharacter::OnCapsuleComponentBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	IGameplayTagAssetInterface* tag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!tag) { return; }

	FGameplayTagContainer tags;
	tag->GetOwnedGameplayTags(tags);

	// Collision with Pogo Target
	if (tags.HasTag(Tag::PogoTarget()) && OtherComp->IsA(USphereComponent::StaticClass()))
	{
		m_PogoTarget = OtherActor;
		PB_Pogo();
	}
	
	/* Collision with collectible */
	if (tags.HasTag(Tag::Collectible())) {
		ACollectible* collectible = Cast<ACollectible>(OtherActor);
		ReceiveCollectible(collectible->CollectibleType);
		collectible->Destruction();
	}

	/* Environmental Hazard collision */
	if (tags.HasTag(Tag::EnvironmentHazard())) {
		CloseHazards.Add(OtherActor);
		if (bPlayerCanTakeDamage)
			PTakeDamage(1, OtherActor);
	}

	/* Add checkpoint, overrides previous checkpoint */
	if (tags.HasTag(Tag::PlayerRespawn())) {
		Checkpoint = Cast<APlayerRespawn>(OtherActor);
		if (!Checkpoint)
			UE_LOG(LogTemp, Warning, TEXT("PLAYER: Failed cast to Checkpoint: %s"), *OtherActor->GetName());
	}

	/* Player enters/falls into a DeathZone */
	if (tags.HasTag(Tag::DeathZone())) {
		Death();
	}
}

void ASteikemannCharacter::OnCapsuleComponentEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	IGameplayTagAssetInterface* tag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!tag) { return; }

	FGameplayTagContainer tags;
	tag->GetOwnedGameplayTags(tags);

	// Collision with Pogo Target
	if (tags.HasTag(Tag::PogoTarget()))
	{
		if (OtherActor == m_PogoTarget && OtherComp->IsA(USphereComponent::StaticClass()))
		{
			if (m_EPogoType == EPogoType::POGO_None)
				m_PogoTarget = nullptr;
			//if (m_EAirState == EAirState::AIR_Pogo || m_EPogoType != EPogoType::POGO_None)
			//	if (m_EAttackState == EAttackState::GroundPound)
			//		m_EAttackState = EAttackState::None;
		}
	}

	/* Environmental Hazard END collision */
	if (tags.HasTag(Tag::EnvironmentHazard())) {
		CloseHazards.Remove(OtherActor);
	}
}

void ASteikemannCharacter::DrawDebugArms(const float& InputAngle)
{
	FVector Ledge{ GetActorLocation() + FVector::UpVector * (LedgeGrab_Height/2.f) + GetActorForwardVector() * 30.f };
	FRotator Rot{ GetActorRotation() };
	float Angle = FMath::Clamp(InputAngle, -90.f, 90.f);

	float ArmLength{ 50.f };
	/* Right Arm */
	{
		FVector RightArmLocation{ Ledge + (GetActorRightVector() * ArmLength) };
		FVector RightArmLocation2{ Ledge + (GetActorRightVector() * ArmLength * 2) + (GetActorForwardVector() * -1.f * ArmLength * 2)};
		if (Angle < 0.f)
		{
			float Alpha = Angle / -90.f;
			//RightArmLocation = FMath::Lerp(RightArmLocation, RightArmLocation2, Alpha);
		}
		DrawDebugLine(GetWorld(), GetActorLocation(), RightArmLocation, FColor::Emerald, false, 0.f, 0, 6.f);
		DrawDebugBox(GetWorld(), RightArmLocation, FVector(30, 30, 30), Rot.Quaternion(), FColor::Emerald, false, 0.f, 0, 4.f);
	}
	/* Left Arm */
	{
		FVector LeftArmLocation{ Ledge + (GetActorRightVector() * ArmLength * -1.f) };
		FVector LeftArmLocation2{ Ledge + (GetActorRightVector() * ArmLength * 2 * -1.f) + (GetActorForwardVector() * -1.f * ArmLength * 2) };
		if (Angle > 0.f)
		{
			float Alpha = Angle / 90.f;
			//LeftArmLocation = FMath::Lerp(LeftArmLocation, LeftArmLocation2, Alpha);
		}
		DrawDebugLine(GetWorld(), GetActorLocation(), LeftArmLocation, FColor::Emerald, false, 0.f, 0, 6.f);
		DrawDebugBox(GetWorld(), LeftArmLocation, FVector(30, 30, 30), Rot.Quaternion(), FColor::Emerald, false, 0.f, 0, 4.f);
	}
}


void ASteikemannCharacter::ResetActorRotationPitchAndRoll(float DeltaTime)
{
	FRotator Rot = GetActorRotation();
	AddActorLocalRotation(FRotator(Rot.Pitch * -1.f, 0.f, Rot.Roll * -1.f));
}

void ASteikemannCharacter::RotateActorYawToVector(FVector AimVector, float DeltaTime /*=0*/)
{
	AimVector.Normalize();

	FVector AimXY = AimVector;
	AimXY.Z = 0.f;
	AimXY.Normalize();

	float YawDotProduct = FVector::DotProduct(AimXY, FVector::ForwardVector);
	float Yaw = FMath::RadiansToDegrees(acosf(YawDotProduct));

	/*		Check if yaw is to the right or left		*/
	float RightDotProduct = FVector::DotProduct(AimXY, FVector::RightVector);
	if (RightDotProduct < 0.f) { Yaw *= -1.f; }

	SetActorRotation(FRotator(GetActorRotation().Pitch, Yaw, 0.f), ETeleportType::TeleportPhysics);
}

void ASteikemannCharacter::RotateActorPitchToVector(FVector AimVector, float DeltaTime/* = 0*/)
{
	FVector Aim{ AimVector };
	Aim.Normalize();

	float Pitch{ FMath::RadiansToDegrees(asinf(Aim.Z)) };

	SetActorRotation( FRotator{ Pitch, GetActorRotation().Yaw, 0.f }, ETeleportType::TeleportPhysics );
}

void ASteikemannCharacter::RotateActorYawPitchToVector(FVector AimVector, float DeltaTime/* = 0*/)
{
	FVector Velocity = AimVector;	Velocity.Normalize();
	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;


	/*		Yaw Rotation		*/
	FVector AimXY = AimVector;
	AimXY.Z = 0.f;
	AimXY.Normalize();

	float YawDotProduct = FVector::DotProduct(AimXY, Forward);
	float Yaw = FMath::RadiansToDegrees(acosf(YawDotProduct));

	/*		Check if yaw is to the right or left		*/
	float RightDotProduct = FVector::DotProduct(AimXY, Right);
	if (RightDotProduct < 0.f) { Yaw *= -1.f; }


	/*		Pitch Rotation		*/
	FVector AimPitch = FVector(1.f, 0.f, AimVector.Z);
	AimPitch.Normalize();
	FVector ForwardPitch = FVector(1.f, 0.f, Forward.Z);
	ForwardPitch.Normalize();

	float PitchDotProduct = FVector::DotProduct(AimPitch, ForwardPitch);
	float Pitch = FMath::RadiansToDegrees(acosf(PitchDotProduct));

	/*		Check if pitch is up or down		*/
	float PitchDirection = ForwardPitch.Z - AimPitch.Z;
	if (PitchDirection > 0.f) { Pitch *= -1.f; }


	/*		Adding Yaw and Pitch rotation to actor		*/
	FRotator Rot{ Pitch, Yaw, 0.f };
	SetActorRotation(Rot);
}

void ASteikemannCharacter::RollActorTowardsLocation(FVector Location, float DeltaTime/* = 0*/)
{
	/*		Roll Rotation		*/
	FVector TowardsLocation = Location - GetActorLocation();
	TowardsLocation.Normalize();

	FVector right = GetActorRightVector();
	float RollDotProduct = FVector::DotProduct(TowardsLocation, right);
	float Roll = FMath::RadiansToDegrees(acosf(RollDotProduct)) - 90.f;
	Roll *= -1.f;

	/*		Add Roll rotation to actor		*/
	AddActorLocalRotation(FRotator(0, 0, Roll));
}

void ASteikemannCharacter::OnGrappleTargetDetectionBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this)
	{
		IGrappleTargetInterface* iGrappleInterface = Cast<IGrappleTargetInterface>(OtherActor);
		if (iGrappleInterface)
		{
			InReachGrappleTargets.AddUnique(OtherActor);
		}
	}
}

void ASteikemannCharacter::OnGrappleTargetDetectionEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != this)
	{
		IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(OtherActor);
		if (GrappleInterface)
		{
			GrappleInterface->OutofReach_Pure();
			InReachGrappleTargets.Remove(OtherActor);

			if (InReachGrappleTargets.Num() == 0)
			{
				GrappledActor = nullptr;
				Active_GrappledActor = nullptr;
			}
		}
	}
}


bool ASteikemannCharacter::CanBeAttacked()
{
	return false;
}

void ASteikemannCharacter::PreBasicAttackMoveCharacter(float DeltaTime)
{
	if (bPreBasicAttackMoveCharacter && !GetMoveComponent()->IsFalling())
	{
		AddActorWorldOffset(AttackDirection * ((PreBasicAttackMovementLength / (1 / SmackAttack_Anticipation_Rate)) * DeltaTime), false, nullptr, ETeleportType::None);
	}
}

void ASteikemannCharacter::SmackAttackMoveCharacter(float DeltaTime)
{
	if (bSmackAttackMoveCharacter && !GetMoveComponent()->IsFalling())
	{
		AddActorWorldOffset(AttackDirection * ((SmackAttackMovementLength) / (1 / SmackAttack_Action_Rate) * DeltaTime), false, nullptr, ETeleportType::None);
	}
}

void ASteikemannCharacter::ScoopAttackMoveCharacter(float DeltaTime)
{
	if (bScoopAttackMoveCharacter)
	{
		AddActorWorldOffset(AttackDirection * ((ScoopAttackMovementLength) / ((1 / ScoopAttack_Action_Rate) + (1 / ScoopAttack_Anticipation_Rate)) * DeltaTime), false, nullptr, ETeleportType::None);
	}
}

void ASteikemannCharacter::Activate_ScoopAttack()
{
	/* Character movement during attack */
	bPreBasicAttackMoveCharacter = false;
	bScoopAttackMoveCharacter = true;

	//bIsScoopAttacking = true;
}

void ASteikemannCharacter::Deactivate_ScoopAttack()
{
	bScoopAttackMoveCharacter = false;
	bHasbeenScoopLaunched = false;

	
}

void ASteikemannCharacter::ComboAttack_Pure()
{
	m_EState = EState::STATE_Attacking;
	m_ESmackAttackState = ESmackAttackState::Attack;
	m_EMovementInputState = EMovementInput::Locked;
	Deactivate_AttackCollider();
	int combo{};
	(AttackComboCount++ % 2 == 0) ? combo = 1 : combo = 2;
	ComboAttack(combo);
}

void ASteikemannCharacter::Activate_SmackAttack()
{
	/* Character movement during attack */
	bPreBasicAttackMoveCharacter = false;
	bSmackAttackMoveCharacter = true;

	//bIsSmackAttacking = true;
}

void ASteikemannCharacter::Deactivate_SmackAttack()
{
	bSmackAttackMoveCharacter = false;
}

void ASteikemannCharacter::Click_Attack()
{
	if (bAttackPress) { return; }
	bAttackPress = true;


	switch (m_EState)
	{
	case EState::STATE_OnGround:
	{
		AttackSmack_Start_Pure();
		RotateToAttack();
		AttackSmack_Start();
		break;
	}
	case EState::STATE_InAir:
	{

		break;
	}
	case EState::STATE_OnWall:
	{

		break;
	}
	case EState::STATE_Attacking:
	{
		if (m_ESmackAttackState == ESmackAttackState::Hold) 
			m_ESmackAttackState = ESmackAttackState::Buffer;
		if (m_ESmackAttackState == ESmackAttackState::Ready)
			ComboAttack_Pure();
		break;
	}
	case EState::STATE_Grappling:
	{
		/* Buffer Attack if Grappling to Dynamic Target */
		if (m_EGrappleType == EGrappleType::Dynamic_Ground)
		{
			if (GetWorldTimerManager().IsTimerActive(TH_BufferAttack)) return;

			float t = GetWorldTimerManager().GetTimerRemaining(TH_Grapplehook_End_Launch) - SmackAttack_GH_TimerRemoval;
			if (t > 0.f) {
				GetWorldTimerManager().SetTimer(TH_BufferAttack, [this]()
					{
						m_EState = EState::STATE_OnGround; 
						AttackSmack_Start_Pure();
						AttackSmack_Start();
					},
					t, false);
				return;
			}
			AttackSmack_Start_Pure();
			AttackSmack_Start();
			return;
		}
		break;
	}
	default:
		break;
	}
	return;
}

void ASteikemannCharacter::UnClick_Attack()
{
	bAttackPress = false;
}

void ASteikemannCharacter::Start_ScoopAttack_Pure()
{
	RotateToAttack();

	Start_ScoopAttack();
}

void ASteikemannCharacter::Click_ScoopAttack()
{
	/* Return conditions */
	if (bClickScoopAttack) { return; }
	if (GetMoveComponent()->IsFalling()) { return; }

	bClickScoopAttack = true;
	
	Start_ScoopAttack_Pure();
}

void ASteikemannCharacter::UnClick_ScoopAttack()
{
	bClickScoopAttack = false;
}

void ASteikemannCharacter::AttackSmack_Start_Pure()
{
	m_EState = EState::STATE_Attacking;
	m_EAttackState = EAttackState::Smack;
	m_ESmackAttackState = ESmackAttackState::Attack;
	m_EMovementInputState = EMovementInput::Locked;
}

void ASteikemannCharacter::Stop_Attack()
{
	AttackComboCount = 0;
	m_EAttackState = EAttackState::None;
	m_EState = EState::STATE_None;
	m_EMovementInputState = EMovementInput::Open;
	SetDefaultState();
}

void ASteikemannCharacter::StartAttackBufferPeriod()
{
	m_ESmackAttackState = ESmackAttackState::Hold;
}

void ASteikemannCharacter::ExecuteAttackBuffer()
{
	if (m_ESmackAttackState == ESmackAttackState::Buffer)
	{
		ComboAttack_Pure();
		return;
	}

	m_ESmackAttackState = ESmackAttackState::Ready;
}

void ASteikemannCharacter::Activate_AttackCollider()
{
			AttackCollider->SetHiddenInGame(false);	// For Debugging
	AttackCollider->SetGenerateOverlapEvents(true);
	AttackCollider->SetRelativeScale3D(AttackColliderScale);
}

void ASteikemannCharacter::Deactivate_AttackCollider()
{
	AttackDirection *= 0;

	AttackCollider->SetHiddenInGame(true);	// For Debugging
	AttackCollider->SetGenerateOverlapEvents(false);
	AttackCollider->SetRelativeScale3D(FVector(0, 0, 0));
}

//bool ASteikemannCharacter::DecideAttackType()
//{
//	/* Stop moving character using the PreBasicAttackMoveCharacter */
//	bPreBasicAttackMoveCharacter = false;
//
//	/* Do SCOOP ATTACK if attack button is still held */
//	if (bAttackPress && !GetMoveComponent()->IsFalling())
//	{
//		Activate_ScoopAttack();
//		bIsSmackAttacking = false;
//		return true;
//	}
//
//	/* SMACK ATTACK if button is not held */
//	Activate_SmackAttack();
//	bIsSmackAttacking = true;
//	return false;
//}

void ASteikemannCharacter::RotateToAttack()
{
	AttackDirection = InputVector;
	if (InputVector.IsNearlyZero())
	{
		AttackDirection = GetActorForwardVector();
		AttackDirection.Z = 0; AttackDirection.Normalize();
	}
	RotateActorYawToVector(AttackDirection);
}


void ASteikemannCharacter::OnAttackColliderBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this)
	{
		IAttackInterface* IAttack = Cast<IAttackInterface>(OtherActor);
		IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
		if (!ITag || !IAttack) { return; }

		/* Get gameplay tags */
		FGameplayTagContainer TCon;	
		ITag->GetOwnedGameplayTags(TCon);
		
		/* Attacking a corruption core */
		if (TCon.HasTag(Tag::CorruptionCore()))
		{
			EAttackType AType;
			if (OverlappedComp == AttackCollider) { AType = EAttackType::SmackAttack; }
			if (OverlappedComp == GroundPoundCollider) { AType = EAttackType::GroundPound; }
			Gen_Attack(IAttack, OtherActor, AType);
		}


		/* Smack attack collider */
		if (OverlappedComp == AttackCollider)
		{
			switch (m_EAttackState)
			{
			case EAttackState::None:
				break;
			case EAttackState::Smack:
				return Do_SmackAttack_Pure(IAttack, OtherActor);
				/*break;*/
			case EAttackState::Scoop:
				Do_ScoopAttack_Pure(IAttack, OtherActor);
				break;
			case EAttackState::GroundPound:
				break;
			default:
				break;
			}
		}

		/* GroundPound collision */
		else if (OverlappedComp == GroundPoundCollider)
		{
			Do_GroundPound_Pure(IAttack, OtherActor);
		}
	}
}

void ASteikemannCharacter::Gen_Attack(IAttackInterface* OtherInterface, AActor* OtherActor, EAttackType& AType)
{
	FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
	Direction = Direction.GetSafeNormal2D();

	OtherInterface->Gen_ReceiveAttack(Direction, SmackAttackStrength, AType);
}

void ASteikemannCharacter::Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	// Burde sjekke om den kan bli angrepet i det hele tatt. 
	const bool b{ OtherInterface->GetCanBeSmackAttacked() };

	if (b)
	{
		//IGameplayTagAssetInterface* tag = Cast<IGameplayTagAssetInterface>(OtherActor);
		//if (tag) {
		//	FGameplayTagContainer con;
		//	tag->GetOwnedGameplayTags(con);
		//	if (con.HasTag(Tag::AubergineDoggo()) || con.HasTag(Tag::GrappleTarget_Dynamic())) {
		//		Stop_GrappleHook();
		//	}
		//}

		FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
		Direction = Direction.GetSafeNormal2D();

		FVector cam;

		if (InputVector.IsNearlyZero())
			Direction = (Direction + (GetActorForwardVector() * SmackDirection_InputMultiplier)).GetSafeNormal2D();
		else {
			Direction = (Direction + (InputVector * SmackDirection_InputMultiplier) + (GetControlRotation().Vector().GetSafeNormal2D() * SmackDirection_CameraMultiplier)).GetSafeNormal2D();
		}

		float angle = FMath::DegreesToRadians(SmackUpwardAngle);
		angle = angle + (angle * (InputVectorRaw.X * SmackAttack_InputAngleMultiplier));

		Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Direction * 1000.f), FColor::Red, false, 2.f, 0, 4.f);
		OtherInterface->Receive_SmackAttack_Pure(Direction, SmackAttackStrength + (SmackAttackStrength * (InputVectorRaw.X * SmackAttack_InputStrengthMultiplier)));
	}
}

void ASteikemannCharacter::Receive_SmackAttack_Pure(const FVector& Direction, const float& Strength)
{
}


void ASteikemannCharacter::Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	const bool b{ OtherInterface->GetCanBeSmackAttacked() };

	if (b)
	{
		/* Rotates player towards scooped actor */
		RotateActorYawToVector((OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal());

		FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
		Direction = Direction.GetSafeNormal2D();
		float angle = FMath::DegreesToRadians(85.f);
		Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);
		OtherInterface->Receive_ScoopAttack_Pure(Direction, ScoopStrength);

		/* Launch player in air together with enemy when doing a Scoop Attack */
		if (!bStayOnGroundDuringScoop && !bHasbeenScoopLaunched)
		{
			//GetMoveComponent()->AddImpulse(FVector::UpVector * ScoopStrength * 0.9f, true);
			GetMoveComponent()->AddImpulse(Direction * ScoopStrength * 0.9f, true);
			bHasbeenScoopLaunched = true;
		}
	}
}

void ASteikemannCharacter::Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength)
{
}

void ASteikemannCharacter::Click_GroundPound()
{
	if (!bGroundPoundPress)
	{
		bGroundPoundPress = true;

		bPB_Groundpound_PredeterminedPogoHit = PB_Groundpound_Predeterminehit();

		if (IsGroundPounding() || IsOnGround() || m_EState == EState::STATE_OnGround) return;
		m_WallState = EOnWallState::WALL_Leave;
		
		ExitOnWall(EState::STATE_Attacking);
		GetMoveComponent()->CancelOnWall();

		Start_GroundPound();
	}
}

void ASteikemannCharacter::UnClick_GroundPound()
{
	bGroundPoundPress = false;
}


void ASteikemannCharacter::Launch_GroundPound()
{
	GetMoveComponent()->GP_Launch(GP_VisualLaunchStrength);
}

void ASteikemannCharacter::Start_GroundPound()
{
	m_EState = EState::STATE_Attacking;
	m_EAttackState = EAttackState::GroundPound;

	GetMoveComponent()->GP_PreLaunch();

	GetWorldTimerManager().SetTimer(THandle_GPHangTime, this, &ASteikemannCharacter::Launch_GroundPound, GP_PrePoundAirtime);
}

void ASteikemannCharacter::Deactivate_GroundPound()
{
	/* Re-attaches GroundPoundCollider to the rootcomponent */
	USceneComponent* RootComp = GetRootComponent();
	if (GroundPoundCollider->GetAttachParent() != RootComp)
	{
		GroundPoundCollider->AttachToComponent(RootComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	GroundPoundCollider->SetGenerateOverlapEvents(false);
	GroundPoundCollider->SetSphereRadius(0.1f);
}


void ASteikemannCharacter::GroundPoundLand(const FHitResult& Hit)
{
	USceneComponent* RootComp = GetRootComponent();
	if (GroundPoundCollider->GetAttachParent() == RootComp)
	{
		GroundPoundCollider->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	GroundPoundCollider->SetWorldLocation(Hit.ImpactPoint, false, nullptr, ETeleportType::TeleportPhysics);
	GroundPoundCollider->SetGenerateOverlapEvents(true);

	GroundPoundCollider->SetSphereRadius(MaxGroundPoundRadius, true);

	GetWorldTimerManager().SetTimer(THandle_GPReset, this, &ASteikemannCharacter::Deactivate_GroundPound, GroundPoundExpandTime);
}

void ASteikemannCharacter::Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{

	const float diff = GetActorLocation().Z - OtherActor->GetActorLocation().Z;
	const float range = 40.f;
	const bool b = diff < range || diff > -range;

	if (b)
	{
		FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
		Direction = Direction.GetSafeNormal2D();
		float angle = FMath::DegreesToRadians(45.f);
		Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);

		float LengthToOtherActor = FVector(GetActorLocation() - OtherActor->GetActorLocation()).Size();
		float Multiplier = /*1.f - */(LengthToOtherActor / MaxGroundPoundRadius);

		OtherInterface->Receive_GroundPound_Pure(Direction, GP_LaunchStrength + ((GP_LaunchStrength/2) * Multiplier));
	}
}

void ASteikemannCharacter::Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength)
{
}


