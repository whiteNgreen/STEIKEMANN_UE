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


	/* Creating Niagara Compnents */
	{
		NiComp_CrouchSlide = CreateNiagaraComponent("Niagara_CrouchSlide", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
		if (NiComp_CrouchSlide) {
			NiComp_CrouchSlide->bAutoActivate = false;
			if (NS_CrouchSlide) { NiComp_CrouchSlide->SetAsset(NS_CrouchSlide); }
		}

	}
	
	/* Setting Variables */
	{
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;

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
		GrappleTargetingDetectionSphere->SetGenerateOverlapEvents(true);
		GrappleTargetingDetectionSphere->SetSphereRadius(GrappleHookRange);
	}

	/*
	* Adding GameplayTags to the GameplayTagsContainer
	*/
	//Player = FGameplayTag::RequestGameplayTag("Pottit");
	Player = Tag_Player;
	GameplayTags.AddTag(Player);
	//PRINTPARLONG("%s", *Player.GetTagName().ToString());
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
		PRINTLONG("Niagara Component not yet completed with its task. Creating temp Niagara Component.");

		UNiagaraComponent* TempNiagaraLand = CreateNiagaraComponent("Niagara_Land", RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, true);
		NiagaraPlayer = TempNiagaraLand;
	}
	
	NiagaraPlayer->SetAsset(NS_Land);
	NiagaraPlayer->SetNiagaraVariableInt("User.SpawnAmount", static_cast<int>(Velocity * NSM_Land_ParticleAmount));
	NiagaraPlayer->SetNiagaraVariableFloat("User.Velocity", Velocity * NSM_Land_ParticleSpeed);
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
	//PRINTPAR("InputVector: %s", *InputVector.ToString());
	//PRINTPAR("InputVectorRaw: %s", *InputVectorRaw.ToString());


	/*		Resets Rotation Pitch and Roll		*/
	if (IsFalling() || GetMoveComponent()->IsWalking()) {
		ResetActorRotationPitchAndRoll(DeltaTime);
	}

	/*		Slipping		  */
	//DetectPhysMaterial();


	
	/*		Jump		*/
	if (bJumping){
		if (JumpKeyHoldTime < fJumpTimerMax){
			JumpKeyHoldTime += DeltaTime;
		}
		else{
			bAddJumpVelocity = false;
		}
	}
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


	/*			Dash			*/
		/* Determines the dash direction vector based on input and controller rotation */
	if (InputVectorRaw.Size() <= 0.05)
	{
		FVector Dir = GetControlRotation().Vector();
		Dir.Z = 0;
		Dir.Normalize();

		DashDirection = Dir;
	}
	else
	{
		DashDirection = InputVectorRaw;
		FRotator Rot = GetControlRotation();
		
		DashDirection = DashDirection.RotateAngleAxis(Rot.Yaw, FVector(0, 0, 1));
	}
	if (GetCharacterMovement()->IsWalking() || IsGrappling()) {
		DashCounter = 1;
	}
	if (IsDashing()) { RotateActorYawToVector(DashDirection, DeltaTime); }



	/* Resetting Wall Jump & Ledge Grab when player is on the ground */
	if (GetMoveComponent()->IsWalking()) {
		WallJump_NonStickTimer = 0.f;

		//ResetWallJumpAndLedgeGrab();
	}



	/*		Wall Jump & LedgeGrab		*/
	//if (GetMoveComponent()->IsFalling())
	//if ((GetMoveComponent()->IsFalling()) && !IsGrappling() && bOnWallActive) 
	if (GetMoveComponent()->IsFalling() && bOnWallActive)
	{
		PRINT("Do OnWallMechanics");
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
	PRINTPAR("WallJump NonstickTimer: %f", WallJump_NonStickTimer);
	bOnWallActive ? PRINT("bOnWallActive = TRUE") : PRINT("bOnWallActive = FALSE");


	/*		Activate grapple targeting		*/
	if (!IsGrappling())
	{
		LineTraceToGrappleableObject();
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
		GrappleHookMesh->SetVisibility(false);
	}
	/*		Grapplehook			*/
	if ((bGrapple_Swing/* && !bGrappleEnd*/) || (bGrapple_PreLaunch/* && !bGrappleEnd*/)) 
	{
		if (GrappledActor.IsValid())
		{
			GrappleRope = GrappledActor->GetActorLocation() - GetActorLocation();
		}

		if (!bGrapple_PreLaunch) {
			//GrappleHook_Swing_RotateCamera(DeltaTime);
			Update_GrappleHook_Swing(DeltaTime);
			if (GetMovementComponent()->IsFalling() && !IsJumping()) { RotateActor_GrappleHook_Swing(DeltaTime); }
			bGrapple_Launch = false;
		}
		else {
			GrappleHook_Drag_RotateCamera(DeltaTime);
			RotateActor_GrappleHook_Drag(DeltaTime);
			if (!bGrapple_Launch) {
				Initial_GrappleHook_Drag(DeltaTime);
			}
			else {
				Update_GrappleHook_Drag(DeltaTime);
			}
		}
	}
	if (IsGrappling()) {
		GrappleHookMesh->SetVisibility(true);
		if (GrappledActor.IsValid()) {
			GrappleHookMesh->SetBoneLocationByName(FName("GrappleHook_Top"), GrappledActor->GetActorLocation(), EBoneSpaces::WorldSpace);
		}
	}

	
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


	/* Crouch */
	//PRINTPAR("Capsule Radius (Unscaled): %f", GetCapsuleComponent()->GetUnscaledCapsuleRadius());
	//PRINTPAR("Capsule Radius   (Scaled): %f", GetCapsuleComponent()->GetScaledCapsuleRadius());
	//PRINTPAR("Capsule Height (Unscaled): %f", GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
	//PRINTPAR("Capsule Height   (Scaled): %f", GetCapsuleComponent()->GetScaledCapsuleHalfHeight());


	/*	
	*	POGO BOUNCE
	* 
	* Raytrace under player to see if they hit an enemy
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


	/* ----------------------- COMBAT TICKS ------------------------------ */
	PreBasicAttackMoveCharacter(DeltaTime);
	SmackAttackMoveCharacter(DeltaTime);
	ScoopAttackMoveCharacter(DeltaTime);

	/* Various debug stuff */
	//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (GetActorForwardVector() * 100.f), FColor::Orange, false, 0.f, 0, 3.f);

	/* Check Jump Count */
	PRINTPAR("Jump Count: %i", JumpCurrentCount);
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


		/* Crouch*/
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ASteikemannCharacter::Start_Crouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ASteikemannCharacter::Stop_Crouch);


	/* Bounce */
	PlayerInputComponent->BindAction("Bounce", IE_Pressed, this, &ASteikemannCharacter::Bounce).bConsumeInput = true;
	PlayerInputComponent->BindAction("Bounce", IE_Released, this, &ASteikemannCharacter::Stop_Bounce).bConsumeInput = true;

	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ASteikemannCharacter::Dash).bConsumeInput = true;
	PlayerInputComponent->BindAction("Dash", IE_Released, this, &ASteikemannCharacter::Stop_Dash).bConsumeInput = true;


	/* GrappleHook */
		/* Grapplehook SWING */
	//PlayerInputComponent->BindAction("GrappleHook_Swing", IE_Pressed, this,	  &ASteikemannCharacter::Start_Grapple_Swing);
	//PlayerInputComponent->BindAction("GrappleHook_Swing", IE_Released, this,  &ASteikemannCharacter::Stop_Grapple_Swing);
		/* Grapplehook DRAG */
	//PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Pressed, this,	  &ASteikemannCharacter::Start_Grapple_Drag);	// Forandre funksjonen / navnet på den, som spilles av og hva input actione heter
	//PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Released, this,	  &ASteikemannCharacter::Stop_Grapple_Drag);
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Pressed, this,	  &ASteikemannCharacter::RightTriggerClick);
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Released, this,	  &ASteikemannCharacter::RightTriggerUn_Click);

	/* Attack - SmackAttack */
	PlayerInputComponent->BindAction("SmackAttack", IE_Pressed, this, &ASteikemannCharacter::Click_Attack);
	PlayerInputComponent->BindAction("SmackAttack", IE_Released, this, &ASteikemannCharacter::UnClick_Attack);

	/* Attack - GroundPound */
	PlayerInputComponent->BindAction("GroundPound", IE_Pressed, this, &ASteikemannCharacter::Click_GroundPound);
	PlayerInputComponent->BindAction("GroundPound", IE_Released, this, &ASteikemannCharacter::UnClick_GroundPound);
}

