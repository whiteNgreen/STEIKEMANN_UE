// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharacter.h"
#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "../DebugMacros.h"
#include "DrawDebugHelpers.h"

// Kismet GameplayStatics
#include "Kismet/GameplayStatics.h"

// Material
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"

// Components
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TimelineComponent.h"
#include "../Components/BouncyShroomActorComponent.h"
#include "../WallDetection/WallDetectionComponent.h"
#include "NiagaraComponent.h"

// Camera
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SplineComponent.h"

// Audio
#include "Components/AudioComponent.h"

// World
#include "GameFrameWork/WorldSettings.h"
#include "../WorldStatics/SteikeWorldStatics.h"

// Steikemann
#include "BaseClasses/StaticVariables.h"
#include "../GameplayTags.h"
#include "../StaticActors/Collectible.h"
#include "../StaticActors/PlayerRespawn.h"
#include "../Dialogue/DialoguePrompt.h"
//#include "../Spawner/EnemySpawner.h"
#include "../StaticActors/Collectible.h"
//#include "../Enemies/SmallEnemy.h"

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

	GrappleTargetingDetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("Grapple Targeting Detection Sphere"));
	GrappleTargetingDetectionSphere->SetupAttachment(GetCapsuleComponent());
	GrappleTargetingDetectionSphere->SetGenerateOverlapEvents(false);
	GrappleTargetingDetectionSphere->SetSphereRadius(0.1f);

	BounceComp = CreateDefaultSubobject<UBouncyShroomActorComponent>("Bounce Component");

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

	TLComp_Attack_SMACK = CreateDefaultSubobject<UTimelineComponent>("TLComp_AttackTurn");
	TLComp_AirFriction = CreateDefaultSubobject<UTimelineComponent>("TLComp_AirFriction");
	TLComp_Dash = CreateDefaultSubobject<UTimelineComponent>("TlComp_Dash");
}

// Called when the game starts or when spawned
void ASteikemannCharacter::BeginPlay()
{
	Super::BeginPlay();

	PRINTPARLONG(3.f, "Gravity Z: %f == %f", GetCharacterMovement()->GetGravityZ(), GetCharacterMovement()->GetGravityZ() / 4.f);

	// Root motion tullball
	SetAnimRootMotionTranslationScale(1.f/4.5f);

	// Base stuff
	MovementComponent = Cast<USteikemannCharMovementComponent>(GetCharacterMovement());
	PlayerController = Cast<APlayerController>(GetController());
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnCapsuleComponentBeginOverlap);
	GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &ASteikemannCharacter::OnCapsuleComponentEndOverlap);
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ASteikemannCharacter::OnCapsuleComponentHit);
	MaxHealth = Health;
	StartTransform = GetActorTransform();
	ResetState();
	Dash_WalkSpeed_Base = GetMoveComponent()->MaxWalkSpeed;

	// Camera
	Base_CameraBoomLength = CameraBoom->TargetArmLength;
	CameraBoom_Location = CameraBoom->GetComponentLocation() - RootComponent->GetComponentLocation();

	/* Creating Niagara Compnents */
	NiComp_CrouchSlide = CreateNiagaraComponent("Niagara_CrouchSlide", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
	if (NiComp_CrouchSlide) {
		NiComp_CrouchSlide->bAutoActivate = false;
		if (NS_CrouchSlide) { NiComp_CrouchSlide->SetAsset(NS_CrouchSlide); }
	}
	NiagaraComp_Attack = CreateNiagaraComponent("Niagara_Attack");
	
	// Delegates
	PostLockedMovementDelegate.BindUObject(this, &ASteikemannCharacter::CancelAnimationMontageIfMoving);
	DeathDelegate.BindLambda([this]() {
		FTimerHandle h;
		TimerManager.SetTimer(h, this, &ASteikemannCharacter::Respawn, RespawnTimer);
		Anim_Death();	// Do IsFalling check to determine which animation to play
		});
	Delegate_MaterialUpdate.AddUObject(this, &ASteikemannCharacter::Material_UpdateParameterCollection_Player);

	// TimelineComponents
	FOnTimelineFloatStatic floatStatic;
	floatStatic.BindUObject(this, &ASteikemannCharacter::TlCurve_AttackTurn);		// Attack Turn and Movement
	TLComp_Attack_SMACK->AddInterpFloat(Curve_AttackTurnStrength, floatStatic);
	floatStatic.BindUObject(this, &ASteikemannCharacter::TlCurve_AttackMovement);
	TLComp_Attack_SMACK->AddInterpFloat(Curve_AttackMovementStrength, floatStatic);
		
	floatStatic.BindUObject(GetMoveComponent().Get(), &USteikemannCharMovementComponent::AirFrictionMultiplier);	// Air Friction
	TLComp_AirFriction->AddInterpFloat(Curve_AirFrictionMultiplier, floatStatic);

	floatStatic.BindUObject(this, &ASteikemannCharacter::TL_Dash);
	TLComp_Dash->AddInterpFloat(Curve_DashStrength, floatStatic);
	FOnTimelineEventStatic EventStatic;
	EventStatic.BindUObject(this, &ASteikemannCharacter::TL_Dash_End);
	TLComp_Dash->SetTimelineFinishedFunc(EventStatic);

	/* Attack Collider */
	AttackCollider->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnAttackColliderBeginOverlap);
	AttackColliderScale = AttackCollider->GetRelativeScale3D();
	AttackCollider->SetRelativeScale3D(FVector(0));
	GroundPoundCollider->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnAttackColliderBeginOverlap);
	
	/* Grapple Targeting Detection Sphere */
	GrappleTargetingDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnGrappleTargetDetectionBeginOverlap);
	GrappleTargetingDetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ASteikemannCharacter::OnGrappleTargetDetectionEndOverlap);

	GrappleTargetingDetectionSphere->SetGenerateOverlapEvents(true);
	GrappleTargetingDetectionSphere->SetSphereRadius(GrappleHookRange);

	WallDetector->SetCapsuleSize(WDC_Capsule_Radius, WDC_Capsule_Halfheight);
	WallDetector->SetDebugStatus(bWDC_Debug);
	WallDetector->SetHeight(Wall_HeightCriteria, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	WallDetector->SetMinLengthToWall(WDC_Length);

	/* Adding GameplayTags to the GameplayTagsContainer */
	GameplayTags.AddTag(Tag::Player());
	mFocusPoints.Empty();
}

