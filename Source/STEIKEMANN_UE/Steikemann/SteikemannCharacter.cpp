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


}

// Called when the game starts or when spawned
void ASteikemannCharacter::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = Cast<USteikemannCharMovementComponent>(GetCharacterMovement());

	PlayerController = Cast<APlayerController>(GetController());

	Base_CameraBoomLength = CameraBoom->TargetArmLength;

	/* Creating Niagara Compnents */
	{
		NiComp_CrouchSlide = CreateNiagaraComponent("Niagara_CrouchSlide", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
		if (NiComp_CrouchSlide) {
			NiComp_CrouchSlide->bAutoActivate = false;
			if (NS_CrouchSlide) { NiComp_CrouchSlide->SetAsset(NS_CrouchSlide); }
		}

	}
	
	/* Attack Collider */
	{
		AttackCollider->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnAttackColliderBeginOverlap);
		AttackColliderScale = AttackCollider->GetRelativeScale3D();
		AttackCollider->SetRelativeScale3D(FVector(0));
		

		GroundPoundCollider->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnAttackColliderBeginOverlap);
		//GroundPoundColliderScale = GroundPoundCollider->GetRelativeScale3D();

	}

	/* Grapple Targeting Detection Sphere */
	{
		GrappleTargetingDetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASteikemannCharacter::OnGrappleTargetDetectionBeginOverlap);
		GrappleTargetingDetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ASteikemannCharacter::OnGrappleTargetDetectionEndOverlap);

		GrappleTargetingDetectionSphere->SetGenerateOverlapEvents(true);
		GrappleTargetingDetectionSphere->SetSphereRadius(GrappleHookRange);
	}

	/* WHY DO I HAVE TO DO THIS??? WITH REQUEST TAG IN THE GETTAG FUNCTIONS AS WELL?? */
	//Tag_Player = FGameplayTag::RequestGameplayTag("Pottit");
	//Tag_Enemy = FGameplayTag::RequestGameplayTag("Enemy");
	//Tag_EnemyAubergineDoggo = FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo");
	//Tag_GrappleTarget = FGameplayTag::RequestGameplayTag("GrappleTarget");
	//Tag_GrappleTarget_Static = FGameplayTag::RequestGameplayTag("GrappleTarget.Static");
	//Tag_GrappleTarget_Dynamic = FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic");
	//Tag_CameraVolume = FGameplayTag::RequestGameplayTag("CameraVolume");

	/*
	* Adding GameplayTags to the GameplayTagsContainer
	*/
	GameplayTags.AddTag(TAG_Player());
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
	if (bIsGroundPounding) {
		GroundPoundLand(Hit);
	}

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
	NiagaraPlayer->SetNiagaraVariableFloat("User.Velocity", bIsGroundPounding ? Velocity * 3.f * NSM_Land_ParticleSpeed : Velocity * NSM_Land_ParticleSpeed);	// Change pending on character is groundpounding or not
	NiagaraPlayer->SetWorldLocationAndRotation(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	NiagaraPlayer->Activate(true);
}


// Called every frame
void ASteikemannCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/* Rotate Inputvector to match the playercontroller */
	{
		InputVector = InputVectorRaw;
		FRotator Rot = GetControlRotation();
		InputVector = InputVector.RotateAngleAxis(Rot.Yaw, FVector::UpVector);

		if (InputVectorRaw.Size() > 1.f || InputVector.Size() > 1.f)
		{
			InputVector.Normalize();
		}
	}

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

	/* Resetting Wall Jump & Ledge Grab when player is on the ground */
	if (GetMoveComponent()->IsWalking()) {
		WallJump_NonStickTimer = 0.f;
	}



	/*		Wall Jump & LedgeGrab		*/
	if (GetMoveComponent()->IsFalling() && bOnWallActive)
	{
		Do_OnWallMechanics(DeltaTime);
	}
	else if (!bOnWallActive)
	{
		if (WallJump_NonStickTimer < WallJump_MaxNonStickTimer)	
		{
			WallJump_NonStickTimer += DeltaTime;

			ResetWallJumpAndLedgeGrab();
		}
		else {
			bOnWallActive = true;
		}
	}


	/* --------- GRAPPLE TARGETING -------- */
	GetGrappleTarget();
	
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
	if ((GetMoveComponent()->IsFalling() || IsJumping()) && GetMoveComponent()->Velocity.Z < 0.f)
	{
		FHitResult Hit{};
		FCollisionQueryParams Params{ "", false, this };
		const bool b = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + FVector::DownVector * 500.f, ECC_PogoCollision, Params);
		if (b)
		{
			CheckIfEnemyBeneath(Hit);
		}
	}

	GuideCamera(DeltaTime);

	/* ----------------------- COMBAT TICKS ------------------------------ */
	PreBasicAttackMoveCharacter(DeltaTime);
	SmackAttackMoveCharacter(DeltaTime);
	ScoopAttackMoveCharacter(DeltaTime);
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


void ASteikemannCharacter::GetGrappleTarget()
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
		if (GrappleInterface && !IsGrappling())
		{
			GrappleInterface->TargetedPure();
		}

		GrappledActor = Target;
	}
	else
	{
		IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
		if (GrappleInterface && !IsGrappling())
		{
			GrappleInterface->TargetedPure();
		}
		return;
	}
}