void ASteikemannCharacter::Start_Grapple_Swing()
{
	if (bGrapple_Available)
	{
		bGrapple_Swing = true;
		Initial_GrappleHook_Swing();
		if (GrappledActor.IsValid())
		{
			IGrappleTargetInterface::Execute_Hooked(GrappledActor.Get());
		}

		if (JumpCurrentCount == 2) { JumpCurrentCount--; }
		
		/* Resetter grappleswing boosten */
		ResetGrappleSwingBoost();
	}
}

void ASteikemannCharacter::Stop_Grapple_Swing()
{
	bGrapple_Swing = false;
	if (GrappledActor.IsValid() && IsGrappling())
	{
		IGrappleTargetInterface::Execute_UnTargeted(GrappledActor.Get());
	}

}

void ASteikemannCharacter::Start_Grapple_Drag()
{
	/* Adjusting rope instead of doing the grappledrag */
	//if (bShouldAdjustRope)
	//{
	//	if (IsGrappleSwinging() && GetMoveComponent()->IsFalling())
	//	{
	//		//AdjustRopeLength(GrappleRope);
	//		bIsAdjustingRopeLength = true;

	//	}
	//	return;
	//}
	if (GrappledActor.IsValid())
	{
		bGrapple_PreLaunch = true;
		IGrappleTargetInterface::Execute_Hooked(GrappledActor.Get());
		if (JumpCurrentCount == 2) { JumpCurrentCount--; }
	}
}