void ASteikemannCharacter::Material_UpdateParameterCollection_Player(float DeltaTime)
{
	UKismetMaterialLibrary::SetScalarParameterValue(GetWorld(), MPC_Player, "CapsuleRadius",	GetCapsuleComponent()->GetScaledCapsuleRadius() + 5.f);
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), MPC_Player, "PlayerLocation",	FLinearColor(GetActorLocation()));
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), MPC_Player, "Velocity",			FLinearColor(GetVelocity()));
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), MPC_Player, "Forward",			FLinearColor(GetActorForwardVector()));
}
void ASteikemannCharacter::NS_Land_Implementation(const FHitResult& Hit)
{
	/* Play Landing particle effect */
	float Velocity = GetVelocity().Size();
	UNiagaraComponent* NiagaraPlayer{ nullptr };

	if (Component_Niagara->IsComplete())
	{
		NiagaraPlayer = Component_Niagara;
	}
	else
	{
		UNiagaraComponent* TempNiagaraLand = CreateNiagaraComponent("Niagara_Land", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, true);
		NiagaraPlayer = TempNiagaraLand;
	}

	NiagaraPlayer->SetAsset(NS_Land);
	NiagaraPlayer->SetNiagaraVariableInt("User.SpawnAmount", static_cast<int>(Velocity * NSM_Land_ParticleAmount));
	NiagaraPlayer->SetNiagaraVariableFloat("User.Velocity", m_EAttackState == EAttackState::Post_GroundPound ? Velocity * 3.f * NSM_Land_ParticleSpeed : Velocity * NSM_Land_ParticleSpeed);	// Change pending on character is groundpounding or not
	NiagaraPlayer->SetWorldLocationAndRotation(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	NiagaraPlayer->Activate(true);
}

// Called every frame
void ASteikemannCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SteikeWorldStatics::PlayerLocation = GetActorLocation();

	if (bIsDead) { return; }

	/* Rotate Inputvector to match the playercontroller */
	FRotator Rot = GetControlRotation();
	m_InputVector = InputVectorRaw.RotateAngleAxis(Rot.Yaw, FVector::UpVector);

	if (InputVectorRaw.Size() > 1.f || m_InputVector.Size() > 1.f)
		m_InputVector.Normalize();

	m_GamepadCameraInput = m_GamepadCameraInputRAW;
	if (m_GamepadCameraInput.Length() > 1.f)
		m_GamepadCameraInput.Normalize();

	/* PRINTING STATE MACHINE INFO */
#ifdef UE_BUILD_DEBUG
	Print_State();
	PRINTPAR("Attack State :: %i", m_EAttackState);
	PRINTPAR("Attack type :: %i", m_EAttackType);
	PRINTPAR("Grapple State :: %i", m_EGrappleState);
	PRINTPAR("Grapple Type  :: %i", m_EGrappleType);
	PRINTPAR("Smack Attack Type :: %i", m_ESmackAttackType);
	PRINTPAR("MovementInputState = %i", m_EMovementInputState);
	//PRINTPAR("Air State :: %i", m_EAirState);
	//PRINTPAR("Ground State :: %i", m_EGroundState);
	//PRINTPAR("Pogo Type :: %i", m_EPogoType);
	//PRINTPAR("Jump Count = %i", JumpCurrentCount);
	//switch (m_EInputType)
	//{
	//case EInputType::MouseNKeyboard:
	//	PRINT("Input Mode: MouseNKeyboard");
	//	break;
	//case EInputType::Gamepad:
	//	PRINT("Input Mode: Gamepad");
	//	break;
	//default:
	//	break;
	//}
#endif

	/*		Resets Rotation Pitch and Roll		*/
	if (IsFalling() || GetMoveComponent()->IsWalking()) {
		ResetActorRotationPitchAndRoll(DeltaTime);
	}
	
	/*		Post Edge Jump		*/
	if (GetCharacterMovement()->IsWalking()){ 
		bCanPostEdgeRegularJump = true;
		TimerManager.SetTimer(PostEdgeJump, [this]() { bCanPostEdgeRegularJump = false; }, PostEdge_JumpTimer_Length, false);
	}


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
		switch (m_EAirState)
		{
		case EAirState::AIR_None:
			break;
		case EAirState::AIR_Freefall:
			GetMoveComponent()->AirFriction2D(m_InputVector);
			break;
		case EAirState::AIR_Jump:
			break;
		case EAirState::AIR_Pogo:
			break;
		default:
			break;
		}

		switch (m_EPogoType)
		{
		case EPogoType::POGO_None:
			break;
		case EPogoType::POGO_Passive:
			PB_Passive_IMPL(m_PogoTarget);
			break;
		case EPogoType::POGO_Active:
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

			// Hang on wall Initial Contact 
			if (ValidateWall() && m_WallJumpData.valid)
			{
				GetMoveComponent()->Initial_OnWall_Hang(m_WallJumpData, OnWall_HangTime);
				m_EState = EState::STATE_OnWall;
				m_WallState = EOnWallState::WALL_Hang;
				Anim_OnWallContact();
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
		//case EAttackState::Scoop:
		//	break;
		case EAttackState::GroundPound:
			if (m_EPogoType == EPogoType::POGO_Groundpound) 
			{
				PB_Groundpound_IMPL(m_PogoTarget);
				break;
			}
			break;
		default:
			break;
		}
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
				TempNiagaraComponents[i]->DestroyComponent();
				TempNiagaraComponents.RemoveAt(i);
				i--;
			}
		}
	}

	
	/* --------------- CAMERA -------------------- */
	if (m_EState == EState::STATE_Grappling) {
		if (m_EGrappleType == EGrappleType::Dynamic_Ground) {
			if (m_EInputType == EInputType::MouseNKeyboard)
			{
				GH_GrappleSmackAimingVector.X = FMath::Clamp(GH_GrappleSmackAimingVector.X + m_MouseMovementInput.X * GH_GrappleSmackAiming_MNK_Multiplier.X, GrappleDynamic_MinPitch, GrappleDynamic_MaxPitch);
				GH_GrappleSmackAimingVector.Y = FMath::Clamp(GH_GrappleSmackAimingVector.Y + m_MouseMovementInput.Y * GH_GrappleSmackAiming_MNK_Multiplier.Y, -1.f, 1.f);

				GH_ShowGrappleSmackCurveIndicator(DeltaTime, 0.f);
				GrappleDynamicGuideCamera(GrappledActor.Get(), DeltaTime);
			}
			else 
			{
				GH_GrappleSmackAimingVector.X = FMath::Clamp(GH_GrappleSmackAimingVector.X + InputVectorRaw.X * GH_GrappleSmackAiming_Gamepad_Multiplier.X, GrappleDynamic_MinPitch, GrappleDynamic_MaxPitch);
				GH_GrappleSmackAimingVector.Y = FMath::Clamp(GH_GrappleSmackAimingVector.Y + InputVectorRaw.Y * GH_GrappleSmackAiming_Gamepad_Multiplier.Y, -1.f, 1.f);

				GH_ShowGrappleSmackCurveIndicator(DeltaTime, 0.f);
				GrappleDynamicGuideCamera(GrappledActor.Get(), DeltaTime);

				//GH_ShowGrappleSmackCurveIndicator_Gamepad(DeltaTime, 0.f);
				//GrappleDynamicGuideCamera_Gamepad(GrappledActor.Get(), DeltaTime);
			}
		}
		else if (Is_GH_StaticTarget() && m_EGrappleState == EGrappleState::Pre_Launch)
			GuideCamera_StaticGrapple(DeltaTime);
	}
	PlaceCameraBoom(DeltaTime);
	if (bCameraLerpBack_PostPrompt)
		bCameraLerpBack_PostPrompt = LerpCameraBackToBoom(DeltaTime);
	GuideCamera(DeltaTime);
	GuideCamera_Movement(DeltaTime);


	EndTick(DeltaTime);
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
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this,		&ASteikemannCharacter::Mouse_AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this,		&ASteikemannCharacter::Mouse_AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this,	&ASteikemannCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up/Down Gamepad", this,		&ASteikemannCharacter::LookUpAtRate);


		/* Jump */
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASteikemannCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::JumpRelease);

	/* -- HUD -- */
	PlayerInputComponent->BindAction("ShowHUD", IE_Pressed, this, &ASteikemannCharacter::ShowHUD_Timed_Pure);

	/* -- CancelButton -- */
	PlayerInputComponent->BindAction("CancelButton", IE_Pressed, this, &ASteikemannCharacter::Click_RightFacebutton);
	PlayerInputComponent->BindAction("CancelButton", IE_Released, this, &ASteikemannCharacter::UnClick_RightFacebutton);
	

	/* -- GRAPPLEHOOK -- */
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Pressed, this,	  &ASteikemannCharacter::RightTriggerClick);
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Released, this,	  &ASteikemannCharacter::RightTriggerUn_Click);


	/* -- SMACK ATTACK -- */
	PlayerInputComponent->BindAction("SmackAttack", IE_Pressed, this, &ASteikemannCharacter::Click_Attack);
	PlayerInputComponent->BindAction("SmackAttack", IE_Released, this, &ASteikemannCharacter::UnClick_Attack);

	/* Attack - GroundPound */
	PlayerInputComponent->BindAction("GroundPound", IE_Pressed, this, &ASteikemannCharacter::Click_GroundPound);
	PlayerInputComponent->BindAction("GroundPound", IE_Released, this, &ASteikemannCharacter::UnClick_GroundPound);
}

void ASteikemannCharacter::EnterPromptArea(ADialoguePrompt* promptActor, FVector promptLocation)
{
	m_PromptActor = promptActor;
	m_PromptState = EPromptState::WithingArea;
	m_PromptLocation = promptLocation;
}

void ASteikemannCharacter::LeavePromptArea()
{
	m_PromptState = EPromptState::None;
}

bool ASteikemannCharacter::ActivatePrompt()
{
	bool b{};
	switch (m_PromptState)
	{
	case EPromptState::None:			return false;

	case EPromptState::WithingArea:
		m_EMovementInputState = EMovementInput::Locked;
		m_PromptState = EPromptState::InPrompt;
		// Get first prompt state
		return m_PromptActor->GetNextPromptState(this, 0);

	case EPromptState::InPrompt:
		// Get next prompt state
		b = m_PromptActor->GetNextPromptState(this);
		if (!b) m_EMovementInputState = EMovementInput::Open;
		return b;
	default:
		break;
	}
	return b;
}

bool ASteikemannCharacter::ExitPrompt()
{
	m_EMovementInputState = EMovementInput::Open;
	m_PromptState = EPromptState::WithingArea;
	// Notify DialoguePrompt of exiting
	m_PromptActor->ExitPrompt_Pure();

	// Start lerping camera back to default position
	m_CameraLerpAlpha_PostPrompt = 0.f;
	bCameraLerpBack_PostPrompt = true;
	return false;
}

bool ASteikemannCharacter::LerpCameraBackToBoom(float DeltaTime)
{
	m_CameraLerpAlpha_PostPrompt = FMath::Min(m_CameraLerpAlpha_PostPrompt += DeltaTime * CameraLerpSpeed_Prompt, 1.f);
	FTransform TargetTransform = CameraBoom->GetSocketTransform(USpringArmComponent::SocketName);
	m_CameraTransform = Camera->GetComponentTransform();

	FVector lerpLocation = FMath::Lerp(m_CameraTransform.GetLocation(), TargetTransform.GetLocation(), m_CameraLerpAlpha_PostPrompt);
	FQuat lerpRotation = FMath::Lerp(m_CameraTransform.GetRotation(), TargetTransform.GetRotation(), m_CameraLerpAlpha_PostPrompt);

	FTransform finalTransform(lerpRotation, lerpLocation, FVector(1.f));
	Camera->SetWorldTransform(finalTransform);

	// End camera lerp
	if (m_CameraLerpAlpha_PostPrompt >= 1.f)
	{
		Camera->SetWorldTransform(m_CameraTransform);
		return false;
	}
	return true;
}

void ASteikemannCharacter::GC_SlerpTowardsVector(const FVector& direction, float alpha, float DeltaTime)
{
	auto p = GetPlayerController();
	FQuat TargetDirection{ direction.GetSafeNormal().Rotation() };
	FQuat NewDirection = FQuat::Slerp(p->GetControlRotation().Quaternion(), TargetDirection, alpha * DeltaTime);
	FRotator rot = NewDirection.Rotator(); rot.Roll = 0.f;
	p->SetControlRotation(rot);
}

void ASteikemannCharacter::PlaceCameraBoom(float DeltaTime)
{
	float Alpha{};
	FHitResult Hit;
	FCollisionQueryParams Params("", false, this);
	FVector TraceStart = GetActorLocation();
	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceStart - (FVector::UpVector * 500.f), ECC_WorldStatic, Params))
	{
		float Dist = FMath::Clamp((float)FVector::Dist(TraceStart, Hit.ImpactPoint) - (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f), 0.f, CameraBoom_LerpHeight);
		Alpha = -(Dist / CameraBoom_LerpHeight) + 1.f;
	}
	Alpha = FMath::Clamp(Alpha, -(CameraBoom_MinHeightFromRoot / CameraBoom_LerpHeight) + 1.f, 1.f);
	CameraBoom_PlacementAlpha = FMath::FInterpTo(CameraBoom_PlacementAlpha, Alpha, DeltaTime, CameraBoom_LerpSpeed);
	float Height = FMath::Clamp(CameraBoom_Location.Z * CameraBoom_PlacementAlpha, CameraBoom_MinHeightFromRoot, CameraBoom_Location.Z);
	CameraBoom->SetWorldLocation(GetActorLocation() + (FVector::UpVector * Height));
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
	if (ActionLocked()) return;
	if (bGrappleClick) return;
	//if (bAttackPress)   return;

	//if (TFunc_GrappleLaunchFunction)	return;
	bGrappleClick = true;
	PRINTLONG(2.f, "   ");
	PRINTLONG(2.f, "Right trigger CLICK");

	GH_Click();
}

void ASteikemannCharacter::RightTriggerUn_Click()
{
	bGrappleClick = false;
	PRINTLONG(2.f, "Right trigger RELEASE");
	//if (/*!GH_InvalidRelease || */!ActionLocked())
	if (bAttackPress)   return;
		GH_DelegateDynamicLaunch();
}

void ASteikemannCharacter::GH_Click()
{
	Print_State(2.f);
	GH_InvalidRelease = false;
	switch (m_EState)
	{
	case EState::STATE_None:		break;
	case EState::STATE_OnGround:
		TL_Dash_End();
		if (bPressedCancelButton)
		{
			if (!GrappledActor.IsValid()) return;
			Active_GrappledActor = GrappledActor;
			PullDynamicTargetOffWall();
			return;
		}
		break;
	case EState::STATE_InAir:
		if (GH_GrappleLaunchLandDelegate()) return;
		break;
	case EState::STATE_OnWall:
		CancelOnWall();
		break;
	case EState::STATE_Attacking:
		if (!GrappledActor.IsValid()) return;
		if (m_EAttackType == EAttackType::SmackAttack) {
			Cancel_SmackAttack();
		}
		else
			return;
		break;
	case EState::STATE_Grappling:	
		//GH_InvalidRelease = true;
		return;
	default:
		break;
	}

	if (!GrappledActor.IsValid()) return;
	Active_GrappledActor = GrappledActor;
	Active_GrappledActor_Location = Active_GrappledActor->GetActorLocation();
	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(Active_GrappledActor.Get());
	IGrappleTargetInterface* IGrapple = Cast<IGrappleTargetInterface>(Active_GrappledActor.Get());
	if (!ITag || !IGrapple) return;
	if (ITag->HasMatchingGameplayTag(Tag::AubergineDoggo()))
		GrappledEnemy = Active_GrappledActor;
	JumpCurrentCount = 1;	// Reset DoubleJump
	GC_StaticGrapple_Alpha = GC_StaticGrapple_StartSpeed;
	GH_GrappleSmackAimingVector = FVector2D(0.f);

	// GRAPPLING STARTS HERE
	GH_SetGrappleType(ITag, IGrapple);

	// Player Animations
	Anim_Grapple_Start();
}