void ASteikemannCharacter::RightTriggerClick()
{
	if (IsGrappling() || IsPostGrapple()) { return; }

	if (GrappledActor.IsValid() && !Active_GrappledActor.IsValid())
	{
		IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(GrappledActor.Get());
		IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
		if (TagInterface && GrappleInterface)
		{
			Active_GrappledActor = GrappledActor;

			/* Checking tags to determine if the grapple target is dynamic or static */
			FGameplayTagContainer GrappledTags;
			TagInterface->GetOwnedGameplayTags(GrappledTags);

			/* Grappling to Enemy/Dynamic Target */
			//if (GrappledTags.HasTag(Tag_GrappleTarget_Dynamic))	// Burde bruke denne taggen på fiendene
			if (GrappledTags.HasTag(TAG_AubergineDoggo()))		// Også spesifisere videre med fiende type
			{
				bGrapplingDynamicTarget = true;
			}
			/* Grappling to Static Target */
			else if (GrappledTags.HasTag(TAG_GrappleTarget_Static()))
			{
				bGrapplingStaticTarget = true;
			}

			Initial_GrappleHook();
		}
	}
}

void ASteikemannCharacter::RightTriggerUn_Click()
{
	// Noe angående slapp grapple knappen
}

void ASteikemannCharacter::Initial_GrappleHook()
{
	if (!Active_GrappledActor.IsValid()) { return; }

	/* Start GrappleHook */
	bIsGrapplehooking = true;
	GetMoveComponent()->bGrappleHook_InitialState = true;

	/* Rotate actor */
	FVector Direction = Active_GrappledActor->GetActorLocation() - GetActorLocation();
	RotateActorYawToVector(Direction.GetSafeNormal());

	/* Call interface HookedPure
	* Dynamic - Rotate actor towards player, play animation
	* Static - Change material */
	IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(Active_GrappledActor.Get());
	if (GrappleInterface)
	{
		GrappleInterface->HookedPure();
		GrappleInterface->HookedPure(GetActorLocation(), true);	// To Rotate dynamic targets towards player 
	}

	/* Pre Launch/Grapple Timer before the GrappleHook is called */
	GetWorldTimerManager().SetTimer(TH_Grapplehook_Start, this, &ASteikemannCharacter::Start_GrappleHook, GrappleDrag_PreLaunch_Timer_Length);
}

void ASteikemannCharacter::Start_GrappleHook()
{
	FGameplayTagContainer GrappledTags;
	IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Active_GrappledActor.Get());
	IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(Active_GrappledActor.Get());
	if (!TagInterface || !GrappleInterface) { return; }


	TagInterface->GetOwnedGameplayTags(GrappledTags);

	/* Grappling to Enemy/Dynamic Target */
	//if (GrappledTags.HasTag(Tag_GrappleTarget_Dynamic))	// Burde bruke denne taggen på fiendene
	if (GrappledTags.HasTag(TAG_AubergineDoggo()))		// Også spesifisere videre med fiende type
	{
		GrappleInterface->HookedPure(GetActorLocation());
	}
	/* Grappling to Static Target */
	else if (GrappledTags.HasTag(TAG_GrappleTarget_Static()))
	{
		Launch_GrappleHook();
		/* Reset double jump */
		JumpCurrentCount = 1;
	}

	GetMoveComponent()->bGrappleHook_InitialState = false;
	
	bIsGrapplehooking = false;
	bIsPostGrapplehooking = true;
	//Active_GrappledActor = nullptr;

	/* Set End Timer - Shared between Static and Dynamic GrappleTarget */
	GetWorldTimerManager().SetTimer(TH_Grapplehook_End_Launch, this, &ASteikemannCharacter::Stop_GrappleHook, GrappleHook_PostLaunchTimer);
}

void ASteikemannCharacter::Launch_GrappleHook()
{
	GetMoveComponent()->DeactivateJumpMechanics();

	/* Grapple Launch */
	FVector LaunchDirection = Active_GrappledActor->GetActorLocation() - GetActorLocation();

	float length = LaunchDirection.Size();

	float LaunchStrength{};
	/* Set launch strength based on length and threshhold */
	length < GrappleHook_Threshhold ? 
		LaunchStrength = GrappleHook_LaunchSpeed : 
		LaunchStrength = GrappleHook_LaunchSpeed + (GrappleHook_LaunchSpeed * ((length - GrappleHook_Threshhold) / (GrappleHookRange - GrappleHook_Threshhold)) / GrappleHook_DividingFactor);
	//length > 500.f ? PRINTPARLONG("LaunchStrength: %f", 2000.f + 2000.f * ((length - 500.f) / (GrappleHookRange - 500.f))) : PRINTPARLONG("LaunchStrength: %f", 2000.f);

	//PRINTPARLONG("LaunchStrength: %f", LaunchStrength);
	//PRINTPARLONG("Length: %f", length);
	//PRINTPARLONG("Range: %f", GrappleHookRange);

	FVector Direction = LaunchDirection.GetSafeNormal() + FVector::UpVector;
	Direction.Normalize();

	GetMoveComponent()->AddImpulse(Direction * LaunchStrength, true);
}