void ASteikemannCharacter::Stop_Grapple_Drag()
{
	//if (bShouldAdjustRope)
	//{ 
	//	bIsAdjustingRopeLength = false;
	//	return;
	//}

	bGrapple_PreLaunch = false;
	bGrapple_Launch = false;
	bGrapple_Swing = false;
	bGrappleEnd = false;
	if (GrappledActor.IsValid() && IsGrappling())
	{
		IGrappleTargetInterface::Execute_UnTargeted(GrappledActor.Get());
	}

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

void ASteikemannCharacter::MoveForward(float value)
{
	InputVectorRaw.X = value;

	if (IsStickingToWall()) { return; }
	if (IsLedgeGrabbing()) { return; }
	if (IsCrouchSliding()) { return; }
	if (IsAdjustingRopeLength()) { return; }
	if (IsAttacking()) { return; }

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
	if (IsAdjustingRopeLength()) { return; }
	if (IsAttacking()) { return; }

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

	if (!bJumpClick)
	{
		bJumpClick = true;
		bJumping = true;
		/* ------ VARIOUS TYPES OF JUMPS ------- */

		/* ---- CROUCH - AND SLIDE- JUMP ---- */
		if (IsCrouching() && GetMoveComponent()->IsWalking())
		{
			/* ---- CROUCH JUMP ---- */
			if (IsCrouchWalking())
			{
				PRINTLONG("--CrouchJump--");
				JumpCurrentCount++;
				bAddJumpVelocity = CanCrouchJump();
				GetMoveComponent()->StartCrouchJump();
				Anim_Activate_Jump();	// Anim_Activate_CrouchJump()
				return;
			}

			/* ---- CROUCH SLIDE JUMP ---- */
			if (IsCrouchSliding())
			{
				JumpCurrentCount++;
				{
					FVector CrouchSlideDirection = GetVelocity();
					CrouchSlideDirection.Normalize();
					bAddJumpVelocity = GetMoveComponent()->CrouchSlideJump(CrouchSlideDirection, InputVector);
				}
				Anim_Activate_Jump();	// Anim_Activate_CrouchSlideJump()
				return;
			}
		}

		/* ---- LEDGE JUMP ---- */
		//if ((GetMoveComponent()->bLedgeGrab) && GetMoveComponent()->IsFalling())
		if (IsLedgeGrabbing() && GetMoveComponent()->IsFalling())
		{
			PRINTLONG("LEDGE JUMP");
			JumpCurrentCount = 1;

			ResetWallJumpAndLedgeGrab();
			bOnWallActive = false;
			WallJump_NonStickTimer = 0.f;

			/*bAddJumpVelocity = */GetMoveComponent()->LedgeJump(LedgeLocation, JumpStrength);
			Anim_Activate_Jump();	//	Anim_Activate_LedgeGrabJump
			return;
		}

		/* ---- WALL JUMP ---- */
		if ((GetMoveComponent()->bStickingToWall || bFoundStickableWall) && GetMoveComponent()->IsFalling())
		{
			//bAddJumpVelocity = GetMoveComponent()->WallJump(Wall_Normal);
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
				PRINTLONG("POST EDGE JUMP");
				JumpCurrentCount++;
			}
			/* after post edge timer is valid */
			else 
			{
				JumpCurrentCount = 2;
			}
			GetMoveComponent()->Jump(JumpStrength);
			return;
		}


		/* ---- DOUBLE JUMP ---- */
		if (CanDoubleJump())
		{
			JumpCurrentCount++;
			GetMoveComponent()->Jump(JumpStrength);
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

	//bPressedJump = true;
	//bJumping = true;	
	//bAddJumpVelocity = (CanJump() || CanDoubleJump());
	
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
					PRINTLONG("--CrouchJump--");
					JumpCurrentCount++;
					bAddJumpVelocity = CanCrouchJump();
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
						bAddJumpVelocity = GetMoveComponent()->CrouchSlideJump(CrouchSlideDirection, InputVector);
					}
					Anim_Activate_Jump();	// Anim_Activate_CrouchSlideJump()
					return;
				}
			}

			/* If player is LedgeGrabbing */
			if ((GetMoveComponent()->bLedgeGrab) && GetMoveComponent()->IsFalling())
			{
				JumpCurrentCount = 1;
				bAddJumpVelocity = GetMoveComponent()->LedgeJump(LedgeLocation, JumpStrength);
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
				//JumpCurrentCount++;
				//JumpCurrentCount = 2;
				//bAddJumpVelocity = GetMoveComponent()->WallJump(Wall_Normal);
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
				bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
				return;
			}
			/* If player walks off edge with no jumpcount after post edge timer is valid */
			if (GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0)
			{
				bCanEdgeJump = true;
				JumpCurrentCount += 2;
				Anim_Activate_Jump();
				bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
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
	//return bAddJumpVelocity && bJumping;
	//return bAddJumpVelocity && bJumping && bActivateJump;
}

bool ASteikemannCharacter::IsFalling() const
{
	if (!GetMoveComponent().IsValid()) { return false; }
	return  GetMoveComponent()->MovementMode == MOVE_Falling  && ( !IsGrappling() && !IsDashing() && !IsStickingToWall() && !IsOnWall() && !IsJumping() );
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
		
		if (Container.HasTagExact(Tag_EnemyAubergineDoggo))
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
	PRINTLONG("POGO BOUNCE");

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
		PRINTLONG("Crouch");
		
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
		PRINTLONG("UnCrouch");

		bIsCrouchWalking = false;

		UnCrouch();
	}
}

void ASteikemannCharacter::Start_CrouchSliding()
{
	if (!bCrouchSliding)
	{
		PRINTLONG("Start CrouchSliding");
		
		bCrouchSliding = true;

		GetCapsuleComponent()->SetCapsuleRadius(25.f);	// Temporary solution for cube to crouch

		Crouch();

		NiComp_CrouchSlide->SetNiagaraVariableVec3("User.M_Velocity", GetVelocity() * -1.f);
		NiComp_CrouchSlide->Activate();

		GetMoveComponent()->Initiate_CrouchSlide(InputVector);

		GetWorldTimerManager().SetTimer(CrouchSlide_TimerHandle, this, &ASteikemannCharacter::Stop_CrouchSliding, CrouchSlide_Time);
	}
}