void ASteikemannCharacter::GH_SetGrappleType(IGameplayTagAssetInterface* ITag, IGrappleTargetInterface* IGrapple)
{
	FGameplayTagContainer tags;
	ITag->GetOwnedGameplayTags(tags);

	GetMoveComponent()->StopJump();
	// Static Target
	if (tags.HasTag(Tag::GrappleTarget_Static()))
	{
		m_EGrappleType = EGrappleType::Static;
		GH_PreLaunch_Static(&ASteikemannCharacter::GH_Launch_Static, IGrapple);
		return;
	}

	// Dynamic Target 
	//	- If actor does not have tag grappletarget_dynamic, then return
	if (!tags.HasTag(Tag::GrappleTarget_Dynamic()))
		return;

		// Dynamic Target Stuck
	if (IGrapple->IsStuck_Pure())
	{
		ResetState();
		if (m_EState == EState::STATE_OnGround)		m_EGrappleType = EGrappleType::Static_StuckEnemy_Ground;
		if (m_EState == EState::STATE_InAir)		m_EGrappleType = EGrappleType::Static_StuckEnemy_Air;
		GH_PreLaunch_Static(&ASteikemannCharacter::GH_Launch_Static_StuckEnemy, IGrapple);
		return;
	}

		// Player in Air
	if (m_EState == EState::STATE_InAir)
	{
		GH_PreLaunch();
		IGrapple->HookedPure();
		IGrapple->HookedPure(GetActorLocation(), true, true);

		m_EGrappleType = EGrappleType::Dynamic_Air;
		GH_PreLaunch_Dynamic(IGrapple, false);
		return;
	}

		// Player on Ground -- Grapplehooking enemies to the player
	if (m_EState == EState::STATE_OnGround)
	{
		GH_PreLaunch();
		IGrapple->HookedPure();
		IGrapple->HookedPure(GetActorLocation(), true, true);
		m_EGrappleType = EGrappleType::Dynamic_Ground;
		GH_GrappleDynamic_Start();
		Cause_LeewayPause_Pure(AttackInterface_LeewayPause_Time);

		TFunc_GrappleLaunchFunction = [this, IGrapple]() {
			GH_Launch_Dynamic(IGrapple, true);
			//TFunc_GrappleLaunchFunction.Reset();
			return;
		};


		TimerManager.SetTimer(TH_GrappleHold, [this]() { TFunc_GrappleLaunchFunction(); TFunc_GrappleLaunchFunction.Reset(); }, GH_GrapplingEnemyHold, false);
		return;
	}
}

void ASteikemannCharacter::GH_PreLaunch_Static(void(ASteikemannCharacter::* LaunchFunction)(), IGrappleTargetInterface* IGrapple)
{
	GH_PreLaunch();
	IGrapple->HookedPure();

	//FTimerHandle h;
	TimerManager.SetTimer(TH_Grapplehook_Pre_Launch,
		[this]()
		{
			m_EGrappleState = EGrappleState::Post_Launch;
			GetMoveComponent()->m_GravityMode = EGravityMode::Default;
		},
		GrappleDrag_PreLaunch_Timer_Length, false);
	TimerManager.SetTimer(TH_Grapplehook_Start, this, LaunchFunction, GrappleDrag_PreLaunch_Timer_Length);

	// End GrappleHook Timer
	TimerManager.SetTimer(TH_Grapplehook_End_Launch, this, &ASteikemannCharacter::GH_Stop, GrappleDrag_PreLaunch_Timer_Length + GrappleHook_PostLaunchTimer);

	// End control rig 
	FTimerHandle h;
	TimerManager.SetTimer(h, this, &ASteikemannCharacter::GH_StopControlRig, GrappleDrag_PreLaunch_Timer_Length + (GrappleHook_PostLaunchTimer * 0.3));
}

void ASteikemannCharacter::GH_PreLaunch_Dynamic(IGrappleTargetInterface* IGrapple, bool OnGround)
{
	TimerManager.SetTimer(TH_Grapplehook_Pre_Launch,
		[this, IGrapple, OnGround]()
		{
			m_EGrappleState = EGrappleState::Post_Launch;
			GetMoveComponent()->m_GravityMode = EGravityMode::Default;
			IGrapple->HookedPure(GetActorLocation(), OnGround, false);

			// Animations
			Anim_Grapple_End_Pure();
		},
		GrappleDrag_PreLaunch_Timer_Length, false);

	// End GrappleHook Timer
	TimerManager.SetTimer(TH_Grapplehook_End_Launch, this, &ASteikemannCharacter::GH_Stop, GrappleDrag_PreLaunch_Timer_Length + GrappleHook_PostLaunchTimer);

	// End control rig 
	FTimerHandle h;
	TimerManager.SetTimer(h, this, &ASteikemannCharacter::GH_StopControlRig, GrappleDrag_PreLaunch_Timer_Length + (GrappleHook_PostLaunchTimer * 0.3));
}

void ASteikemannCharacter::GH_Launch_Dynamic(IGrappleTargetInterface* IGrapple, bool OnGround)
{
	GH_GrappleDynamic_End();
	m_EGrappleState = EGrappleState::Post_Launch;
	GetMoveComponent()->m_GravityMode = EGravityMode::Default;
	IGrapple->HookedPure(GetActorLocation(), OnGround, false);
	TLComp_AirFriction->PlayFromStart();

	// Animations
	Anim_Grapple_End_Pure();

	// End GrappleHook Timer
	TimerManager.SetTimer(TH_Grapplehook_End_Launch, this, &ASteikemannCharacter::GH_Stop, GrappleHook_PostLaunchTimer);

	// End control rig 
	FTimerHandle h;
	TimerManager.SetTimer(h, this, &ASteikemannCharacter::GH_StopControlRig, (GrappleHook_PostLaunchTimer * 0.3));
}

bool ASteikemannCharacter::Is_GH_StaticTarget() const
{
	return m_EGrappleType == EGrappleType::Static || m_EGrappleType == EGrappleType::Static_StuckEnemy_Ground || m_EGrappleType == EGrappleType::Static_StuckEnemy_Air;
}

void ASteikemannCharacter::GH_PreLaunch()
{
	m_EState = EState::STATE_Grappling;
	m_EGrappleState = EGrappleState::Pre_Launch;
	GetMoveComponent()->m_GravityMode = EGravityMode::ForcedNone;	// Fredrik hadde en crash her når han lekte med infinite grapple på static target
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
	TLComp_AirFriction->PlayFromStart();

	// Animations
	Anim_Grapple_End_Pure();
}

void ASteikemannCharacter::GH_Launch_Static_StuckEnemy()
{
	GetMoveComponent()->DeactivateJumpMechanics();
	FVector GrappledLocation = GH_GetTargetLocation();
	FVector Direction = GrappledLocation - GetActorLocation();
	FVector Direction2D = FVector(Direction.X, Direction.Y, 0);

	FVector Velocity = Direction2D / GrappleHook_Time_ToStuckEnemy;

	float z{};
	z = ((GrappledLocation.Z + GrappleHook_AboveStuckEnemy) - GetActorLocation().Z);

	Velocity.Z = (z / GrappleHook_Time_ToStuckEnemy) + (0.5 * m_BaseGravityZ * GrappleHook_Time_ToStuckEnemy * -1.f);
	GetMoveComponent()->AddImpulse(Velocity, true);
	TLComp_AirFriction->PlayFromStart();

	// When grapple hooking to stuck enemy, set a wall jump activation timer 
	m_WallState = EOnWallState::WALL_Leave;
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, GrappleHook_Time_ToStuckEnemy + OnWallActivation_PostStuckEnemyGrappled, false);

	// Animations
	Anim_Grapple_End();
}

void ASteikemannCharacter::GH_Stop(EState newstate)
{
	Active_GrappledActor = nullptr;

	m_EState = newstate;
	m_EGrappleState = EGrappleState::None;
	m_EGrappleType = EGrappleType::None;

	SetDefaultState();
}

void ASteikemannCharacter::GH_Stop()
{
	GH_Stop(EState::STATE_None);
}

void ASteikemannCharacter::GH_Cancel()
{
	TFunc_GrappleLaunchFunction.Reset();
	IGrappleTargetInterface* iGrapple = Cast<IGrappleTargetInterface>(Active_GrappledActor);
	if (iGrapple)
		iGrapple->UnHookedPure();

	GH_Stop();
	TimerManager.ClearTimer(TH_GrappleHold);
	TimerManager.ClearTimer(TH_Grapplehook_Start);
	TimerManager.ClearTimer(TH_Grapplehook_Pre_Launch);
	TimerManager.ClearTimer(TH_Grapplehook_End_Launch);
	GetMoveComponent()->m_GravityMode = EGravityMode::Default;
	GH_GrappleDynamic_End();
	if (GetMoveComponent()->IsWalking())
		JumpCurrentCount = 0;
}

void ASteikemannCharacter::PullDynamicTargetOffWall()
{
	if (!Active_GrappledActor.IsValid()) return;
	IGrappleTargetInterface* IGrapple = Cast<IGrappleTargetInterface>(Active_GrappledActor);
	if (!IGrapple) return;
	IGrapple->PullFree_Pure(GetActorLocation());

	TimerManager.ClearTimer(TH_Grapplehook_Start);
	TimerManager.ClearTimer(TH_Grapplehook_Pre_Launch);
	TimerManager.ClearTimer(TH_Grapplehook_End_Launch);

	GetMoveComponent()->m_GravityMode = EGravityMode::Default;
	GH_Stop();

	m_EMovementInputState = EMovementInput::Locked;
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { m_EMovementInputState = EMovementInput::Open; }, GH_PostPullingTargetFreeTime, false);
}

bool ASteikemannCharacter::GH_GrappleLaunchLandDelegate()
{
	if (IsOnGround()) return false;
	if (Delegate_GrappleEnemyOnLand.IsBound()) return false;

	if (CheckStaticWorldBeneathCharacter(150.f))
	{
		Delegate_GrappleEnemyOnLand.BindUObject(this, &ASteikemannCharacter::GH_Click);
		TimerManager.SetTimer(TH_UnbindGrappleEnemyOnLand, [this]() { Delegate_GrappleEnemyOnLand.Unbind(); }, 1.f, false);

		return true;
	}
	return false;
}

FVector ASteikemannCharacter::GH_GetTargetLocation() const
{
	if (Active_GrappledActor.IsValid())
		return Active_GrappledActor->GetActorLocation();
	if (GrappledActor.IsValid())
		 return GrappledActor->GetActorLocation();
	if (GrappledEnemy.IsValid())
		return GrappledEnemy->GetActorLocation();
	return FVector();
}