void ASteikemannCharacter::Stop_GrappleHook()
{
	bIsPostGrapplehooking = false;
	Active_GrappledActor = nullptr;
	bGrapplingStaticTarget = false;
	bGrapplingDynamicTarget = false;
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
	/* Lerp CameraBoom TargetArmLength back to it's original length, after the changes caused by CAMERA_Absolute */
	if (bCamLerpBackToPosition) {
		float Current = CameraBoom->TargetArmLength;
		float L = FMath::FInterpTo(Current, Base_CameraBoomLength, DeltaTime, 2.f);
		CameraBoom->TargetArmLength = L;

		if (CameraBoom->TargetArmLength == Base_CameraBoomLength) {
			bCamLerpBackToPosition = false;
		}
	}


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

	auto LA_SLerpToQuat = [&](FQuat& Target, float alpha, APlayerController* Con) {
		FQuat Rot{ Con->GetControlRotation() };
		FQuat New{ FQuat::Slerp(Rot, Target, alpha) };
		FRotator Rot1 = New.Rotator();
		Rot1.Roll = 0.f;
		Con->SetControlRotation(Rot1);
	};

	auto LA_Absolute = [&](FocusPoint& P, float& alpha, APlayerController* Con) {
		alpha >= 1.f ? alpha = 1.f : alpha += DeltaTime * P.LerpSpeed;
		FQuat VToP{ ((P.Location - FVector(0, 0, PitchAdjust(CameraGuide_Pitch, CameraGuide_Pitch_MIN, CameraGuide_Pitch_MAX)/*Pitch Adjustment*/)) - CameraBoom->GetComponentLocation()).Rotation() };	// Juster pitch adjustment basert på z til VToP uten adjustment

		LA_SLerpToQuat(VToP, alpha, Con);
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

		LA_SLerpToQuat(VToP_quat, alpha, Con);
	};

	auto CAM_LerpToPosition = [&](FQuat& Target, float& alpha, float& length, float lerpSpeed, APlayerController* Con) {
		LA_SLerpToQuat(Target, alpha, Con);

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

		LA_SLerpToQuat(VtoP_quat, alpha, Con);
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


void ASteikemannCharacter::MoveForward(float value)
{
	InputVectorRaw.X = value;

	if (IsStickingToWall()) { return; }
	if (IsLedgeGrabbing()) { return; }
	if (IsCrouchSliding()) { return; }
	if (IsAttacking()) { return; }
	if (IsGrappling()) { return; }

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

	if (IsStickingToWall()) { return; }
	if (IsLedgeGrabbing())	{ return; }
	if (IsCrouchSliding()) { return; }
	if (IsAttacking()) { return; }
	if (IsGrappling()) { return; }

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
	OnLanded(Hit);

	LandedDelegate.Broadcast(Hit);
}

void ASteikemannCharacter::Jump()
{
	/* Don't Jump if player is Grappling */
	if (IsGrappling() && GetMoveComponent()->IsFalling()) { return; }
	if (bPostWallJump) { return; }

	if (!bJumpClick)
	{
		bJumpClick = true;
		bJumping = true;

		/* ------ VARIOUS TYPES OF JUMPS ------- */

		/* ---- SLIDE JUMP ---- */
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

		/* ---- LEDGE JUMP ---- */
		//if ((GetMoveComponent()->bLedgeGrab) && GetMoveComponent()->IsFalling())
		if (IsLedgeGrabbing() && GetMoveComponent()->IsFalling())
		{
			JumpCurrentCount = 1;

			ResetWallJumpAndLedgeGrab();
			bOnWallActive = false;
			WallJump_NonStickTimer = 0.f;

			/*bAddJumpVelocity = */GetMoveComponent()->LedgeJump(LedgeLocation, JumpStrength);
			Anim_Activate_Jump();	//	Anim_Activate_LedgeGrabJump
			return;
		}

		/* ---- WALL JUMP ---- */
		if ((GetMoveComponent()->bStickingToWall || bFoundStickableWall) && GetMoveComponent()->IsFalling() || 
			(GetMoveComponent()->IsFalling() && Jump_DetectWall()))
		{
			GetMoveComponent()->WallJump(Wall_Normal, JumpStrength);
			Anim_Activate_Jump();	//	Anim_Activate_WallJump
			return;
		}

		/* If player walked off an edge with no jumpcount */
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
			return;
		}


		/* ---- DOUBLE JUMP ---- */
		if (CanDoubleJump())
		{
			JumpCurrentCount++;
			//GetMoveComponent()->Jump(JumpStrength);
			GetMoveComponent()->DoubleJump(InputVector.GetSafeNormal(), JumpStrength * DoubleJump_MultiplicationFactor);
			GetMoveComponent()->StartJumpHeightHold();
			Anim_Activate_Jump();	// Anim DoubleJump
			return;
		}

		/* ---- REGULAR JUMP ---- */
		if (CanJump())
		{
			JumpCurrentCount++;
			GetMoveComponent()->Jump(JumpStrength);
			Anim_Activate_Jump();
			return;
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
			if ((GetMoveComponent()->bLedgeGrab) && GetMoveComponent()->IsFalling())
			{
				JumpCurrentCount = 1;
				/*bAddJumpVelocity = */GetMoveComponent()->LedgeJump(LedgeLocation, JumpStrength);
				ResetWallJumpAndLedgeGrab();
				Anim_Activate_Jump();	//	Anim_Activate_LedgeGrabJump
				return;
			}

			/* If player is sticking to a wall */
			if ((GetMoveComponent()->bStickingToWall || bFoundStickableWall ) && GetMoveComponent()->IsFalling())
			{	
				if (JumpCurrentCount == 2)
				{
					//JumpCurrentCount++;
				}
				Anim_Activate_Jump();	//	Anim_Activate_WallJump
				return;
			}


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
	return  GetMoveComponent()->MovementMode == MOVE_Falling  && ( !IsGrappling()/* && !IsDashing()*/ && !IsStickingToWall() && !IsOnWall() && !IsJumping() );
}

bool ASteikemannCharacter::IsOnGround() const
{
	if (!GetMoveComponent().IsValid()) { return false; }
	return GetMoveComponent()->MovementMode == MOVE_Walking;
}

void ASteikemannCharacter::CheckIfEnemyBeneath(const FHitResult& Hit)
{
	IGameplayTagAssetInterface* Interface = Cast<IGameplayTagAssetInterface>(Hit.GetActor());

	if (Interface)
	{
		FGameplayTagContainer Container;
		Interface->GetOwnedGameplayTags(Container);
		
		if (Container.HasTagExact(TAG_AubergineDoggo()))
		{
			if (CheckDistanceToEnemy(Hit))
			{
				PogoBounce(Hit.GetActor()->GetActorLocation());
			}
		}
	}
}

bool ASteikemannCharacter::CheckDistanceToEnemy(const FHitResult& Hit)
{
	float LengthToEnemy = GetActorLocation().Z - Hit.GetActor()->GetActorLocation().Z;

	float Difference = (GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) + (PogoContingency);

	/* If Player is close to the enemy */
	if (LengthToEnemy - Difference < 0.f)
	{
		return true;
	}

	return false;
}

void ASteikemannCharacter::PogoBounce(const FVector& EnemyLocation)
{
	/* Pogo bounce 
	* The direction of the bounce should be based on: 
	*	Enemy location
	*	Input Direction
	*/
	JumpCurrentCount = 1;	// Resets double jump
	GetMoveComponent()->Velocity.Z = 0.f;

	FVector Direction = FVector::UpVector + (InputVector * PogoInputDirectionMultiplier); Direction.Normalize();
	GetMoveComponent()->AddImpulse(Direction * PogoBounceStrength, true);	// Simple method of bouncing player atm
	Anim_Activate_Jump();
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


bool ASteikemannCharacter::DetectLedge(FVector& Out_IntendedPlayerLedgeLocation, const FHitResult& In_WallHit, FHitResult& Out_Ledge, float Vertical_GrabLength, float Horizontal_GrabLength)
{
	FCollisionQueryParams Params(FName(""), false, this);

	/* Finds the orthogonal vector from the players forward axis(which matches wall.normal * -1.f) towards the UpVector */
	FVector OrthoPlayerToUp{ FVector::CrossProduct(In_WallHit.Normal * -1.f, FVector::CrossProduct(FVector::UpVector, In_WallHit.Normal * -1.f)) };

	/* Common locations */
	FVector LocationFromWallHit	{ In_WallHit.ImpactPoint + (In_WallHit.Normal * (GetCapsuleComponent()->GetUnscaledCapsuleRadius() + 5.f))};	// What would be the players location from the wall itself

	FVector FirstTracePoint_Start{ LocationFromWallHit + (OrthoPlayerToUp * Vertical_GrabLength) };
	FVector FirstTracePoint_End	 { LocationFromWallHit + (OrthoPlayerToUp * Vertical_GrabLength) + (In_WallHit.Normal * -1.f * Horizontal_GrabLength) };

	/* Do a raytrace above the In_WallHit to check if there is a ledge nearby */
	FHitResult FirstHit{};
	const bool bFirstCheck = GetWorld()->LineTraceSingleByChannel(FirstHit,
		FirstTracePoint_Start,
		FirstTracePoint_End,	
		ECC_Visibility, Params);


	/* Potential ledge above */
	if (!bFirstCheck)
	{
		float ContingencyLength{ 10.f };	// Trace a little inward of the edge just to make sure that the linetrace hits
		FVector SecondTracePoint_Start { FirstTracePoint_Start + (In_WallHit.Normal * -1.f * (GetCapsuleComponent()->GetUnscaledCapsuleRadius() + ContingencyLength)) };
		FVector SecondTracePoint_End   { SecondTracePoint_Start + ((OrthoPlayerToUp * -1.f) * Horizontal_GrabLength) };
		
		FHitResult SecondHit{};
		const bool bSecondHit = GetWorld()->LineTraceSingleByChannel(SecondHit,
			SecondTracePoint_Start,
			SecondTracePoint_End,
			ECC_Visibility, Params);


		/* Found Ledge */
		if (bSecondHit)
		{
			Out_Ledge = SecondHit;	// Returning ledge hitresult

			LengthToLedge = FVector(SecondTracePoint_Start - SecondHit.ImpactPoint).Size();

			/* Find the players intended position in relation to the ledge */
			Out_IntendedPlayerLedgeLocation =
				SecondHit.ImpactPoint
				+ (In_WallHit.Normal * GetCapsuleComponent()->GetUnscaledCapsuleRadius());			// From Wall
			Out_IntendedPlayerLedgeLocation.Z = SecondHit.ImpactPoint.Z - LedgeGrab_HoldLength;		// Downward

			return true;
		}
	}

	return false;
}

void ASteikemannCharacter::MoveActorToLedge(float DeltaTime)
{
	/*		Move actor toward PlayersLedgeLocation	  */
	FVector Position{};
	if (FVector(GetActorLocation() - PlayersLedgeLocation).Size() > 50.f)
	{
		Position = FMath::Lerp(GetActorLocation(), PlayersLedgeLocation, PositionLerpAlpha);
	}
	else 
	{
		Position = PlayersLedgeLocation;
	}
	SetActorLocation(Position, false, nullptr, ETeleportType::TeleportPhysics);
}

void ASteikemannCharacter::DrawDebugArms(const float& InputAngle)
{
	FVector Ledge{ LedgeHit.ImpactPoint };
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


void ASteikemannCharacter::ResetWallJumpAndLedgeGrab()
{
	GetMoveComponent()->bWallSlowDown	= false;
	GetMoveComponent()->bStickingToWall	= false;
	bFoundStickableWall					= false;
	InputAngleToForward					= 0.f;
	InputDotProdToForward				= 1.f;

	bIsLedgeGrabbing					= false;
	bFoundLedge							= false;
	GetMoveComponent()->bLedgeGrab		= false;
	GetMoveComponent()->bLedgeJump		= false;
}

void ASteikemannCharacter::ResetActorRotationPitchAndRoll(float DeltaTime)
{
	FRotator Rot = GetActorRotation();
	AddActorLocalRotation(FRotator(Rot.Pitch * -1.f, 0.f, Rot.Roll * -1.f));
}

void ASteikemannCharacter::RotateActorYawToVector(FVector AimVector, float DeltaTime /*=0*/)
{
	FVector Aim = AimVector;
	Aim.Normalize();

	FVector AimXY = Aim;
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


void ASteikemannCharacter::Do_OnWallMechanics(float DeltaTime)
{
	if (GetMoveComponent()->IsWalking())
	{
		ResetWallJumpAndLedgeGrab();
		return;
	}

	/*		Detect Wall		 */
	static float PostWallJumpTimer{};	//	Post WallJump Timer		 
	if (GetMoveComponent()->bWallJump || GetMoveComponent()->bLedgeJump)
	{
		static float TimerLength{};
		if (GetMoveComponent()->bWallJump)	TimerLength = 0.25f;
		if (GetMoveComponent()->bLedgeJump)	TimerLength = 0.75f;
		PostWallJumpTimer += DeltaTime;
		if (PostWallJumpTimer > TimerLength) // Venter 0.25 sekunder før den skal kunne fortsette med å søke etter nære vegger 
		{
			PostWallJumpTimer = 0.f;
			bFoundWall = DetectNearbyWall();
		}
	}
	else {
		PostWallJumpTimer = 0.f;
		bFoundWall = DetectNearbyWall();

	}
	if (!bFoundWall) {
		bFoundLedge = false; bIsLedgeGrabbing = false;	// Ledgegrab bools
		bFoundStickableWall = false;					// Wall-Jump/Slide bool
	}
	(ActorToWall_Length < WallJump_ActivationRange) ? bFoundStickableWall = true : bFoundStickableWall = false;	// If the detected wall is within range for wall-jump/slide


	/*			Detect Ledge		  */
	if (!IsLedgeGrabbing() && (ActorToWall_Length < LedgeGrab_ActivationRange))
	{
		if (bFoundWall)
		{
			/*		Find Ledge 		 */
			bFoundLedge = DetectLedge(PlayersLedgeLocation, WallHit, LedgeHit, LedgeGrab_VerticalGrabLength, LedgeGrab_HorizontalGrabLength);
		}
	}

	/*			Ledge Grab			*/
	if (bFoundLedge)
	{
		bool b{};
		/* Only activate LedgeGrab if the actor is below the ledge */
		if ((GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight()) < LedgeHit.ImpactPoint.Z)
		{
			bIsLedgeGrabbing = true;
			if (LengthToLedge < GetVelocity().Z)
			{
				/* What do, if the velocity is higher than the grablength */
			}
			if (GetVelocity().Z >= 0.f)
			{
				if (GetVelocity().Z < 200.f)
				{
					b = Do_LedgeGrab(DeltaTime);
				}
			}
			else if (GetVelocity().Z < 0.f)
			{
				b = Do_LedgeGrab(DeltaTime);
			}
		}
	}


	/*			Wall Jump / Wall Slide			 */
	if (bFoundStickableWall && !bFoundLedge/* && (ActorToWall_Length < WallJump_ActivationRange)*/)
	{
		/* If the player is sticking to the wall */
		if (GetMoveComponent()->bStickingToWall)
		{
			//if (JumpCurrentCount == 2) { JumpCurrentCount = 1; }	// Resets DoubleJump

			/* Release from wall if they have stuck to it for the max duration */
			WallJump_StickTimer += DeltaTime;
			if (WallJump_StickTimer > WallJump_MaxStickTimer)
			{
				bOnWallActive = false;
				WallJump_NonStickTimer = 0.f;
				GetMoveComponent()->ReleaseFromWall(Wall_Normal);
				return;
			}
		}
		else { WallJump_StickTimer = 0.f; }

		/*		Rotating and moving actor to wall	  */
		if (GetMoveComponent()->bStickingToWall || GetMoveComponent()->bWallSlowDown)
		{
			SetActorLocation_WallJump(DeltaTime);
			RotateActorYawToVector(Wall_Normal * -1.f);
			RotateActorPitchToVector(Wall_Normal * -1.f);

			CalcAngleFromActorForwardToInput();

			if (InputDotProdToForward < -0.5f)	// Drop down
			{
				bOnWallActive = false;
				WallJump_NonStickTimer = 0.f;
				GetMoveComponent()->ReleaseFromWall(WallHit.Normal);
				return;
			}
		}
		// Wall Slow Down	(On way down)
		if (GetMoveComponent()->bWallSlowDown)
		{
			// Play particle effects
			if (NS_WallSlide) {
				Component_Niagara->SetAsset(NS_WallSlide);
				Component_Niagara->SetNiagaraVariableInt("User.SpawnAmount", NS_WallSlide_ParticleAmount * DeltaTime);
				Component_Niagara->SetWorldLocationAndRotation(GetMesh()->GetSocketLocation("Front"), GetMesh()->GetSocketRotation("Front"));
				Component_Niagara->Activate(true);
			}

			// Play Sound

		}
	// Do adjustments to the actor rotation acording to animation
	}
}

bool ASteikemannCharacter::DetectNearbyWall()
{
	bool bHit{};
	FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);

	/* Raytrace in a total of 8 directions around the player */
	
	/* Raytrace first in the Forward/Backwards axis and Right/Left */
	FVector LinetraceVector = GetControlRotation().Vector();
		LinetraceVector.Z = 0.f;
		LinetraceVector.Normalize();

	for (int i = 0; i < 4; i++)
	{
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (LinetraceVector * WallDetectionRange), FColor::Yellow, false, 0.f, 0, 4.f);
		bHit = GetWorld()->LineTraceSingleByChannel(WallHit, GetActorLocation(), GetActorLocation() + (LinetraceVector * WallDetectionRange), ECC_Visibility, Params);
		LinetraceVector = LinetraceVector.RotateAngleAxis(90, FVector(0, 0, 1));
		if (bHit) { 
			StickingSpot = WallHit.ImpactPoint; 
			Wall_Normal = WallHit.Normal; 
			//return bHit; 
			break;
		}
	}

	if (!bHit)
	{
		/* Then do raytrace of the 45 degree angle between the 4 previous axis */
		LinetraceVector = LinetraceVector.RotateAngleAxis(45.f, FVector(0, 0, 1));
		for (int i = 0; i < 4; i++)
		{
				//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (LinetraceVector * WallDetectionRange), FColor::Red, false, 0.f, 0, 4.f);
			bHit = GetWorld()->LineTraceSingleByChannel(WallHit, GetActorLocation(), GetActorLocation() + (LinetraceVector * WallDetectionRange), ECC_Visibility, Params);
			LinetraceVector = LinetraceVector.RotateAngleAxis(90, FVector(0, 0, 1));
			if (bHit) {
				StickingSpot = WallHit.ImpactPoint;
				Wall_Normal = WallHit.Normal;
				//return bHit;
				break;
			}
		}
	}

	/*		Dette fungerer på flate vegger, men kommer til og føre til problemer på cylindriske- / mer avanserte- former	 */
		/* Burde kjøre ekstra sjekker for å se om det er andre flater i nærheten som passer bedre */
	if (bHit)
	{
		FromActorToWall = WallHit.Normal * -1.f;

		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (FromActorToWall * 200.f), FColor::White, false, 0.f, 0.f, 6.f);

		const bool b = GetWorld()->LineTraceSingleByChannel(WallHit, GetActorLocation(), GetActorLocation() + (FromActorToWall * 200.f), ECC_Visibility, Params);
		if (b)
		{
			ActorToWall_Length = FVector(GetActorLocation() - WallHit.ImpactPoint).Size();
			return b;
		}
	}
		/* Hvordan håndtere hjørner? 
				Sjekke om veggen som er truffet har en passende vinkel i forhold til playercontroller? 
				Er det den eneste veggen vi treffer? (TArray kanskje for å samle alle nære vegger) */
	ActorToWall_Length = WallDetectionRange;
	return false;
}

void ASteikemannCharacter::CalcAngleFromActorForwardToInput()
{
	/* Checks the angle between the actors forward axis and the input vector. Used in animations*/
	if (InputVector.SizeSquared() > 0.2f)
	{
		InputAngleToForward = FMath::RadiansToDegrees(acosf(FVector::DotProduct(GetActorForwardVector(), InputVector)));
		float InputAngleDirection = FVector::DotProduct(GetActorRightVector(), InputVector);
		if (InputAngleDirection > 0.f) { InputAngleToForward *= -1.f; }

		InputDotProdToForward = FVector::DotProduct(GetActorForwardVector(), InputVector);
	}
	else { InputAngleToForward = 0.f; }
}

void ASteikemannCharacter::SetActorLocation_WallJump(float DeltaTime)
{
	float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() + OnWall_ExtraCharacterLengthFromWall;
	if (ActorToWall_Length > CapsuleRadius) 
	{
		float Move{};
		if ((ActorToWall_Length - CapsuleRadius) > 5.f) {
			Move = FMath::FInterpTo(Move, ActorToWall_Length - CapsuleRadius, DeltaTime, 10.f);
		}
		else {
			Move = ActorToWall_Length - CapsuleRadius;
		}
		SetActorRelativeLocation(GetActorLocation() + (FromActorToWall * Move), false, nullptr, ETeleportType::TeleportPhysics);
	}
}

bool ASteikemannCharacter::IsStickingToWall() const
{
	if (GetMoveComponent().IsValid()) {
		return GetMoveComponent()->bStickingToWall;
	}
	return false;
}

bool ASteikemannCharacter::Jump_DetectWall()
{
	PRINTLONG("DETECTING JUMP WALL");
	bool b{};
	FTimerHandle h;
	//auto func = [](FTimerHandle h){
		//return true;
		//GetWorldTimerManager().SetTimer(h, this, &ASteikemannCharacter::WallJump_Reset, PostWallJumpTimer);
	//}

	bFoundWall = DetectNearbyWall();

	(ActorToWall_Length < WallJump_JumpWallActivation) ?
		b = true:
		b = false;

	if (b) { GetWorldTimerManager().SetTimer(h, this, &ASteikemannCharacter::WallJump_Reset, fPostWallJumpTimer); }

	bPostWallJump = b;
	return b;
}

void ASteikemannCharacter::WallJump_Reset()
{
	bPostWallJump = false;
}

bool ASteikemannCharacter::IsOnWall() const
{
	if (GetMoveComponent().IsValid()) {
		return GetMoveComponent()->bWallSlowDown;
	}
	return false;
}

bool ASteikemannCharacter::Do_LedgeGrab(float DeltaTime)
{
	bIsLedgeGrabbing = true;

	GetMoveComponent()->Start_LedgeGrab();
	MoveActorToLedge(DeltaTime);
	RotateActorYawToVector(Wall_Normal * -1.f);
	RotateActorPitchToVector(Wall_Normal * -1.f);

	CalcAngleFromActorForwardToInput();

	/* Tmp DebugArms */
	DrawDebugArms(InputAngleToForward);


	if (InputDotProdToForward < -0.5f)	// Drop down
	{
		bOnWallActive = false;
		WallJump_NonStickTimer = 0.f;
		GetMoveComponent()->ReleaseFromWall(WallHit.Normal);
		ResetWallJumpAndLedgeGrab();
		return false;
	}

	return true;
}

bool ASteikemannCharacter::IsGrappling() const
{
	return bGrapple_PreLaunch || bGrapple_Launch || bIsGrapplehooking;
}


void ASteikemannCharacter::CanBeAttacked()
{
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

	bIsScoopAttacking = true;
}

void ASteikemannCharacter::Deactivate_ScoopAttack()
{
	bScoopAttackMoveCharacter = false;
	bHasbeenScoopLaunched = false;

	
}

void ASteikemannCharacter::Activate_SmackAttack()
{
	/* Character movement during attack */
	bPreBasicAttackMoveCharacter = false;
	bSmackAttackMoveCharacter = true;

	bIsSmackAttacking = true;
}

void ASteikemannCharacter::Deactivate_SmackAttack()
{
	bSmackAttackMoveCharacter = false;
}

void ASteikemannCharacter::Click_Attack()
{
	/* Return conditions */
	if (bAttackPress) { return; }
	if (!bCanAttack) { return; }
	if (bAttacking) { return; }
	if (IsLedgeGrabbing()) { return; }
	if (IsOnWall() || IsStickingToWall()) { return; }
	if (IsGrappling()) 
	{ 
		/* Buffer Attack if Grappling to Dynamic Target */
		if (bGrapplingDynamicTarget)
		{
			FTimerHandle h;
			float t = GetWorldTimerManager().GetTimerRemaining(TH_Grapplehook_Start);
			GetWorldTimerManager().SetTimer(h, this, &ASteikemannCharacter::Click_Attack, t);
		}
		return; 
	}	
		

	bAttackPress = true;
	
	bAttacking = true;
	bCanAttack = false;

	RotateToAttack();
	Start_Attack();
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
	if (!bCanAttack) { return; }
	if (bAttacking) { return; }

	bClickScoopAttack = true;
	
	bAttacking = true;
	bCanAttack = false;

	Start_ScoopAttack_Pure();
}

void ASteikemannCharacter::UnClick_ScoopAttack()
{
	bClickScoopAttack = false;
}

void ASteikemannCharacter::Start_Attack_Pure()
{
	/* Movement during attack */
	bPreBasicAttackMoveCharacter = true;
}

void ASteikemannCharacter::Stop_Attack()
{
	//PRINTLONG("Can attack again");
	bCanAttack = true;
	bIsScoopAttacking = false;
	bIsSmackAttacking = false;
}

void ASteikemannCharacter::Activate_AttackCollider()
{
			AttackCollider->SetHiddenInGame(false);	// For Debugging
	AttackCollider->SetGenerateOverlapEvents(true);
	AttackCollider->SetRelativeScale3D(AttackColliderScale);
}

void ASteikemannCharacter::Deactivate_AttackCollider()
{
	//PRINTLONG("Deactivate Attack");
	bAttacking = false;
	AttackDirection *= 0;

	AttackCollider->SetHiddenInGame(true);	// For Debugging
	AttackCollider->SetGenerateOverlapEvents(false);
	AttackCollider->SetRelativeScale3D(FVector(0, 0, 0));
}

bool ASteikemannCharacter::DecideAttackType()
{
	/* Stop moving character using the PreBasicAttackMoveCharacter */
	bPreBasicAttackMoveCharacter = false;

	/* Do SCOOP ATTACK if attack button is still held */
	if (bAttackPress && !GetMoveComponent()->IsFalling())
	{
		Activate_ScoopAttack();
		bIsSmackAttacking = false;
		return true;
	}

	/* SMACK ATTACK if button is not held */
	Activate_SmackAttack();
	bIsSmackAttacking = true;
	return false;
}

void ASteikemannCharacter::RotateToAttack()
{
	if (bSmackDirectionDecidedByInput)
	{
		AttackDirection = InputVector;
	}
	if (!bSmackDirectionDecidedByInput || InputVector.IsNearlyZero())
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
		/* Smack attack collider */
		if (OverlappedComp == AttackCollider)
		{
			IAttackInterface* Interface = Cast<IAttackInterface>(OtherActor);
			if (Interface) {
				if (bIsSmackAttacking)
				{
					Do_SmackAttack_Pure(Interface, OtherActor);
				}
				else /*if (bIsScoopAttacking)*/
				{
					Do_ScoopAttack_Pure(Interface, OtherActor);
				}
			}


		}

		/* GroundPound collision */
		else if (OverlappedComp == GroundPoundCollider)
		{
			IAttackInterface* Interface = Cast<IAttackInterface>(OtherActor);
			if (Interface) {
				Do_GroundPound_Pure(Interface, OtherActor);
			}
		}
	}
}


void ASteikemannCharacter::Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	// Burde sjekke om den kan bli angrepet i det hele tatt. 
	const bool b{ OtherInterface->GetCanBeSmackAttacked() };

	if (b)
	{
		FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
		Direction = Direction.GetSafeNormal2D();
		float angle = FMath::DegreesToRadians(SmackUpwardAngle);
		Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);
		OtherInterface->Receive_SmackAttack_Pure(Direction, SmackAttackStrength);
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
	//PRINTLONG("Click GroundPound");
	if (!bGroundPoundPress && GetMoveComponent()->IsFalling() && !bIsGroundPounding)
	{
		bGroundPoundPress = true;

		Start_GroundPound();
	}
}

void ASteikemannCharacter::UnClick_GroundPound()
{
	bGroundPoundPress = false;
}


void ASteikemannCharacter::Launch_GroundPound()
{
	GetMoveComponent()->GP_Launch();
}

void ASteikemannCharacter::Start_GroundPound()
{
	bCanGroundPound = false;
	bIsGroundPounding = true;

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

	bIsGroundPounding = false;
	bCanGroundPound = true;
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


void ASteikemannCharacter::GrappleHook_Drag_RotateCamera(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }

	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();

	FRotator radiusRotation = radius.Rotation();
	FRotator PitchPoint = radiusRotation - FRotator{ GrappleDrag_Camera_PitchPoint, 0, 0 };

	FVector con = GetControlRotation().Vector();
	FRotator controllerRotation = con.Rotation();

	float YawTo = radiusRotation.Yaw - controllerRotation.Yaw;

	float PitchTo = controllerRotation.Pitch - PitchPoint.Pitch;


	float YawRotate = FMath::FInterpTo(0.f, YawTo, DeltaTime, GrappleDrag_Camera_InterpSpeed);
	AddControllerYawInput(YawRotate);

	float PitchRotate = FMath::FInterpTo(0.f, PitchTo, DeltaTime, GrappleDrag_Camera_InterpSpeed);
	AddControllerPitchInput(PitchRotate);
}

void ASteikemannCharacter::RotateActor_GrappleHook_Drag(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }

	RotateActorYawToVector(GrappledActor->GetActorLocation() - GetActorLocation());
	RotateActorPitchToVector(GrappledActor->GetActorLocation() - GetActorLocation());
}