void ASteikemannCharacter::Stop_CrouchSliding()
{
	PRINTLONG("STOP CrouchSliding");

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

	//		Finds the orthogonal vector from the players forward axis (which matches wall.normal * -1.f) towards the UpVector
	FVector OrthoPlayerToUp{ FVector::CrossProduct(In_WallHit.Normal * -1.f, FVector::CrossProduct(FVector::UpVector, In_WallHit.Normal * -1.f)) };

	//		Common locations
	FVector LocationFromWallHit	{ In_WallHit.ImpactPoint + (In_WallHit.Normal * (GetCapsuleComponent()->GetUnscaledCapsuleRadius() + 5.f))};	// What would be the players location from the wall itself
		//DrawDebugBox(GetWorld(), LocationFromWallHit, FVector(10), FColor::White, false, 0.f, 1, 3.f);

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
			
			
			/* Draw Permanent DebugLines  */
			{
				DrawDebugLine(GetWorld(), FirstTracePoint_Start,	FirstTracePoint_End,	FColor::Red,	false, 1.f, 0, 4.f);
				DrawDebugLine(GetWorld(), SecondTracePoint_Start,	SecondTracePoint_End,	FColor::Blue,	false, 1.f, 0, 4.f);
			}

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
			RightArmLocation = FMath::Lerp(RightArmLocation, RightArmLocation2, Alpha);
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
			LeftArmLocation = FMath::Lerp(LeftArmLocation, LeftArmLocation2, Alpha);
		}
		DrawDebugLine(GetWorld(), GetActorLocation(), LeftArmLocation, FColor::Emerald, false, 0.f, 0, 6.f);
		DrawDebugBox(GetWorld(), LeftArmLocation, FVector(30, 30, 30), Rot.Quaternion(), FColor::Emerald, false, 0.f, 0, 4.f);
	}
}


void ASteikemannCharacter::ResetWallJumpAndLedgeGrab()
{
	//bOnWallActive						= false;

	GetMoveComponent()->bWallSlowDown	= false;
	GetMoveComponent()->bStickingToWall	= false;
	bFoundStickableWall					= false;
	InputAngleToForward					= 0.f;
	InputDotProdToForward				= 1.f;

	//WallJump_NonStickTimer				= 0.f;

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

	PRINTPAR("Pitch Angle: %f", Pitch);

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
		IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(OtherActor);
		if (TagInterface)
		{

			PRINTPARLONG("Detecting Actor: %s", *OtherActor->GetName());
		}
		else
		{
			//IGrappleTargetInterface
		}
	}
}

/* Aiming system for grapplehook */
bool ASteikemannCharacter::LineTraceToGrappleableObject()
{
	FVector DeprojectWorldLocation{};
	FVector DeprojectDirection{};

	TArray<TWeakObjectPtr<AActor>> OnScreenActors{};

	ViewPortSize = GEngine->GameViewport->Viewport->GetSizeXY();
	float Step = ViewPortSize.X / 16;	// Assuming the aspect ratio is 16:9 

	FHitResult Hit{};
	TArray<FHitResult> MultiHit{};
	FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);

	FVector2D DeprojectScreenLocation{};

	int traces{};

	TWeakObjectPtr<AActor> Grappled{ nullptr };
	
	/* Adjusting the GrappleAimYChange based on the playercontrollers pitch */
	FVector BackwardControllerPitch = GetControlRotation().Vector() * -1.f;
	BackwardControllerPitch *= FVector(0, 0, 1);

	AimingLocation = ViewPortSize / 2 - FVector2D(0.f, ViewPortSize.Y / GrappleAimYChange_Base);
	AimingLocationPercentage = FVector2D(AimingLocation.X / ViewPortSize.X, AimingLocation.Y / ViewPortSize.Y);

	/* Do a multilinetrace from the onscreen location of 'AimingLocation' */
	UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(GetWorld(), 0), AimingLocation, DeprojectWorldLocation, DeprojectDirection);
	GetWorld()->LineTraceMultiByChannel(MultiHit, DeprojectWorldLocation, DeprojectWorldLocation + (DeprojectDirection * GrappleHookRange), GRAPPLE_HOOK, Params);
	if (MultiHit.Num() > 0) 
	{ 
		for (const auto& it : MultiHit) {
			OnScreenActors.AddUnique(it.GetActor());
		}
	}

	/* If the linetrace hit grappletarget actors, find the one closest to the middle of the screen */
	if (OnScreenActors.Num() > 0)
	{
		float range{ 10000.f };
		FVector2D OnScreenLocation;
		
		for (const auto& it : OnScreenActors)
		{
			UGameplayStatics::ProjectWorldToScreen(UGameplayStatics::GetPlayerController(GetWorld(), 0), it->GetActorLocation(), OnScreenLocation);
			
			FVector2D LengthToTarget = AimingLocation - OnScreenLocation;
			float L = LengthToTarget.Size();

			if (L < range)
			{
				Grappled = it;
				range = L;
			}
		}
	} 
	else { 
		bGrapple_Available = false;
		Grappled = nullptr;
	}


	if (Grappled.IsValid()) { 
		bGrapple_Available = true; 

		/* Fører til kræsj når en ser på fienden */
		//IGrappleTargetInterface* Interface = Cast<IGrappleTargetInterface>(Grappled.Get());
		//if (Interface)
		//{
		//	Interface->TargetedPure();
		//	GpT_GrappledActorTag = Interface->GetGrappledGameplayTag_Pure();
		//	PRINTPAR("Grappled Tag: %s", *GpT_GrappledActorTag.GetTagName().ToString());
		//}
		/* If the interface does not exist, we TEMPORARILY directly call the Blueprint Functions */
		//else
		{
			/* Interface functions to the actor that is aimed at */
			if (!IsGrappling()) {
				IGrappleTargetInterface::Execute_Targeted(Grappled.Get());

				FGameplayTag tag = IGrappleTargetInterface::Execute_GetGrappledGameplayTag(Grappled.Get());
				//PRINTPAR("Grappled Tag: %s", *tag.GetTagName().ToString());
			}

			/* Interface function to un_target the actor we aimed at */
			if (Grappled != GrappledActor && GrappledActor.IsValid())
			{
				IGrappleTargetInterface::Execute_UnTargeted(GrappledActor.Get());
			}
		}
	}
	else {		/* Untarget if Grappled returned NULL */
		if (GrappledActor.IsValid()){	
			IGrappleTargetInterface::Execute_UnTargeted(GrappledActor.Get());	// Call BP Interface Function
		}

		GpT_GrappledActorTag = FGameplayTag::EmptyTag;
	}

	GrappledActor = Grappled;

	return bGrapple_Available;
}