void ASteikemannCharacter::StartAnimLerp_ControlRig()
{
	bGH_LerpControlRig = true;
}

FVector ASteikemannCharacter::GH_GrappleSmackAiming_MNK(AActor* Target)
{
	if (!Target) return FVector();
	FVector GrappledActorDirection = FVector(Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

	FVector Direction = GrappledActorDirection.RotateAngleAxis(GrappleDynamic_MaxYaw * GH_GrappleSmackAimingVector.Y, FVector::UpVector);

	float Pitch = FMath::Clamp(GH_GrappleSmackAimingVector.X, GrappleDynamic_MinPitch, GrappleDynamic_MaxPitch);
	float PitchAngle = asinf(Pitch);
	Direction = (cosf(PitchAngle) * Direction) + (sinf(PitchAngle) * FVector::UpVector);

	return Direction;
}

// Decrepid
void ASteikemannCharacter::GH_ShowGrappleSmackCurveIndicator_Gamepad(float DeltaTime, float DrawTime)
{
	if (!Active_GrappledActor.IsValid()) return;

	/**
	* Showing the initial Impulse
	*/ 
	FVector Direction = GetControlRotation().Vector().GetSafeNormal2D();
	float AdditionalStrength{ 1.f };
	float angle{};
	if (m_InputVector.IsNearlyZero())
		Direction = (Direction + (GetActorForwardVector() * SmackDirection_InputMultiplier)).GetSafeNormal2D();
	else {
		Direction = (Direction + (m_InputVector * SmackDirection_InputMultiplier) + (GetControlRotation().Vector().GetSafeNormal2D() * SmackDirection_CameraMultiplier)).GetSafeNormal2D();
	}
	angle = FMath::DegreesToRadians(SmackUpwardAngle);
	angle = angle + (angle * (((InputVectorRaw.X + InputVectorRaw.Length()) / 2.f) * SmackAttack_InputAngleMultiplier));
	Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);
	float strength = (GrappleSmack_Strength * AdditionalStrength + (GrappleSmack_Strength * (InputVectorRaw.X * Grapplesmack_DirectionMultiplier)));
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Direction * strength * DeltaTime, FColor::Red, false, DrawTime, -1, 6.f);

	GH_ShowGrappleSmackCurve(DeltaTime, Direction, strength, DrawTime);
}

void ASteikemannCharacter::GH_ShowGrappleSmackCurveIndicator(float DeltaTime, float DrawTime)
{
	if (!Active_GrappledActor.IsValid()) return;

	FVector Direction = GH_GrappleSmackAiming_MNK(Active_GrappledActor.Get());
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + Direction * SmackAttackStrength, FColor::Red, false, DrawTime, 0, 4.f);

	GH_ShowGrappleSmackCurve(DeltaTime, Direction, GrappleSmack_Strength + (GrappleSmack_Strength * Grapplesmack_DirectionMultiplier), DrawTime);
}

void ASteikemannCharacter::GH_ShowGrappleSmackCurve(float DeltaTime, FVector Direction, float SmackStrength, float DrawTime)
{
	/**
	* Showing the curve the grappled enemy will fall along
	*/
	FVector Velocity = Direction * SmackStrength;
	FVector Gravity = FVector(0.f, 0.f, GetWorld()->GetGravityZ() * 4.f);
	float time = DeltaTime;
	FVector start = GetActorLocation();
	FVector end = start + Velocity * DeltaTime;
	for (; time < 4.f; time += DeltaTime)
	{
		if (GH_ShowGrappleSmackImpactIndicator(start, end, DrawTime)) break;
		//DrawDebugLine(GetWorld(), start, end, FColor::Blue, false, DrawTime, -1, 6.f);
		Velocity += Gravity * DeltaTime;
		start = end;
		end += Velocity * DeltaTime;
	}
}

bool ASteikemannCharacter::GH_ShowGrappleSmackImpactIndicator(FVector start, FVector end, float DrawTime)
{
	FHitResult Hit;
	FCollisionQueryParams Params("", false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, start, end, ECC_Visibility, Params))
	{
		//DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 20.f, FColor::Orange, false, DrawTime);
		GH_AimIndicator_LocationDirection(Hit.ImpactPoint, Hit.ImpactNormal, FVector(start - end).GetSafeNormal(), FVector(Hit.ImpactPoint - Camera->GetComponentLocation()).GetSafeNormal());
		return true;
	}
	return false;
}

void ASteikemannCharacter::Cause_LeewayPause_Pure(float Pausetime)
{
	/* Just iterating through the InReachGrappleTargets for now. 
	* It is the current method of checking which relevant actors that are nearby without having to create a new method.
	* Though the method of checking should probably be done through the gamemode or 
	* something. Where it can know about the active enemies and iterate through them to call the interface function.
	*/
	for (auto& target : InReachGrappleTargets)
	{
		if (target == Active_GrappledActor) continue;
		if (auto iattack = Cast<IAttackInterface>(target)) {
			iattack->Receive_LeewayPause_Pure(Pausetime);
		}
	}
}

void ASteikemannCharacter::Anim_Grapple_End_Pure()
{
	Anim_Grapple_End();
}

void ASteikemannCharacter::GH_StopControlRig()
{
	// Animation
	bGH_LerpControlRig = false;
}

void ASteikemannCharacter::GH_DelegateDynamicLaunch()
{
	if (!TFunc_GrappleLaunchFunction)	return;

	// Call grapple launch when releasing button, but only if the minimal time (GrappleDrag_PreLaunch_Timer_Length) has elapsed
	if (TimerManager.GetTimerElapsed(TH_GrappleHold) < GrappleDrag_PreLaunch_Timer_Length) {
		TimerManager.SetTimer(TH_GrappleHold, [this]() { TFunc_GrappleLaunchFunction(); TFunc_GrappleLaunchFunction.Reset(); }, GrappleDrag_PreLaunch_Timer_Length - TimerManager.GetTimerElapsed(TH_GrappleHold), false);
		return;
	}
	else {	// SJEKK UTEN DENNE
		TimerManager.ClearTimer(TH_GrappleHold);
		TFunc_GrappleLaunchFunction();
		TFunc_GrappleLaunchFunction.Reset();
	}
}


UPhysicalMaterial* ASteikemannCharacter::DetectPhysMaterial()
{
	FVector Start = GetActorLocation();
	FVector End = GetActorLocation() - FVector(0, 0, 100);
	FHitResult Hit;

	FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);
	Params.bReturnPhysicalMaterial = true;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

	if (bHit)
	{
		return Hit.PhysMaterial.Get();

		// Decrepid 
		GetMoveComponent()->Traced_GroundFriction = Hit.PhysMaterial->Friction;
		TEnumAsByte<EPhysicalSurface> surface = Hit.PhysMaterial->SurfaceType;

		FString string;
		switch (surface)
		{
		case SurfaceType_Default:
			string = "Default";
			break;

		case SurfaceType1:
			string = "Slippery";
			break;
		default:
			break;
		}
	}
	return nullptr;
}

void ASteikemannCharacter::PlayCameraShake(TSubclassOf<UCameraShakeBase> shake, float falloff)
{
	/* Skal regne ut Velocity.Z ogs� basere falloff p� hvor stor den er */
	UGameplayStatics::PlayWorldCameraShake(GetWorld(), shake, Camera->GetComponentLocation() + FVector(0, 0, 1), 0.f, 10.f, falloff);
}

/* ----------------- Read this function from the bottom->up after POINT ----------------- */
void ASteikemannCharacter::GuideCamera(float DeltaTime)
{
	/* Lambda function for SLerp Playercontroller Quaternion to target Quat */	// Kunne bare v�rt en funksjon som tar inn en enum verdi
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
	FocusPoint FP = mFocusPoints[0];	// Hardkoder for f�rste array punkt n�, S� vil ikke h�ndterer flere volumes og prioriteringene mellom dem
	/* Get location used for */
	switch (FP.ComponentType)
	{
	case EFocusType::FOCUS_Point:
		FP.Location = FP.ComponentLocation;
		break;
	case EFocusType::FOCUS_Spline:
		// Bruk Internal_key til � finne lokasjonen
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

		/* Juster for h�yden */
		float Zdiff = FP.Location.Z - GetActorLocation().Z;
		if (Zdiff < 0){ Zdiff *= -1.f; }
		Z += Zdiff * CameraGuide_ZdiffMultiplier;
		
		return Z;
	};



	auto LA_Absolute = [&](FocusPoint& P, float& alpha, APlayerController* Con) {
		alpha >= 1.f ? alpha = 1.f : alpha += DeltaTime * P.LerpSpeed;
		FQuat VToP{ ((P.Location - FVector(0, 0, PitchAdjust(CameraGuide_Pitch, CameraGuide_Pitch_MIN, CameraGuide_Pitch_MAX)/*Pitch Adjustment*/)) - CameraBoom->GetComponentLocation()).Rotation() };	// Juster pitch adjustment basert p� z til VToP uten adjustment

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
		// Set Target Arm length slik at kamera sitter p� target
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
	FVector right = FVector::CrossProduct(FVector::UpVector, old);
	old = old.RotateAngleAxis(90.f * z, right);
	//old.X *= 1.f - FMath::Abs(z);
	//old.Y *= 1.f - FMath::Abs(z);
	//old.Z = z;
	//old.Normalize();
	FQuat target{ old.Rotation() };

	FQuat Rot{ GetPlayerController()->GetControlRotation() };
	FQuat New{ FQuat::Slerp(Rot, target, alpha) };
	FRotator Rot1 = New.Rotator();
	Rot1.Roll = 0.f;
	GetPlayerController()->SetControlRotation(Rot1);
}

	// decrepid -- not in use
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

void ASteikemannCharacter::GrappleDynamicGuideCamera_Gamepad(AActor* target, float deltatime)
{
	if (!target) return;

	FVector input = InputVectorRaw;
	FVector Direction;

	FVector grappled = target->GetActorLocation();
	grappled.Z = GetActorLocation().Z;
	FVector toGrapple = (grappled - GetActorLocation()).GetSafeNormal();
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
	float x = FMath::Abs(input.X);
	float alphaX = 1.f - x;

	// Default Guide towards grappled target
	FVector defaultDir = toGrapple2D;
	defaultDir.Z = GrappleDynamic_DefaultPitch;

	FVector currentdirection = defaultDir;
	FVector up = FVector::CrossProduct(currentdirection, FVector::CrossProduct(FVector::UpVector, currentdirection));
	FVector pitchDirection = ((currentdirection * 1.f) + (up * FMath::Clamp(input.X, GrappleDynamic_MinPitch, GrappleDynamic_MaxPitch)));
	GuideCameraTowardsVector(pitchDirection, x * GrappleDynamic_PitchAlpha);

	GuideCameraTowardsVector(defaultDir, alphaX * GrappleDynamic_DefaultAlpha);
}

void ASteikemannCharacter::GrappleDynamicGuideCamera(AActor* target, float deltatime)
{
	if (!target) return;

	FVector toGrapple = (target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	FVector toGrapple2D = toGrapple.GetSafeNormal2D();

	// Default Guide towards grappled target
	FVector defaultDir = toGrapple2D;
	defaultDir.Z = GrappleDynamic_DefaultPitch;
	FVector AimDirection = GH_GrappleSmackAiming_MNK(target);
	GuideCameraTowardsVector(defaultDir + (AimDirection.GetSafeNormal2D() * GH_GrappleSmackAiming_MNK_CameraWeight.Y) + (FVector(0,0,AimDirection.Z * GH_GrappleSmackAiming_MNK_CameraWeight.X)), 0.1f);
}

void ASteikemannCharacter::GuideCamera_Movement(float DeltaTime)
{
	auto c = GetCharacterMovement();
	FVector vel = c->Velocity;
	if (m_GamepadCameraInputRAW.SquaredLength() > 0.1f || vel.SquaredLength() < 5.f) {
		GC_Mov_Alpha = FMath::Max(GC_Mov_Alpha - (GC_Mov_BaseNegativeLerpSpeed * DeltaTime * GC_Mov_NegativeLerpSpeed), 0.f);
		return;
	}

	auto p = GetPlayerController();
	float velValue = vel.Length() / GC_Mov_MaxVelocity;
	float dot = FVector::DotProduct(p->GetControlRotation().Vector().GetSafeNormal(), vel.GetSafeNormal());
	float dotN = (dot / 2.f) + 0.5f;
	if (dot < GC_Mov_DotproductLimit) {
		GC_Mov_Alpha = FMath::Min(GC_Mov_Alpha + (dotN * velValue * DeltaTime * GC_Mov_PositiveLerpSpeed), GC_Mov_Alpha_MAX);
	}
	else {
		GC_Mov_Alpha = FMath::Max(GC_Mov_Alpha - (dotN * velValue * DeltaTime * GC_Mov_NegativeLerpSpeed), 0.f);
	}
	GC_SlerpTowardsVector(vel, GC_Mov_Alpha, DeltaTime);
}

void ASteikemannCharacter::GuideCamera_StaticGrapple(float DeltaTime)
{
	if (!Active_GrappledActor.Get()) 
		return;

	GC_StaticGrapple_Alpha = FMath::Max(GC_StaticGrapple_Alpha - (GC_StaticGrapple_SlerpSpeed * DeltaTime), 0.f);
	FVector dir = Active_GrappledActor->GetActorLocation() - GetActorLocation();
	dir.Z *= GC_StaticGrapple_PitchMulti;
	GC_SlerpTowardsVector(dir, GC_StaticGrapple_Alpha, DeltaTime);
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
		if (m_EAirState == EAirState::AIR_Pogo)				return;
		break;
	case EState::STATE_OnWall:								return;
	case EState::STATE_Attacking: 
		if (m_EAttackState == EAttackState::GroundPound)
			if (GetCharacterMovement()->IsWalking())
				m_EState = EState::STATE_OnGround;
		return;
	case EState::STATE_Grappling:							return;
	default:
		break;
	}

	ResetState();
}

void ASteikemannCharacter::AllowActionCancelationWithInput()
{
	switch (m_EMovementInputState)
	{
	case EMovementInput::Open:
		break;
	case EMovementInput::Locked:
		m_EMovementInputState = EMovementInput::Open;
		BreakMovementInput(InputVectorRaw.X);
		BreakMovementInput(InputVectorRaw.Y);
		break;
	case EMovementInput::PeriodLocked:
		break;
	default:
		break;
	}
}

bool ASteikemannCharacter::BreakMovementInput(float value)
{
	if (m_EMovementInputState == EMovementInput::Locked || m_EMovementInputState == EMovementInput::PeriodLocked) return true;
	switch (m_EState)
	{
	case EState::STATE_OnGround:
		if (m_EAttackState == EAttackState::Post_GroundPound && (value > 0.3f || value < -0.3f)) {
			m_EAttackState = EAttackState::None;
			StopAnimMontage();
		}
		break;
	case EState::STATE_InAir:  break;
	case EState::STATE_OnWall: return true;
	case EState::STATE_Attacking:
	{
		if (m_EMovementInputState == EMovementInput::Open && (value > 0.1f || value < -0.1f) && m_EAttackState != EAttackState::GroundPound)
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
	default:					  break;
	}
	return false;
}

bool ASteikemannCharacter::ActionLocked() const
{
	return (m_EMovementInputState == EMovementInput::Locked || m_EMovementInputState == EMovementInput::PeriodLocked);
}

void ASteikemannCharacter::MoveForward(float value)
{
	InputVectorRaw.X = value;

	if (BreakMovementInput(value)) return;

	float movement = value;

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

void ASteikemannCharacter::AddControllerYawInput(float Val)
{
	if (m_PromptState == EPromptState::InPrompt) return;
	Super::AddControllerYawInput(Val);
}

void ASteikemannCharacter::AddControllerPitchInput(float Val)
{
	if (m_PromptState == EPromptState::InPrompt) return;
	Super::AddControllerPitchInput(Val);
}

void ASteikemannCharacter::Mouse_AddControllerYawInput(float Val)
{
	if (m_EInputType != EInputType::MouseNKeyboard) return;
	m_MouseMovementInput.Y = Val;
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
		break;
	case EState::STATE_Grappling:
		if (m_EGrappleType == EGrappleType::Dynamic_Ground)
			return;
		break;
	default:
		break;
	}
	AddControllerYawInput(Val);
}

void ASteikemannCharacter::Mouse_AddControllerPitchInput(float Val)
{
	if (m_EInputType != EInputType::MouseNKeyboard) return;
	m_MouseMovementInput.X = Val;
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
		break;
	case EState::STATE_Grappling:
		if (m_EGrappleType == EGrappleType::Dynamic_Ground)
			return;
		break;
	default:
		break;
	}
	AddControllerPitchInput(Val);
}


void ASteikemannCharacter::TurnAtRate(float rate)
{
	if (m_EInputType != EInputType::Gamepad) return;
	m_GamepadCameraInputRAW.Y = rate;
	if (m_PromptState == EPromptState::InPrompt) return;
	switch (m_EState)	// No camera turns for controller during certain actions
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
		break;
	case EState::STATE_Grappling:
		if (m_EGrappleType == EGrappleType::Dynamic_Ground) return;
		break;
	default:
		break;
	}
	AddControllerYawInput(rate * TurnRate * GetWorld()->GetDeltaSeconds());
}

void ASteikemannCharacter::LookUpAtRate(float rate)
{
	if (m_EInputType != EInputType::Gamepad) return;
	m_GamepadCameraInputRAW.X = rate;
	if (m_PromptState == EPromptState::InPrompt) return;
	switch (m_EState)	// No camera turns for controller during certain actions
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
		break;
	case EState::STATE_Grappling:
		if (m_EGrappleType == EGrappleType::Dynamic_Ground) return;
		break;
	default:
		break;
	}
	AddControllerPitchInput(rate * TurnRate * GetWorld()->GetDeltaSeconds());
}

void ASteikemannCharacter::LockMovementForPeriod(float time, TFunction<void()> lambdaCall)
{
	m_EMovementInputState = EMovementInput::PeriodLocked;
	TimerManager.SetTimer(TH_MovementPeriodLocked, [this, lambdaCall]() { 
		m_EMovementInputState = EMovementInput::Open; 
		PostLockedMovementDelegate.Execute(lambdaCall);
		}, time, false);
}

void ASteikemannCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (bIsDead)
	{
		DeathDelegate_Land.Execute();
		DeathDelegate_Land.Unbind();
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("Landed"));
	switch (m_EState)
	{
	case EState::STATE_None:
		break;
	case EState::STATE_OnGround:
		break;
	case EState::STATE_InAir:
		if (m_EAirState == EAirState::AIR_Pogo)
			return;
		Anim_Land();
		break;
	case EState::STATE_OnWall:
		break;
	case EState::STATE_Attacking:
		if (bPB_Groundpound_PredeterminedPogoHit)
			return;
		if (m_EAttackState == EAttackState::GroundPound)
		{
			if (IsGroundPounding()) {
				GroundPoundLand(Hit);
			}
		}
		break;
	case EState::STATE_Grappling:
		break;
	default:
		break;
	}

	OnLanded(Hit);
	LandedDelegate.Broadcast(Hit);

	bPB_Groundpound_PredeterminedPogoHit = false;

	if (Delegate_GrappleEnemyOnLand.IsBound()) {
		m_EState = EState::STATE_OnGround;
		Delegate_NextFrameDelegate.AddUObject(this, &ASteikemannCharacter::GH_Click);
		Delegate_GrappleEnemyOnLand.Unbind();
		TimerManager.ClearTimer(TH_UnbindGrappleEnemyOnLand);
	}
}

void ASteikemannCharacter::Jump()
{
	if (ActivatePrompt()) return;

	// Cases where Jump is skipped
	switch (m_EState)
	{
	case EState::STATE_None:		break;
	case EState::STATE_OnGround:	break;
	case EState::STATE_InAir:		break;
	case EState::STATE_OnWall:		break;
	case EState::STATE_Attacking:	break;
	case EState::STATE_Grappling:	break;
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
			Jump_OnGround();
			break;
		}
		case EState::STATE_InAir:	// Double Jump
		{
			if (m_EPogoType != EPogoType::POGO_Leave)
				if (PB_Active_TargetDetection())
				{
					UE_LOG(LogTemp, Warning, TEXT("----------------"));

					m_EPogoType = EPogoType::POGO_Active;
					break;
				}

			if (GetMoveComponent()->IsFalling() && JumpCurrentCount == 0)
			{
				if (bCanPostEdgeRegularJump)
					Jump_OnGround();
				else 
					Jump_DoubleJump();
				break;
			}
			if (CanDoubleJump())
			{
				Jump_DoubleJump();
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
				TLComp_AirFriction->PlayFromStart();
				GetMoveComponent()->LedgeJump(m_InputVector, JumpStrength);
				GetMoveComponent()->m_GravityMode = EGravityMode::LerpToDefault;
				TimerManager.SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, 0.5f, false);
				break;
			}

			// Walljump
			m_EState = EState::STATE_InAir;
			m_WallState = EOnWallState::WALL_Leave;
			
			
			if (m_InputVector.SizeSquared() > 0.5f)
				RotateActorYawToVector(m_InputVector);
			else
				RotateActorYawToVector(m_WallJumpData.Normal);

			TimerManager.SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, OnWall_Reset_OnWallJump_Timer, false);
			GetMoveComponent()->WallJump(m_InputVector, JumpStrength);
			Anim_Activate_Jump();//Anim_Activate_WallJump
			TLComp_AirFriction->PlayFromStart();
			break;
		}
		case EState::STATE_Attacking:
		{
			break;
		}
		case EState::STATE_Grappling:
		{
			if (m_EGrappleState == EGrappleState::Post_Launch)
			{
				Jump_DoubleJump();
			}
			if (m_EGrappleState == EGrappleState::Pre_Launch)
			{
				if (m_EGrappleType == EGrappleType::Dynamic_Ground) {
					GH_Cancel();
					Jump_OnGround();
				}
			}
			break;
		}
		default:
			break;
		}
	}
}