void ASteikemannCharacter::Bounce()
{
	if (!bBounceClick && GetCharacterMovement()->GetMovementName() == "Falling")
	{
		FHitResult Hit;
		FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);
		bBounce = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() - FVector{ 0, 0, BounceCheckLength }, ECC_Visibility, Params);
		if (bBounce) {
			GetMoveComponent()->Bounce(Hit.ImpactNormal);
		}
	}

	bBounceClick = true;
}

void ASteikemannCharacter::Stop_Bounce()
{
	bBounceClick = false;
	bBounce = false;
}

void ASteikemannCharacter::Dash()
{
	if (!bDashClick)
	{
		bDashClick = true;
	
		/* If character is swinging then the dash is not executed */
		if (IsGrappleSwinging())
		{
			/* If LT + RT is held, then launch the character with Grapplehook_Drag */
			if (IsAdjustingRopeLength())
			{
				Start_Grapple_Drag();
			}

			/* Do the grapplehook swing boost, if they can */
			if (bCanGrappleSwingBoost && DashCounter == 1)
			{
				//PRINTLONG("GrappleHook BOOST----");
				/* The Boost */	// Should be function? 
				{
					DashCounter--;
					bCanGrappleSwingBoost = false;

					FVector BoostDirection = GetMoveComponent()->Velocity;
					BoostDirection.Normalize();
					GetMoveComponent()->AddImpulse(BoostDirection * GrappleSwingBoostStrength, true);
				}
				GetWorldTimerManager().SetTimer(TH_ResetGrappleSwingBoost, this, &ASteikemannCharacter::ResetGrappleSwingBoost, TimeBetweenGrappleSwingBoosts);

				//return;
			}
		}

		else if (!bDash && DashCounter == 1)
		{
			PRINTLONG("----DASH----");
			bDash = true;
			DashCounter--;
			Activate_Dash();
			GetMoveComponent()->Start_Dash(Pre_DashTime, DashTime, DashLength, DashDirection);
		}
	}
}

void ASteikemannCharacter::Stop_Dash()
{
	bDashClick = false;
}

bool ASteikemannCharacter::IsDashing() const
{
	return bDash;
}

void ASteikemannCharacter::ResetGrappleSwingBoost()
{
	DashCounter = 1;
	bCanGrappleSwingBoost = true;
}

void ASteikemannCharacter::RightTriggerClick()
{
	if (GrappledActor.IsValid())
	{
		//FGameplayTag GrappledTag;
		FGameplayTagContainer GrappledTags;
		IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(GrappledActor.Get());
		if (TagInterface)
		{
			TagInterface->GetOwnedGameplayTags(GrappledTags);

			//if (GrappledTags.HasTag(Tag_GrappleTarget_Dynamic))	// Burde bruke denne taggen på fiendene
			if (GrappledTags.HasTag(Tag_EnemyAubergineDoggo))		// Også kanskje spesifisere videre med fiende type
			{
				PRINTLONG("Grappled to DYNAMIC TARGET");
				
				IGrappleTargetInterface* GrappleInterface = Cast<IGrappleTargetInterface>(GrappledActor.Get());
				if (GrappleInterface)
				{
					GrappleInterface->HookedPure(GetActorLocation());
				}
			}
		}
		else
		{
			Start_Grapple_Drag();
		}

	}
	
}

void ASteikemannCharacter::RightTriggerUn_Click()
{
	/* Skal spilleren faktisk få lov til å kansellere GrappleDrag? */
	//Stop_Grapple_Drag();
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
		//GetMoveComponent()->bWallSlowDown = false;
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
		//if (!b) { return; }
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
		// Sticking To Wall	(Stop)

		// Wall Slow Down	
		// Stop

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
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (LinetraceVector * WallDetectionRange), FColor::Yellow, false, 0.f, 0, 4.f);
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
				DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (LinetraceVector * WallDetectionRange), FColor::Red, false, 0.f, 0, 4.f);
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

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (FromActorToWall * 200.f), FColor::White, false, 0.f, 0.f, 6.f);

		const bool b = GetWorld()->LineTraceSingleByChannel(WallHit, GetActorLocation(), GetActorLocation() + (FromActorToWall * 200.f), ECC_Visibility, Params);
		if (b)
		{
			ActorToWall_Length = FVector(GetActorLocation() - WallHit.ImpactPoint).Size();
			PRINTPAR("Length To Wall = %f", ActorToWall_Length);
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
		float InputAngleDirection{ FVector::DotProduct(GetActorRightVector(), InputVector) };
		if (InputAngleDirection > 0.f) { InputAngleToForward *= -1.f; }

		InputDotProdToForward = FVector::DotProduct(GetActorForwardVector(), InputVector);

		//PRINTPAR("INPUT ANGLE FROM ACTOR FORWARD: %f", InputAngleToForward);
		//PRINTPAR("DOT PROD FROM ACTOR FORWARD   : %f", InputDotProdToForward);
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
		PRINTPAR("Move to wall : %f", Move);
	}
}

bool ASteikemannCharacter::IsStickingToWall() const
{
	if (GetMoveComponent().IsValid()) {
		return GetMoveComponent()->bStickingToWall;
	}
	return false;
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


void ASteikemannCharacter::Initial_GrappleHook_Swing()
{
	if (!GrappledActor.IsValid()) { return; }

	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
	Initial_GrappleRopeLength = radius.Size();
	/* If the rope is shorter then the minimum, set it to the minimum length */
	if (Initial_GrappleRopeLength < Min_GrappleRopeLength) { 
		
		/* If player is too close to the grappled actor/object. Adjust playerposition to be at the minimum rope length */
		if (GetMoveComponent()->IsFalling()) {
			FVector Adjustment = radius;
			Adjustment.Normalize();
			Adjustment *= (Min_GrappleRopeLength - Initial_GrappleRopeLength) * -1.f;
			//Adjustment *= (Min_GrappleRopeLength / Initial_GrappleRopeLength) - 1.f;

			PRINTPARLONG("GrappleRope INITIAL Adjustment = %f", Adjustment.Size());
				DrawDebugBox(GetWorld(), GetActorLocation(), FVector(10), FQuat(GetActorRotation()), FColor::Blue, false, 4.f, 0, 5.f);
			SetActorRelativeLocation(GetActorLocation() + Adjustment, false, nullptr, ETeleportType::TeleportPhysics);
				DrawDebugBox(GetWorld(), GetActorLocation(), FVector(10), FQuat(GetActorRotation()), FColor::Red, false, 4.f, 0, 5.f);
		}

		Initial_GrappleRopeLength = Min_GrappleRopeLength; 
	}
	GrappleRopeLength = Initial_GrappleRopeLength;



	FVector currentVelocity = GetCharacterMovement()->Velocity;
	if (currentVelocity.Size() > 0)
	{
		FVector newVelocity = FVector::CrossProduct(radius, (FVector::CrossProduct(currentVelocity, radius)));
		newVelocity.Normalize();

		/* Using Clamp as temporary solution to the max speed */
		float V = (PI * GrappleRopeLength) / GrappleHook_SwingTime;
		V = FMath::Clamp(V, 0.f, GrappleHook_Swing_MaxSpeed);
		newVelocity *= V;

		GetCharacterMovement()->Velocity = newVelocity;
	}
}

void ASteikemannCharacter::Update_GrappleHook_Swing(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }
	//PRINTPAR("GrappleRadiusLength: %f", GrappleRopeLength);

	FVector currentVelocity = GetCharacterMovement()->Velocity;

	/* Vector from Character -> GrappledActor */
	FVector ToGrappledActor = GrappledActor->GetActorLocation() - GetActorLocation();
	float fRadius = ToGrappledActor.Size();
	//PRINTPAR("fRadius: %f", fRadius);

	/* On Ground */
	if (GetCharacterMovement()->IsWalking() || IsJumping()) 
	{
		GrappleRopeLength = ToGrappledActor.Size();
	}

	/*
	* Find length from grappletarget to the ground beneath
	* If the rope is shorter than that distance move the player towards the grappletarget in 2D space
	*/
	{
		/* Find the length from the grappled actor to the ground beneath it. */
		FVector FromGrappledToGround{};
		float LengthFromGrappledToGround{};
		FHitResult Hit{};
		FCollisionQueryParams Params{ "", false, this };
		const bool b = GetWorld()->LineTraceSingleByChannel(Hit, GrappledActor->GetActorLocation(), GrappledActor->GetActorLocation() + FVector::DownVector * GrappleHookRange, ECC_Visibility, Params);
		if (b)
		{
			FromGrappledToGround = Hit.ImpactPoint - GrappledActor->GetActorLocation();
			LengthFromGrappledToGround = FromGrappledToGround.Size();

			/* Drag player towards grappletarget in 2D space */
			FVector ToGrappledActor2D{ FVector(ToGrappledActor.X, ToGrappledActor.Y, 0.f) };
			ToGrappledActor2D.Normalize();

			if (fRadius + GrappleHook_OnGroundMoveTowardsTarget > LengthFromGrappledToGround)
			{
				/* Move the actor towards the grappled target in 2D space */	/* Currently only moves player a flat amount towards the target */
				if (GetMoveComponent()->IsFalling())
				{
					SetActorRelativeLocation(GetActorLocation() + (ToGrappledActor2D * (GrappleHook_OnGroundDragSpeed * DeltaTime * GrappleHook_OnGroundDragSpeed_InAirMultiplier /* BP Value */)), false, nullptr, ETeleportType::TeleportPhysics);
					GrappleRopeLength -= GrappleHook_OnGroundDragSpeed * DeltaTime * GrappleHook_OnGroundDragSpeed_InAirMultiplier/* BP Value */;
				}
				else
				{
					SetActorRelativeLocation(GetActorLocation() + (ToGrappledActor2D * (GrappleHook_OnGroundDragSpeed * DeltaTime)), false, nullptr, ETeleportType::TeleportPhysics);
					GrappleRopeLength -= GrappleHook_OnGroundDragSpeed * DeltaTime;
				}

			}
			if (fRadius < LengthFromGrappledToGround)/* This is the transition between on ground and to in air, with Grapplehook Swing */
			{
				if (GetCharacterMovement()->IsWalking())
				{
					FVector Move = ToGrappledActor; Move.Normalize();
					SetActorRelativeLocation(GetActorLocation() + (Move * (GrappleHook_OnGroundMoveTowardsTarget * DeltaTime)), false, nullptr, ETeleportType::TeleportPhysics);
					GrappleRopeLength -= GrappleHook_OnGroundMoveTowardsTarget;
					GetMoveComponent()->SetMovementMode(MOVE_Falling);
				}
			}


			/* Debug stuff */
			DrawDebugLine(GetWorld(), GrappledActor->GetActorLocation(), GrappledActor->GetActorLocation() + FVector::DownVector * GrappleHookRange, FColor::Black, false, -1, 0, 6.f);
			//PRINTPAR("Length To Ground From GrappleTarget: %f", LengthFromGrappledToGround);
			//PRINTPAR("Length Between Player And GrappleTarget: %f", fRadius);


		}
	}

	/* GrappleSwing IN AIR */
	if (currentVelocity.SizeSquared() > 0 && GetMoveComponent()->IsFalling()) 
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + ToGrappledActor, FColor::Green, false, -1, 0, 4.f);

		/* Adjust actor location to match the initial length from the grappled object */
		if (fRadius > GrappleRopeLength) 
		{
			float L = (fRadius / GrappleRopeLength) - 1.f;
			FVector adjustment = ToGrappledActor * L;
			/* Only adjust location if character IsFalling */
			//if (GetMovementComponent()->IsFalling()) 
			{
				SetActorRelativeLocation(GetActorLocation() + adjustment, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}

		/* New Velocity */	// Is missing: normalizing the vectors before crossproduct or finding the length of the velocity. So that it doesn't exponentially scale
		FVector newVelocity = FVector::CrossProduct(ToGrappledActor, (FVector::CrossProduct(currentVelocity, ToGrappledActor)));
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + newVelocity, FColor::Purple, false, -1, 0, 4.f);

		newVelocity = (currentVelocity.Size() / newVelocity.Size()) * newVelocity;

		/* Sets new velocity if character IsFalling */
		if (GetMovementComponent()->IsFalling() && !IsJumping()) {
			GetCharacterMovement()->Velocity = newVelocity;	// Setter nye velocity
		}

		static float Timer{};
		Timer += DeltaTime;
		//if (Timer > 0.1f)
		//{
			/* Play camera shake */
			float x =		 FMath::Clamp((newVelocity.Size() / 5000.f),	  0.001f, 1.f);
			float falloff =	 FMath::Clamp((cosf((x) * (PI / 2)) * 50.f),		0.f, 50.f);
			//PRINTPAR("Speed: %f", newVelocity.Size());
			//PRINTPAR("Camera shake Falloff: %f", falloff);
			PlayCameraShake(MYShake, falloff);
			Timer = 0.f;
		//}
		//PRINTPAR("CS Timer: %f", Timer);
	}
	
	if (IsAdjustingRopeLength()) {
		AdjustRopeLength(GrappleRope);
	}
}