void ASteikemannCharacter::JumpRelease()
{
	bJumpClick = false;
}

void ASteikemannCharacter::Jump_OnGround()
{
	JumpCurrentCount++;
	GetMoveComponent()->Jump(JumpStrength);
	Anim_Activate_Jump();
	TLComp_AirFriction->PlayFromStart();
	m_EState = EState::STATE_InAir;
	TL_Dash_End();

	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, OnWallActivation_PostJumpingOnGround, false);
	m_WallState = EOnWallState::WALL_Leave;
}

void ASteikemannCharacter::Jump_DoubleJump()
{
	JumpCurrentCount = 2;
	GetMoveComponent()->DoubleJump(m_InputVector.GetSafeNormal(), JumpStrength * DoubleJump_MultiplicationFactor);
	GetMoveComponent()->StartJumpHeightHold();
	Anim_Activate_DoubleJump();//Anim DoubleJump
	TLComp_AirFriction->PlayFromStart();

	RotateActorYawToVector(m_InputVector);
}

void ASteikemannCharacter::Jump_Undetermined()
{
	m_EMovementInputState = EMovementInput::Open;
	if (CanDoubleJump()) {
		Jump_DoubleJump();
		return;
	}
	Jump_OnGround();
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
	return  GetMoveComponent()->MovementMode == MOVE_Falling;
}

bool ASteikemannCharacter::IsOnGround() const
{
	if (!GetMoveComponent().IsValid()) { return false; }
	return GetMoveComponent()->MovementMode == MOVE_Walking;
}

void ASteikemannCharacter::CancelAnimationMontageIfMoving(TFunction<void()> lambdaCall)
{
	if (InputVectorRaw.SquaredLength() > 0.5)
		StopAnimMontage();
	if (lambdaCall)
		lambdaCall;
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
	if (!OtherActor) return false;
	// Validate Distance
	if (!PB_ValidTargetDistance(OtherActor->GetActorLocation()))	
		return false;
	if (GetCharacterMovement()->Velocity.Z > 0.f)
		return false;

	// Launch Passive Pogo
	PB_EnterPogoState(PB_StateTimer_Passive);
	PB_Launch_Passive();

	// Animation
	Anim_Pogo_Passive();

	// Affect Pogo Target

	return true;
}

void ASteikemannCharacter::PB_Launch_Passive()
{
	FVector Direction = GetCharacterMovement()->Velocity.GetSafeNormal2D();
	Direction = (Direction + m_InputVector) / 2.f;

	GetCharacterMovement()->Velocity *= 0.f;
	GetCharacterMovement()->AddImpulse((FVector::UpVector * PB_LaunchStrength_Z_Passive) + (Direction * PB_LaunchStrength_MultiXY_Passive), true);

}

void ASteikemannCharacter::PB_Active_IMPL()
{
	m_EAirState = EAirState::AIR_Pogo;
	m_EPogoType = EPogoType::POGO_Leave;
	//TimerManager.ClearTimer(TH_Pogo);

	PB_Launch_Active();
	PB_EnterPogoState(PB_StateTimer_Active);

	TimerManager.SetTimer(TH_PB_ExitHandle, [this](){ m_EPogoType = EPogoType::POGO_None; }, PB_StateTimer_Active, false);

	// Visual Effects
	Anim_Pogo_Active();
}

void ASteikemannCharacter::PB_Launch_Active()
{
	FVector direction = FVector((FVector::UpVector * (1.f - (m_InputVector.Size() * PB_InputMulti_Active))) + (m_InputVector * PB_InputMulti_Active)).GetSafeNormal();
	GetMoveComponent()->PB_Launch_Active(direction, PB_LaunchStrength_Active);
}

bool ASteikemannCharacter::PB_Groundpound_IMPL(AActor* OtherActor)
{
	if (!PB_ValidTargetDistance(OtherActor->GetActorLocation()) && !bPB_Groundpound_LaunchNextFrame)
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

	FVector Direction = ((FVector::UpVector * (1.f - PB_InputMulti_Groundpound)) + (m_InputVector * PB_InputMulti_Groundpound)).GetSafeNormal();
	GetCharacterMovement()->AddImpulse(Direction * PB_LaunchStrength_Groundpound, true);	// Simple method of bouncing player atm
	Anim_Activate_Jump();	// Anim_Pogo-Groundpound 
}

bool ASteikemannCharacter::PB_Groundpound_Predeterminehit()
{
	FCollisionQueryParams Params("", false, this);

	// Air Target
	float capWidth{ 1.5f };
	FCollisionShape Cap = FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius() * capWidth, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * capWidth);
	TArray<FHitResult> AirHits;
	bool air = GetWorld()->SweepMultiByChannel(AirHits, GetActorLocation(), GetActorLocation(), FQuat(1.f, 0.f, 0.f, 0.f), ECC_PogoCollision, Cap, Params);
	if (air) {
		for (const auto& Hit : AirHits) {
			if (IGameplayTagAssetInterface* tag = Cast<IGameplayTagAssetInterface>(Hit.GetActor())) {
				if (tag->HasMatchingGameplayTag(Tag::Enemy())) {
					bPB_Groundpound_LaunchNextFrame = true;
					PB_Groundpound_TargetActor = Hit.GetActor();
					return true;
				}
			}
		}
	}

	// Ground
	FHitResult GroundHit;
	bool ground = GetWorld()->LineTraceSingleByChannel(GroundHit, GetActorLocation() + FVector(0,0, 150.f), GetActorLocation() - FVector(0, 0, 1000.f), ECC_WorldStatic, Params);
	if (!ground)
	{
		ground = GetWorld()->LineTraceSingleByChannel(GroundHit, GetActorLocation() + FVector(0, 0, 150.f), GetActorLocation() - FVector(0, 0, 1000.f), ECC_PogoCollision, Params);
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
		m_EPogoType = EPogoType::POGO_Groundpound;
		return;
	}
	m_EPogoType = EPogoType::POGO_Passive;
}

void ASteikemannCharacter::PB_EnterPogoState(float time)
{
	m_EAirState = EAirState::AIR_Pogo;
	bPB_Groundpound_LaunchNextFrame = false;
	bPB_Groundpound_PredeterminedPogoHit = false;
	TimerManager.SetTimer(TH_Pogo, [this](){ m_EAirState = EAirState::AIR_Freefall;  }, time, false);
}


void ASteikemannCharacter::Dash_Start()
{
	m_EGroundState = EGroundState::GROUND_Dash;
	TLComp_Dash->PlayFromStart();
	TimerManager.SetTimer(TH_Dash, TLComp_Dash->GetTimelineLength(), false);
}

void ASteikemannCharacter::TL_Dash(float value)
{
	GetMoveComponent()->MaxWalkSpeed = Dash_WalkSpeed_Base + (Dash_WalkSpeed_Add * value);
	FRotator Rot = m_InputVector.Rotation();
	FVector NewVel = FVector::ForwardVector * GetMoveComponent()->Velocity.Length();
	GetMoveComponent()->Velocity = NewVel.RotateAngleAxis(Rot.Yaw, FVector::UpVector);
}

void ASteikemannCharacter::TL_Dash_End()
{
	TLComp_Dash->Stop();
	GetMoveComponent()->MaxWalkSpeed = Dash_WalkSpeed_Base;
	m_EGroundState = EGroundState::GROUND_Walk;
}

bool ASteikemannCharacter::Can_Dash_Start() const
{
	switch (m_EGroundState)
	{
	case EGroundState::GROUND_Walk:		return true;
	case EGroundState::GROUND_Roll:		return false;
	case EGroundState::GROUND_Dash:		
		if (TimerManager.GetTimerElapsed(TH_Dash) / TLComp_Dash->GetTimelineLength() > Dash_StartAgainPercentage)
			return true;
		break;
	default:
		break;
	}
	return false;
}

bool ASteikemannCharacter::ShroomBounce(FVector direction, float strength)
{
	if (!Super::ShroomBounce(direction, strength)) 
		return false;

	TLComp_AirFriction->PlayFromStart();
	JumpCurrentCount = 1;
	return true;
}

void ASteikemannCharacter::Click_RightFacebutton()
{
	if (m_PromptState == EPromptState::InPrompt)
		ExitPrompt();
	
	switch (m_EState)
	{
	case EState::STATE_None:
		break;
	case EState::STATE_OnGround:
		if (Can_Dash_Start())
			Dash_Start();
		break;
	case EState::STATE_InAir:
		break;
	case EState::STATE_OnWall:
		CancelOnWall();
		break;
	case EState::STATE_Attacking: 
		return;
	case EState::STATE_Grappling:
	{
		if (m_EGrappleState == EGrappleState::Pre_Launch)
		{
			if (m_EGrappleType == EGrappleType::Static_StuckEnemy_Ground)
				PullDynamicTargetOffWall();
			if (m_EGrappleType == EGrappleType::Dynamic_Ground)
				GH_Cancel();
		}
		return;
	}
	default:
		break;
	}

	if (bPressedCancelButton) { return; }

	bPressedCancelButton = true;
}

void ASteikemannCharacter::UnClick_RightFacebutton()
{
	bPressedCancelButton = false;
}

void ASteikemannCharacter::ReceiveCollectible(ECollectibleType type)
{
	switch (type)
	{
	case ECollectibleType::Common:
		CollectibleCommon++;
		UpdateSapCollectible();
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
	UpdateHealthWidget();

	// Hair Material Change
	HealthHairColor(Health);
}

void ASteikemannCharacter::PTakeDamage(int damage, AActor* otheractor, int i/* = 0*/)
{
	if (bIsDead || !bPlayerCanTakeDamage) { return; }

	bPlayerCanTakeDamage = false;
	Health = FMath::Clamp(Health -= damage, 0, MaxHealth);
	if (Health == 0) {
		Death();
	}

	/* Launch player */
	FVector direction = (GetActorLocation() - otheractor->GetActorLocation()).GetSafeNormal();
	direction = (direction + FVector::UpVector).GetSafeNormal();

	auto ITag = Cast<IGameplayTagAssetInterface>(otheractor);
	if (ITag)
	{
		if (ITag->HasMatchingGameplayTag(Tag::AubergineDoggo()))
			SetActorRotation(FRotator(0, (-direction).Rotation().Yaw, 0));
	}

	TimerManager.SetTimer(THDamageBuffer, [this](){
			bPlayerCanTakeDamage = true; 
			PTakeRepeatDamage();
		}, 
		DamageInvincibilityTime, false);

	/* Damage launch */
	GetMoveComponent()->Velocity *= 0.f;
	GetMoveComponent()->AddImpulse(direction * SelfDamageLaunchStrength, true);

	// Animation
	Anim_TakeDamage();

	UpdateHealthWidget();

	// Taking damage Visual Effects
	TakeDamage_Impl();

	// Hair Material Change
	HealthHairColor(Health);
}

void ASteikemannCharacter::PTakeDamage(int damage, const FVector& Direction, int i)
{
	if (bIsDead || !bPlayerCanTakeDamage) { return; }

	bPlayerCanTakeDamage = false;
	Health = FMath::Clamp(Health -= damage, 0, MaxHealth);
	if (Health == 0) { Death(); }

	m_EMovementInputState = EMovementInput::Locked;
	TimerManager.SetTimer(THDamageBuffer, [this]() { bPlayerCanTakeDamage = true; PTakeRepeatDamage(); }, DamageInvincibilityTime, false);
	FTimerHandle Move;
	TimerManager.SetTimer(Move, [this]() { m_EMovementInputState = EMovementInput::Open; }, DamageInvincibilityTime / 2.f, false);
	
	/* Damage launch */
	GetMoveComponent()->Velocity *= 0.f;
	GetMoveComponent()->AddImpulse(FVector(Direction + FVector::UpVector).GetSafeNormal() * SelfDamageLaunchStrength, true);
	SetActorRotation(FRotator(0, (-Direction).Rotation().Yaw, 0));

	// Animation
	Anim_TakeDamage();

	UpdateHealthWidget();

	// Taking damage Visual Effects
	TakeDamage_Impl();

	// Hair Material Change
	HealthHairColor(Health);
}

bool ASteikemannCharacter::PTakeRepeatDamage()
{
	TArray<FHitResult> Hits;
	FCollisionQueryParams Params("", false, this);
	double width = GetCapsuleComponent()->GetScaledCapsuleRadius();
	FCollisionShape box = FCollisionShape::MakeBox(FVector(width, width, 10.0));
	if (!GetWorld()->SweepMultiByChannel(Hits, GetActorLocation(), GetActorLocation() + (FVector::DownVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), FQuat(1.f, 0.f, 0.f, 0.f), ECC_WorldStatic, box, Params))
		return false;

	for (const auto& Hit : Hits)
	{
		IGameplayTagAssetInterface* itag = Cast<IGameplayTagAssetInterface>(Hit.GetActor());
		if (!itag) continue;

		if (itag->HasMatchingGameplayTag(Tag::EnvironmentHazard())) {
			PTakeDamage(1, Hit.GetActor());
			return true;
		}
	}
	return false;
}

void ASteikemannCharacter::Pickup_InkFlower()
{
	InkFlowerCollectible++;
	UpdateInkCollectible();
}

void ASteikemannCharacter::CheckForNewJournalEntry()
{
	if (InkFlowerCollectible >= 3) {
		GetJournalEntry();
		InkFlowerCollectible = 0;
	}
}

void ASteikemannCharacter::ShowHUD_Timed_Pure()
{
	if (ActionLocked()) return;
	ShowHUD_Timed();
}

void ASteikemannCharacter::Death()
{
	PRINTLONG(2.f, "POTITT IS DEAD");

	bIsDead = true;
	if (GetPlayerController())
		DisableInput(GetPlayerController());

	// Delegate called on land
	DeathDelegate_Land.BindLambda([this]() { DeathDelegate.Execute(); });
}

void ASteikemannCharacter::Death_Deathzone()
{
	//Camera->DetachFromParent(true);
	Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	Death();
	DeathDelegate.Execute();
}

void ASteikemannCharacter::Respawn()
{
	PRINTLONG(2.f, "RESPAWN PLAYER");

	/* Reset player */
	Health = MaxHealth;
	UpdateHealthWidget();
	HealthHairColor(Health);
	bIsDead = false;
	bPlayerCanTakeDamage = true;
	TimerManager.ClearTimer(THDamageBuffer);
	m_EMovementInputState = EMovementInput::Open;
	m_EGroundState = EGroundState::GROUND_Walk;
	EnableInput(GetPlayerController());
	GetMoveComponent()->Velocity *= 0;
	CancelAnimation();
	DeathDelegate_Land.Unbind();
	Camera->AttachToComponent(CameraBoom, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);

	if (Checkpoint) {
		FTransform T = Checkpoint->GetSpawnTransform();
		SetActorTransform(T, false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}
	SetActorTransform(StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
}

void ASteikemannCharacter::SetWallInputDirection()
{
	//float dot = FVector::DotProduct(m_InputVector, m_WallJumpData.Normal * -1.f);
	FVector right = FVector::CrossProduct(FVector::UpVector, m_WallJumpData.Normal * -1.f);
	float direction = FVector::DotProduct(m_InputVector, right);
	//if (direction < 0) dot *= -1.f;

	WallInputDirection = direction;
}

void ASteikemannCharacter::ExitOnWall(EState state)
{
	// TODO: Reevaluate m_State EState
	m_EState = state;
	m_WallState = EOnWallState::WALL_Leave;
	FTimerHandle h;
	TimerManager.SetTimer(h, [this]() { m_WallState = EOnWallState::WALL_None; }, 0.5f, false);
}

bool ASteikemannCharacter::IsOnWall() const
{
	return m_EState == EState::STATE_OnWall && m_WallState == EOnWallState::WALL_Drag;
}

bool ASteikemannCharacter::IsLedgeGrabbing() const
{
	return m_EState == EState::STATE_OnWall && m_WallState == EOnWallState::WALL_Ledgegrab;
}

bool ASteikemannCharacter::Anim_IsOnWall() const
{
	return m_EState == EState::STATE_OnWall && (m_WallState == EOnWallState::WALL_Hang || m_WallState == EOnWallState::WALL_Drag);
}

bool ASteikemannCharacter::Validate_Ledge(FHitResult& hit)
{
	FHitResult h;
	FCollisionQueryParams param("", false, this);
	const bool b1 = GetWorld()->LineTraceSingleByChannel(h, m_Ledgedata.ActorLocation, m_Ledgedata.ActorLocation + (FVector::DownVector * (GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.f)), ECC_PlayerWallDetection, param);
	if (b1)
		return false;
	const bool b2 = GetWorld()->LineTraceSingleByChannel(hit, m_Ledgedata.TraceLocation, m_Ledgedata.TraceLocation - (m_Walldata.Normal * 100.f), ECC_PlayerWallDetection, param);
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
	FVector location = m_Ledgedata.ActorLocation + LedgeGrab_ActorZOffset;

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

	// Canceling Animation
	CancelAnimation();
}

void ASteikemannCharacter::LedgeGrab()
{
}

bool ASteikemannCharacter::ValidateWall()
{
	FVector start = GetActorLocation() + FVector(0, 0, Wall_HeightCriteria);
	FHitResult hit;
	FCollisionQueryParams Params("", false, this);
	const bool b = GetWorld()->LineTraceSingleByChannel(hit, start, start + (m_WallJumpData.Normal * 200.f * -1.f), ECC_PlayerWallDetection, Params);
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
		SetWallInputDirection();
		OnWall_Drag_IMPL(deltatime, (GetMoveComponent()->Velocity.Z/GetMoveComponent()->WJ_DragSpeed) * -1.f);	// VelocityZ scale from 0->1
		break;
	case EOnWallState::WALL_Ledgegrab:
		//DrawDebugArms(0.f);
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
	TimerManager.SetTimer(TH_OnWall_Cancel, [this]() { m_WallState = EOnWallState::WALL_None; }, OnWall_CancelTimer, false);
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
		if (ACollectible* collectible = Cast<ACollectible>(OtherActor))
		{
			ReceiveCollectible(collectible->CollectibleType);
			collectible->Destruction();
		}
		else if (ACollectible_Static* collectible_static = Cast<ACollectible_Static>(OtherActor))
		{
			ReceiveCollectible(collectible_static->CollectibleType);
			collectible_static->Destruction();
			PRINTPARLONG(2.f, "Collected -> %s", *collectible_static->GetName());
		}
	}

	/* Add checkpoint, overrides previous checkpoint */
	if (tags.HasTag(Tag::PlayerRespawn())) {
		Checkpoint = Cast<APlayerRespawn>(OtherActor);
		if (!Checkpoint)
			UE_LOG(LogTemp, Warning, TEXT("PLAYER: Failed cast to Checkpoint: %s"), *OtherActor->GetName());
	}

	/* Player enters/falls into a DeathZone */
	if (tags.HasTag(Tag::DeathZone())) {
		Death_Deathzone();
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
		}
	}
}

void ASteikemannCharacter::OnCapsuleComponentHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	IGameplayTagAssetInterface* itag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!itag) { return; }

	/* Environmental Hazard collision */
	if (itag->HasMatchingGameplayTag(Tag::EnvironmentHazard())) {
		if (bPlayerCanTakeDamage)
			PTakeDamage(1, OtherActor);
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
		if (OtherComp->IsA(UBoxComponent::StaticClass())) return;

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

bool ASteikemannCharacter::IsSmackAttacking() const
{
	return m_EState == EState::STATE_Attacking && m_EAttackState == EAttackState::Smack;
}

void ASteikemannCharacter::Click_Attack()
{
	if (bAttackPress)   return; 
	bAttackPress = true;

	if (m_EState == EState::STATE_Attacking)
		BufferDelegate_Attack(&ASteikemannCharacter::ComboAttack_Pure);

	if (ActionLocked()) return;
	switch (m_EState)
	{
	case EState::STATE_OnGround:
	{
		AttackSmack_Start_Ground_Pure();
		TL_Dash_End();
		break;
	}
	case EState::STATE_InAir:
	{
		AttackSmack_Start_Ground_Pure();
		break;
	}
	case EState::STATE_OnWall:
	{
		break;
	}
	case EState::STATE_Attacking:
	{
		BufferDelegate_Attack(&ASteikemannCharacter::ComboAttack_Pure);
		break;
	}
	case EState::STATE_Grappling:
	{
		/* Buffer Attack if Grappling to Dynamic Target */
		if (m_EGrappleType == EGrappleType::Dynamic_Ground)
		{
			m_ESmackAttackType = ESmackAttackType::GrappleSmack;
			if (TimerManager.IsTimerActive(TH_BufferAttack)) return;

			GH_DelegateDynamicLaunch();
			float t = 
				((TimerManager.IsTimerActive(TH_Grapplehook_End_Launch) ? TimerManager.GetTimerRemaining(TH_Grapplehook_End_Launch) : GrappleHook_PostLaunchTimer) + 
				(TimerManager.IsTimerActive(TH_GrappleHold) ? TimerManager.GetTimerRemaining(TH_GrappleHold) : 0.f))
				- SmackAttack_GH_TimerRemoval;
			if (t > 0.f) {
				TimerManager.SetTimer(TH_BufferAttack, this, &ASteikemannCharacter::AttackSmack_Grapple_Pure, t);
				return;
			}
			AttackSmack_Grapple_Pure();
			return;
		}
		break;
	}
	default:
		break;
	}
	m_ESmackAttackType = ESmackAttackType::Regular;
	m_EAttackType = EAttackType::SmackAttack;
	return;
}

void ASteikemannCharacter::UnClick_Attack()
{
	bAttackPress = false;
}

void ASteikemannCharacter::AttackSmack_Start_Pure()
{
	m_EState = EState::STATE_Attacking;
	m_EAttackState = EAttackState::Smack;
	m_EMovementInputState = EMovementInput::Locked;
	AttackSmack_Start();
}

void ASteikemannCharacter::AttackSmack_Start_Ground_Pure()
{
	AttackSmack_Start_Pure();
	RotateToAttack();

	if (!IsFalling())
		TLComp_Attack_SMACK->PlayFromStart();
}

void ASteikemannCharacter::AttackSmack_Grapple_Pure()
{
	TimerManager.ClearTimer(TH_Grapplehook_End_Launch);
	GH_Stop(EState::STATE_Attacking);
	Delegate_PostAttackBuffer.BindUObject(this, &ASteikemannCharacter::PostAttack_GrappleSmack);

	AttackSmack_Start_Pure();
}

void ASteikemannCharacter::ComboAttack_Pure()
{
	m_EState = EState::STATE_Attacking;
	m_EAttackState = EAttackState::Smack;
	m_EMovementInputState = EMovementInput::Locked;
	Deactivate_AttackCollider();
	int combo{};
	(AttackComboCount++ % 2 == 0) ? combo = 2 : combo = 1;
	ComboAttack(combo);
	if (!IsFalling())
		TLComp_Attack_SMACK->PlayFromStart();
}

void ASteikemannCharacter::Cancel_SmackAttack()
{
	EndAttackBufferPeriod();
	Deactivate_AttackCollider();
	Stop_Attack();

	AttackComboCount = 0;
	AttackContactedActors.Empty();
	m_EAttackState = EAttackState::None;
	m_EAttackType = EAttackType::None;
	m_ESmackAttackType = ESmackAttackType::Regular;
	m_EMovementInputState = EMovementInput::Open;
	ResetState();
}

void ASteikemannCharacter::Stop_Attack()
{
	PRINTLONG(2.f, "STOP ATTACK");
	AttackComboCount = 0;
	AttackContactedActors.Empty();
	m_EAttackState = EAttackState::None;
	m_EAttackType = EAttackType::None;
	m_ESmackAttackType = ESmackAttackType::Regular;
	m_EState = EState::STATE_None;
	m_EMovementInputState = EMovementInput::Open;

	//EndAttackBufferPeriod();// testing

	SetDefaultState();
}

void ASteikemannCharacter::StartAttackBufferPeriod()
{
}

void ASteikemannCharacter::ExecuteAttackBuffer()
{
	Delegate_AttackBuffer.ExecuteIfBound();
	Delegate_AttackBuffer.Unbind();
	m_EAttackState = EAttackState::Post_Buffer;
}

void ASteikemannCharacter::EndAttackBufferPeriod()
{
	Delegate_PostAttackBuffer.Unbind();
	m_EAttackState = EAttackState::None;
}

void ASteikemannCharacter::PostAttack_GrappleSmack(EPostAttackType& type)
{
	type = EPostAttackType::GrappleSmack;
}

void ASteikemannCharacter::BufferDelegate_Attack(void(ASteikemannCharacter::* func)())
{
	if (m_EAttackState == EAttackState::Post_Buffer) {
		std::invoke(func, this);
	}
	else {
		Delegate_AttackBuffer.BindUObject(this, func);
	}
}

void ASteikemannCharacter::AttackContact(AActor* target)
{
	Super::AttackContact(target);
	// Particles
	AttackContact_Particles(AttackCollider->GetComponentLocation(), AttackCollider->GetComponentQuat());
}

void ASteikemannCharacter::AttackContact_Particles(FVector location, FQuat direction)
{
	NiagaraComp_Attack->SetWorldLocationAndRotation(location, direction, false, nullptr, ETeleportType::TeleportPhysics);

	NiagaraComp_Attack->SetAsset(NS_AttackContact);
	NiagaraComp_Attack->Activate(true);
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
	AttackContactedActors.Empty();

	AttackCollider->SetHiddenInGame(true);	// For Debugging
	AttackCollider->SetGenerateOverlapEvents(false);
	AttackCollider->SetRelativeScale3D(FVector(0, 0, 0));
}


void ASteikemannCharacter::RotateToAttack()
{
	AttackDirection = m_InputVector;
	if (m_InputVector.IsNearlyZero())
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
		
		EAttackType AType = EAttackType::None;
		if (OverlappedComp == AttackCollider) { AType = EAttackType::SmackAttack; }
		if (OverlappedComp == GroundPoundCollider) { AType = EAttackType::GroundPound; }

		//AttackContactDelegate_Instigator.Broadcast();
		AttackContactDelegate.Broadcast(OtherActor);
		
		/* Attacking a corruption core || Enemy Spawner || InkFlower*/
		if (TCon.HasTag(Tag::CorruptionCore()) || 
			TCon.HasTag(Tag::InkFlower()) || 
			TCon.HasTag(Tag::EnemySpawner()))
		{
			Gen_Attack(IAttack, OtherActor, AType);
		}


		/* Smack attack collider */
		if (OverlappedComp == AttackCollider && OtherComp->GetClass() == UCapsuleComponent::StaticClass())
		{
			switch (m_EAttackState)
			{
			case EAttackState::None:
				break;
			case EAttackState::Smack:
				Do_SmackAttack_Pure(IAttack, OtherActor);
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

void ASteikemannCharacter::Gen_Attack(IAttackInterface* OtherInterface, AActor* OtherActor, const EAttackType AType)
{
	FVector Direction_ActorToActor{ OtherActor->GetActorLocation() - GetActorLocation() };
	FVector Direction_ComponentToActor = FVector(OtherActor->GetActorLocation() - GetMesh()->GetSocketLocation("Stav_CollisionSocket"));
	//FVector Direction = FVector(Direction_ActorToActor.GetSafeNormal() + Direction_ComponentToActor.GetSafeNormal()).GetSafeNormal();
	FVector Direction = Direction_ComponentToActor.GetSafeNormal();
	OtherInterface->Gen_ReceiveAttack(Direction, SmackAttackStrength, AType);
	DRAWLINE(Direction * 300.f, FColor::Red, 2.f);
}

void ASteikemannCharacter::TlCurve_AttackTurn_IMPL(float value)
{
	FVector proj = m_InputVector.ProjectOnTo(GetActorRightVector());
	FVector Dir = FVector(GetActorForwardVector() + (proj * value)).GetSafeNormal2D();
	SetActorRotation(Dir.Rotation(), ETeleportType::TeleportPhysics);
}

void ASteikemannCharacter::TlCurve_AttackMovement_IMPL(float value)
{
	float dot = FMath::Clamp(FVector::DotProduct(GetActorForwardVector(), m_InputVector), 0.f, 1.f);
	float strength = value * dot * m_InputVector.Length() * GetWorld()->GetDeltaSeconds();
	AddActorWorldOffset(GetActorForwardVector() * strength, false, nullptr, ETeleportType::None);
}

void ASteikemannCharacter::TlCurve_AttackTurn(float value)
{
	TlCurve_AttackTurn_IMPL(value);
}

void ASteikemannCharacter::TlCurve_AttackMovement(float value)
{
	TlCurve_AttackMovement_IMPL(value);
}

void ASteikemannCharacter::Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	// Burde sjekke om den kan bli angrepet i det hele tatt. 
	const bool b{ OtherInterface->GetCanBeSmackAttacked() };
	TLComp_Attack_SMACK->Stop();

	if (b)
	{
		// GrappleSmack
		if (m_ESmackAttackType == ESmackAttackType::GrappleSmack)
		{
			FVector Direction = GH_GrappleSmackAiming_MNK(OtherActor);
			OtherInterface->Receive_SmackAttack_Pure(Direction, GrappleSmack_Strength + (GrappleSmack_Strength * Grapplesmack_DirectionMultiplier));
			return;
		}

		// Regular Smack
		FVector Direction = (FVector::UpVector * SmackUpwardAngle) + (GetActorForwardVector().GetSafeNormal2D() * (1.f - FMath::Abs(SmackUpwardAngle)));
		OtherInterface->Receive_SmackAttack_Pure(Direction, SmackAttackStrength);
		return;
	}
}

void ASteikemannCharacter::Receive_SmackAttack_Pure(const FVector Direction, const float Strength, const bool bOverrideStrength)
{
	PTakeDamage(1, Direction);
}

void ASteikemannCharacter::Click_GroundPound()
{
	if (ActionLocked()) return;
	if (!bGroundPoundPress)
	{
		bGroundPoundPress = true;
		Do_GroundPound();
	}
}

void ASteikemannCharacter::UnClick_GroundPound()
{
	bGroundPoundPress = false;
}


void ASteikemannCharacter::Do_GroundPound()
{
	if (IsGroundPounding() || IsOnGround() || m_EState == EState::STATE_OnGround) return;

	bPB_Groundpound_PredeterminedPogoHit = PB_Groundpound_Predeterminehit();
	m_WallState = EOnWallState::WALL_Leave;
	m_EAttackType = EAttackType::GroundPound;

	ExitOnWall(EState::STATE_Attacking);
	GetMoveComponent()->CancelOnWall();

	Start_GroundPound();
}

void ASteikemannCharacter::Launch_GroundPound()
{
	float strength = GP_VisualLaunchStrength;
	if (bPB_Groundpound_LaunchNextFrame && PB_Groundpound_TargetActor) {
		PB_Groundpound_IMPL(PB_Groundpound_TargetActor);
		strength = 0.f;
	}
	GetMoveComponent()->GP_Launch(strength);
}

void ASteikemannCharacter::Start_GroundPound()
{
	m_EState = EState::STATE_Attacking;
	m_EAttackState = EAttackState::GroundPound;

	GetMoveComponent()->GP_PreLaunch();

	TimerManager.SetTimer(THandle_GPHangTime, this, &ASteikemannCharacter::Launch_GroundPound, GP_PrePoundAirtime);

	// Animation
	Anim_GroundPound_Initial();
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

	TimerManager.SetTimer(THandle_GPReset, this, &ASteikemannCharacter::Deactivate_GroundPound, GroundPoundExpandTime);

	// Locking movement for a period, GP_MovementPeriodLocket	// Setting AttackState as Post Groundpound, read in BreakMovementInput for animation montage canceling
	m_EAttackState = EAttackState::Post_GroundPound;
	LockMovementForPeriod(GP_MovementPeriodLocked, nullptr);

	// Animation
	Anim_GroundPound_Land_Ground();
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

#ifdef UE_BUILD_DEBUG
void ASteikemannCharacter::Print_State()
{
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
}
void ASteikemannCharacter::Print_State(float time)
{
	switch (m_EState)
	{
	case EState::STATE_OnGround:
		PRINTLONG(time, "STATE_OnGround");
		break;
	case EState::STATE_InAir:
		PRINTLONG(time, "STATE_InAir");
		break;
	case EState::STATE_OnWall:
		PRINTLONG(time, "STATE_OnWall");
		break;
	case EState::STATE_Attacking:
		PRINTLONG(time, "STATE_Attacking");
		break;
	case EState::STATE_Grappling:
		PRINTLONG(time, "STATE_Grappling");
		break;
	default:
		break;
	}
}
#endif