void ASteikemannCharacter::RotateActor_GrappleHook_Swing(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }

	RotateActorYawToVector(GetVelocity());
	RotateActorPitchToVector(GetVelocity());
	RollActorTowardsLocation(GrappledActor->GetActorLocation());
}

void ASteikemannCharacter::Initial_GrappleHook_Drag(float DeltaTime)
{
	if (!GrappledActor.IsValid()){ return; }


	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();

	if (GrappleDrag_PreLaunch_Timer >= 0) {
		GrappleDrag_PreLaunch_Timer -= DeltaTime;
		GetCharacterMovement()->Velocity *= 0;
		GetCharacterMovement()->GravityScale = 0;
	}
	else {
		bGrapple_Launch = true;
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
		GrappleDrag_CurrentSpeed = 0.f;
	}
}

void ASteikemannCharacter::Update_GrappleHook_Drag(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }
	
	/* If character is on the ground, set the movement mode to Falling */
	if (GetCharacterMovement()->GetMovementName() == "Walking"){
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
	FVector newVelocity = radius;
	newVelocity.Normalize();

	/* Base the time multiplier with the distance to the grappled actor */
	float D = GrappleDrag_Update_TimeMultiplier / ((radius.Size() - GrappleDrag_MinRadiusDistance) / 1000.f);
	D = FMath::Max(D, GrappleDrag_Update_Time_MIN_Multiplier);

	GrappleDrag_CurrentSpeed = FMath::FInterpTo(GrappleDrag_CurrentSpeed, GrappleDrag_MaxSpeed, DeltaTime, D);

	newVelocity *= GrappleDrag_CurrentSpeed;

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + newVelocity, FColor::Red, false, 0, 0, 4.f);

	if (radius.Size() > GrappleDrag_MinRadiusDistance) {
		GetCharacterMovement()->Velocity = newVelocity;
	}
	else {
		bGrapple_Launch = false;
		//bGrappleEnd = true;
		Stop_Grapple_Drag();
	}
}

bool ASteikemannCharacter::IsGrappling() const
{
	return bGrapple_Swing || bGrapple_PreLaunch || bGrapple_Launch;
}

void ASteikemannCharacter::AdjustRopeLength(FVector Rope)
{
	//if (Rope.Size() > Min_GrappleRopeLength)
	{
		//float X_Input = InputVectorRaw.X;

		/* Adjustment of rope in air */
		if (GetMoveComponent()->IsFalling()) 
		{
			float Adjustment = RopeAdjustmentSpeed * InputVectorRaw.X;
			FVector AdjustmentVector = Rope;
			AdjustmentVector.Normalize();

			if (GrappleRopeLength < Min_GrappleRopeLength)
			{
				Adjustment = GrappleRopeLength - Min_GrappleRopeLength;
				GrappleRopeLength = Min_GrappleRopeLength;
			}
			
			AdjustmentVector *= Adjustment;
			SetActorRelativeLocation(GetActorLocation() + AdjustmentVector, false, nullptr, ETeleportType::TeleportPhysics);

			GrappleRopeLength -= Adjustment;
			
			//PRINT("-- ADJUSTING ROPE LENGTH IN AIR --");
			//PRINTPAR("Adjusting rope = %f", Adjustment);

			//bIsAdjustingRopeLength = true;
		}
	}
}

void ASteikemannCharacter::CanBeAttacked()
{
}

void ASteikemannCharacter::PreBasicAttackMoveCharacter(float DeltaTime)
{
	if (bPreBasicAttackMoveCharacter)
	{
		AddActorWorldOffset(AttackDirection * ((PreBasicAttackMovementLength / (1 / BasicAttack_CommonAnticipation_Rate)) * DeltaTime), false, nullptr, ETeleportType::None);
	}
}

void ASteikemannCharacter::SmackAttackMoveCharacter(float DeltaTime)
{
	if (bSmackAttackMoveCharacter)
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
	bScoopAttackMoveCharacter = true;
}

void ASteikemannCharacter::Deactivate_ScoopAttack()
{
	bScoopAttackMoveCharacter = false;
	bHasbeenScoopLaunched = false;
}

void ASteikemannCharacter::Activate_SmackAttack()
{
	bSmackAttackMoveCharacter = true;
}

void ASteikemannCharacter::Deactivate_SmackAttack()
{
	bSmackAttackMoveCharacter = false;
}

void ASteikemannCharacter::Click_Attack()
{
	if (!bAttackPress && bCanAttack) 
	{
		bAttackPress = true;
		
		if (!bAttacking)
		{
			bAttacking = true;

			//SmackAttackMovementLength_Step = (SmackAttackMovementLength) / (1 / SmackAttack_Action_Rate);


			if (bSmackDirectionDecidedByInput)
			{
				AttackDirection = InputVector;
			}
			if (!bSmackDirectionDecidedByInput || InputVector.IsNearlyZero())
			{
				AttackDirection = GetControlRotation().Vector(); 
				AttackDirection.Z = 0; AttackDirection.Normalize();
			}
			RotateActorYawToVector(AttackDirection);

			Start_Attack();
		}
	}
}

void ASteikemannCharacter::UnClick_Attack()
{
	bAttackPress = false;
}

void ASteikemannCharacter::Start_Attack_Pure()
{
	bPreBasicAttackMoveCharacter = true;
}

void ASteikemannCharacter::Stop_Attack()
{
	PRINTLONG("Can attack again");
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
	PRINTLONG("Deactivate Attack");
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
	PRINTLONG("Click GroundPound");
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
	PRINTLONG("LAUNCH GroundPound");
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
	GroundPoundCollider->SetHiddenInGame(false);

	bIsGroundPounding = false;
	bCanGroundPound = true;
}


void ASteikemannCharacter::GroundPoundLand(const FHitResult& Hit)
{
	PRINTLONG("GroundPound LAND");

	// Detach GP collider on Hit.ImpactLocation
	USceneComponent* RootComp = GetRootComponent();
	if (GroundPoundCollider->GetAttachParent() == RootComp)
	{
		GroundPoundCollider->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	GroundPoundCollider->SetWorldLocation(Hit.ImpactPoint, false, nullptr, ETeleportType::TeleportPhysics);
	GroundPoundCollider->SetGenerateOverlapEvents(true);
	GroundPoundCollider->SetHiddenInGame(false);

	GroundPoundCollider->SetSphereRadius(MaxGroundPoundRadius, true);

	GetWorldTimerManager().SetTimer(THandle_GPReset, this, &ASteikemannCharacter::Deactivate_GroundPound, GroundPoundExpandTime);
}

void ASteikemannCharacter::Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor)
{
	PRINTPARLONG("GroundPound Attack ON: %s", *OtherActor->GetName());

	const float diff = GetActorLocation().Z - OtherActor->GetActorLocation().Z;
	const float range = 40.f;
	const bool b = diff < range || diff > -range;
	PRINTPARLONG("DIFF: %f", diff);

	if (b)
	{
		FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
		Direction = Direction.GetSafeNormal2D();
		float angle = FMath::DegreesToRadians(45.f);
		Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);

		float LengthToOtherActor = FVector(GetActorLocation() - OtherActor->GetActorLocation()).Size();
		float Multiplier = /*1.f - */(LengthToOtherActor / MaxGroundPoundRadius);
		PRINTPARLONG("Multiplier: %f", Multiplier);

		//OtherInterface->Receive_GroundPound_Pure(Direction, GP_LaunchStrength * Multiplier);
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

void ASteikemannCharacter::GrappleHook_Swing_RotateCamera(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }


	FVector con = GetControlRotation().Vector();
	//PRINTPAR("Con: %s", *con.ToString());
	con.Z = 0.f;
	con.Normalize();
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (con * 20000), FColor::Yellow, false, 0, 0, 4.f);

	FVector D = con.RotateAngleAxis(90.f, FVector(0, 0, 1));
	D.Z = 0.f;
	D.Normalize();
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (D * 20000), FColor::Red, false, 0, 0, 4.f);

	FVector vel = GetCharacterMovement()->Velocity;
	vel.Z = 0.f;
	vel.Normalize();

	float dotproduct = FVector::DotProduct(con, vel);
	float angle = FMath::RadiansToDegrees(acosf(dotproduct));

	float rightDotproduct = FVector::DotProduct(D, vel);

	if (rightDotproduct <= 0.f) { angle *= -1.f; }
	//PRINTPAR("angle: %f", angle);
	
	float YawRotate = FMath::FInterpTo(0.f, angle, DeltaTime, 3.f);
	AddControllerYawInput(YawRotate);
}
