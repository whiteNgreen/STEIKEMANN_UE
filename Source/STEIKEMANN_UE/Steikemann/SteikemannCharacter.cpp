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

	Component_Audio = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	Component_Audio->SetupAttachment(RootComponent);

	Component_Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	Component_Niagara->SetupAttachment(RootComponent);

	AttackCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("AttackCollider"));
	AttackCollider->SetupAttachment(RootComponent);
	AttackCollider->SetGenerateOverlapEvents(false);
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
		AttackCollider->SetRelativeScale3D(FVector(0, 0, 0));
	}
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
		InputVector.Normalize();
		FRotator Rot = GetControlRotation();
		InputVector = InputVector.RotateAngleAxis(Rot.Yaw, FVector::UpVector);
	}


	/*		Resets Rotation Pitch and Roll		*/
	if (IsFalling() || MovementComponent->IsWalking()) {
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
	if (IsDashing()) { RotateActorYawToVector(DeltaTime, DashDirection); }



	/* Resetting Wall Jump & Ledge Grab when player is on the ground */
	if (MovementComponent->IsWalking()) {
		WallJump_NonStickTimer = 0.f;

		//ResetWallJumpAndLedgeGrab();
	}



	/*		Wall Jump & LedgeGrab		*/
	if ((MovementComponent->IsFalling()) && !IsGrappling() && bOnWallActive) 
	{
		Do_OnWallMechanics(DeltaTime);
	}
	else if (!bOnWallActive)
	{
		if (WallJump_NonStickTimer < WallJump_MaxNonStickTimer)	{
			WallJump_NonStickTimer += DeltaTime;

			ResetWallJumpAndLedgeGrab();
		}
		else {
			bOnWallActive = true;
		}
	}


	/*		Activate grapple targeting		*/
	if (!IsGrappling())
	{
		LineTraceToGrappleableObject();
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
		GrappleHookMesh->SetVisibility(false);
	}
	/*		Grapplehook			*/
	if ((bGrapple_Swing && !bGrappleEnd) || (bGrapple_PreLaunch && !bGrappleEnd)) 
	{
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
				PRINTLONG("Removing completed TempNiagaraComponent");
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
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::StopJumping).bConsumeInput = true;


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
	PlayerInputComponent->BindAction("GrappleHook_Swing", IE_Pressed, this,	  &ASteikemannCharacter::Start_Grapple_Swing);
	PlayerInputComponent->BindAction("GrappleHook_Swing", IE_Released, this,  &ASteikemannCharacter::Stop_Grapple_Swing);
		/* Grapplehook DRAG */
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Pressed, this,	  &ASteikemannCharacter::Start_Grapple_Drag);
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Released, this,	  &ASteikemannCharacter::Stop_Grapple_Drag);

	/* Attack - SmackAttack */
	PlayerInputComponent->BindAction("SmackAttack", IE_Pressed, this, &ASteikemannCharacter::Click_Attack);
	PlayerInputComponent->BindAction("SmackAttack", IE_Released, this, &ASteikemannCharacter::UnClick_Attack);
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
	if (GrappledActor.IsValid())
	{
		bGrapple_PreLaunch = true;
		IGrappleTargetInterface::Execute_Hooked(GrappledActor.Get());
		if (JumpCurrentCount == 2) { JumpCurrentCount--; }
	}
}

void ASteikemannCharacter::Stop_Grapple_Drag()
{
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
		MovementComponent->Traced_GroundFriction = Hit.PhysMaterial->Friction;
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
	/* Skal regne ut Velocity.Z ogs� basere falloff p� hvor stor den er */
	UGameplayStatics::PlayWorldCameraShake(GetWorld(), shake, Camera->GetComponentLocation() + FVector(0, 0, 1), 0.f, 10.f, falloff);
}

void ASteikemannCharacter::MoveForward(float value)
{
	InputVectorRaw.X = value;

	if (IsStickingToWall()) { return; }
	if (IsLedgeGrabbing()) { return; }
	if (IsCrouchSliding()) { return; }

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
	if (IsGrappling() && GetMovementComponent()->IsFalling()) { return; }

	bPressedJump = true;
	bJumping = true;	
	bAddJumpVelocity = (CanJump() || CanDoubleJump());
	
}


void ASteikemannCharacter::StopJumping()
{
	bPressedJump = false;
	bActivateJump = false;
	bJumping = false;
	bAddJumpVelocity = false;
	bCanEdgeJump = false;
	bCanPostEdgeJump = false;
	if (MovementComponent.IsValid()){
		MovementComponent->bWallJump = false;
		MovementComponent->bLedgeJump = false;
	}
	ResetJumpState();
}

void ASteikemannCharacter::CheckJumpInput(float DeltaTime)
{
	JumpCurrentCountPreJump = JumpCurrentCount;

	if (GetCharacterMovement())
	{
		if (bPressedJump)
		{
			/* If player is LedgeGrabbing */
			if ((MovementComponent->bLedgeGrab) && MovementComponent->IsFalling())
			{
				JumpCurrentCount = 1;
				bAddJumpVelocity = MovementComponent->LedgeJump(LedgeLocation);
				ResetWallJumpAndLedgeGrab();
				Activate_Jump();
				return;
			}

			/* If player is sticking to a wall */
			if (( MovementComponent->bStickingToWall || bFoundStickableWall ) && MovementComponent->IsFalling())
			{
				JumpCurrentCount = 1;
				//bAddJumpVelocity = true;
				bAddJumpVelocity = MovementComponent->WallJump(Wall_Normal);
				Activate_Jump();
				return;
			}


			/* If player walks off edge with no jumpcount and the post edge timer is valid */
			if (GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0 && PostEdge_JumpTimer < PostEdge_JumpTimer_Length) 
			{
				//PRINTLONG("POST EDGE JUMP");
				bCanPostEdgeJump = true;
				JumpCurrentCount++;
				Activate_Jump();
				bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
				return;
			}
			/* If player walks off edge with no jumpcount after post edge timer is valid */
			if (GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0)
			{
				bCanEdgeJump = true;
				JumpCurrentCount += 2;
				Activate_Jump();
				bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
				return;
			}

			// If this is the first jump and we're already falling,
			// then increment the JumpCount to compensate.
			const bool bFirstJump = JumpCurrentCount == 0;
			if (bFirstJump && GetCharacterMovement()->IsFalling())
			{
				Activate_Jump();
				JumpCurrentCount++;
			}
			if (CanDoubleJump() && GetCharacterMovement()->IsFalling())
			{
				Activate_Jump();
				GetCharacterMovement()->DoJump(bClientUpdating);
				JumpCurrentCount++;
			}

			const bool bDidJump = CanJump() && GetCharacterMovement()->DoJump(bClientUpdating);
			if (bDidJump)
			{
				Activate_Jump();
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
	return bAddJumpVelocity && bJumping;
	//return bAddJumpVelocity && bJumping && bActivateJump;
}

bool ASteikemannCharacter::IsFalling() const
{
	if (!MovementComponent.IsValid()) { return false; }
	return  MovementComponent->MovementMode == MOVE_Falling  && ( !IsGrappling() && !IsDashing() && !IsStickingToWall() && !IsOnWall() && !IsJumping() );
}

bool ASteikemannCharacter::IsOnGround() const
{
	if (!MovementComponent.IsValid()) { return false; }
	return MovementComponent->MovementMode == MOVE_Walking;
}

void ASteikemannCharacter::Start_Crouch()
{
	bPressedCrouch = true;

	if (MovementComponent->IsWalking())
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

		MovementComponent->Initiate_CrouchSlide(InputVector);

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
	bOnWallActive						= false;

	MovementComponent->bWallSlowDown	= false;
	MovementComponent->bStickingToWall	= false;
	bFoundStickableWall					= false;
	InputAngleToForward					= 0.f;
	InputDotProdToForward				= 1.f;

	//WallJump_NonStickTimer				= 0.f;

	bIsLedgeGrabbing					= false;
	bFoundLedge							= false;
	MovementComponent->bLedgeGrab		= false;
	MovementComponent->bLedgeJump		= false;
}

void ASteikemannCharacter::ResetActorRotationPitchAndRoll(float DeltaTime)
{
	FRotator Rot = GetActorRotation();
	AddActorLocalRotation(FRotator(Rot.Pitch * -1.f, 0.f, Rot.Roll * -1.f));
}

void ASteikemannCharacter::RotateActorYawToVector(float DeltaTime, FVector AimVector)
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

void ASteikemannCharacter::RotateActorPitchToVector(float DeltaTime, FVector AimVector)
{
	FVector Aim{ AimVector };
	Aim.Normalize();

	float Pitch{ FMath::RadiansToDegrees(asinf(Aim.Z)) };

	PRINTPAR("Pitch Angle: %f", Pitch);

	SetActorRotation( FRotator{ Pitch, GetActorRotation().Yaw, 0.f }, ETeleportType::TeleportPhysics );
}

void ASteikemannCharacter::RotateActorYawPitchToVector(float DeltaTime, FVector AimVector)
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

void ASteikemannCharacter::RollActorTowardsLocation(float DeltaTime, FVector Location)
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

		/* Interface functions to the actor that is aimed at */
		if (!IsGrappling()) {
			IGrappleTargetInterface::Execute_Targeted(Grappled.Get());
		}

		/* Interface function to un_target the actor we aimed at */
		if (Grappled != GrappledActor && GrappledActor.IsValid())
		{
			IGrappleTargetInterface::Execute_UnTargeted(GrappledActor.Get());
		}
	}
	else {		/* Untarget if Grappled returned NULL */
		if (GrappledActor.IsValid()){
			IGrappleTargetInterface::Execute_UnTargeted(GrappledActor.Get());
		}
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
			MovementComponent->Bounce(Hit.ImpactNormal);
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
	if (!bDashClick && !bDash && DashCounter == 1)
	{
		bDash = true;
		DashCounter--;
		Activate_Dash();
		MovementComponent->Start_Dash(Pre_DashTime, DashTime, DashLength, DashDirection);
	}
	bDashClick = true;
}

void ASteikemannCharacter::Stop_Dash()
{
	bDashClick = false;
}

bool ASteikemannCharacter::IsDashing() const
{
	return bDash;
}

void ASteikemannCharacter::Do_OnWallMechanics(float DeltaTime)
{
	/*		Detect Wall		 */
	static float PostWallJumpTimer{};	//	Post WallJump Timer		 
	if (MovementComponent->bWallJump || MovementComponent->bLedgeJump)
	{
		static float TimerLength{};
		if (MovementComponent->bWallJump)	TimerLength = 0.25;
		if (MovementComponent->bLedgeJump)	TimerLength = 0.75;
		PostWallJumpTimer += DeltaTime;
		if (PostWallJumpTimer > TimerLength) // Venter 0.25 sekunder f�r den skal kunne fortsette med � s�ke etter n�re vegger 
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
		if (GetActorLocation().Z < LedgeHit.ImpactPoint.Z)
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
		if (MovementComponent->bStickingToWall)
		{
			if (JumpCurrentCount == 2) { JumpCurrentCount = 1; }	// Resets DoubleJump
			WallJump_StickTimer += DeltaTime;
			if (WallJump_StickTimer > WallJump_MaxStickTimer)
			{
				bOnWallActive = false;
				WallJump_NonStickTimer = 0.f;
				MovementComponent->ReleaseFromWall(Wall_Normal);
				return;
			}
		}
		else { WallJump_StickTimer = 0.f; }

		/*		Rotating and moving actor to wall	  */
		if (MovementComponent->bStickingToWall || MovementComponent->bWallSlowDown)
		{
			SetActorLocation_WallJump(DeltaTime);
			RotateActorYawToVector(DeltaTime, Wall_Normal * -1.f);
			RotateActorPitchToVector(DeltaTime, Wall_Normal * -1.f);

			CalcAngleFromActorForwardToInput();

			if (InputDotProdToForward < -0.5f)	// Drop down
			{
				bOnWallActive = false;
				WallJump_NonStickTimer = 0.f;
				MovementComponent->ReleaseFromWall(WallHit.Normal);
				return;
			}
		}
		// Wall Slow Down	(On way down)
		if (MovementComponent->bWallSlowDown)
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

	/*		Dette fungerer p� flate vegger, men kommer til og f�re til problemer p� cylindriske- / mer avanserte- former	 */
		/* Burde kj�re ekstra sjekker for � se om det er andre flater i n�rheten som passer bedre */
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
		/* Hvordan h�ndtere hj�rner? 
				Sjekke om veggen som er truffet har en passende vinkel i forhold til playercontroller? 
				Er det den eneste veggen vi treffer? (TArray kanskje for � samle alle n�re vegger) */
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

		PRINTPAR("INPUT ANGLE FROM ACTOR FORWARD: %f", InputAngleToForward);
		PRINTPAR("DOT PROD FROM ACTOR FORWARD   : %f", InputDotProdToForward);
	}
	else { InputAngleToForward = 0.f; }
}

void ASteikemannCharacter::SetActorLocation_WallJump(float DeltaTime)
{
	float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius() + 1.f;
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
	if (MovementComponent.IsValid()) {
		return MovementComponent->bStickingToWall;
	}
	return false;
}

bool ASteikemannCharacter::IsOnWall() const
{
	if (MovementComponent.IsValid()) {
		return MovementComponent->bWallSlowDown;
	}
	return false;
}

bool ASteikemannCharacter::Do_LedgeGrab(float DeltaTime)
{
	bIsLedgeGrabbing = true;

	MovementComponent->Start_LedgeGrab();
	MoveActorToLedge(DeltaTime);
	RotateActorYawToVector(DeltaTime, Wall_Normal * -1.f);
	RotateActorPitchToVector(DeltaTime, Wall_Normal * -1.f);

	CalcAngleFromActorForwardToInput();

	/* Tmp DebugArms */
	DrawDebugArms(InputAngleToForward);


	if (InputDotProdToForward < -0.5f)	// Drop down
	{
		bOnWallActive = false;
		WallJump_NonStickTimer = 0.f;
		MovementComponent->ReleaseFromWall(WallHit.Normal);
		return false;
	}

	return true;
}


void ASteikemannCharacter::Initial_GrappleHook_Swing()
{
	if (!GrappledActor.IsValid()) { return; }

	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
	GrappleRadiusLength = radius.Size();

	FVector currentVelocity = GetCharacterMovement()->Velocity;
	if (currentVelocity.Size() > 0)
	{
		FVector newVelocity = FVector::CrossProduct(radius, (FVector::CrossProduct(currentVelocity, radius)));
		newVelocity.Normalize();

		/* Using Clamp as temporary solution to the max speed */
		float V = (PI * GrappleRadiusLength) / GrappleHook_SwingTime;
		V = FMath::Clamp(V, 0.f, GrappleHook_Swing_MaxSpeed);
		newVelocity *= V;

		GetCharacterMovement()->Velocity = newVelocity;
	}
}

void ASteikemannCharacter::Update_GrappleHook_Swing(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }
	PRINTPAR("GrappleRadiusLength: %f", GrappleRadiusLength);

	FVector currentVelocity = GetCharacterMovement()->Velocity;

	if (currentVelocity.SizeSquared() > 0) {
		FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
		float fRadius = radius.Size();
		if (GetCharacterMovement()->IsWalking() || IsJumping()) {
			GrappleRadiusLength = radius.Size();
		}
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + radius, FColor::Green, false, -1, 0, 4.f);

		/* Adjust actor location to match the initial length from the grappled object */
		if (fRadius > GrappleRadiusLength) {
			float L = (fRadius / GrappleRadiusLength) - 1;
			FVector adjustment = radius * L;
			/* Only adjust location if character IsFalling */
			if (GetMovementComponent()->IsFalling()) {
				SetActorRelativeLocation(GetActorLocation() + adjustment, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}

		/* New Velocity */
		FVector newVelocity = FVector::CrossProduct(radius, (FVector::CrossProduct(currentVelocity, radius)));
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + newVelocity, FColor::Purple, false, -1, 0, 4.f);

		/* New Velocity in relation to the downward axis */	// NOT IN USE
		//FVector backWards = FVector::CrossProduct(radius, (FVector::CrossProduct(FVector::DownVector, radius)));
			//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + backWards, FColor::Blue, false, -1, 0, 4.f);

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
			PRINTPAR("Speed: %f", newVelocity.Size());
			PRINTPAR("Camera shake Falloff: %f", falloff);
			PlayCameraShake(MYShake, falloff);
			Timer = 0.f;
		//}
		PRINTPAR("CS Timer: %f", Timer);
	}
}

void ASteikemannCharacter::RotateActor_GrappleHook_Swing(float DeltaTime)
{
	if (!GrappledActor.IsValid()) { return; }

	RotateActorYawToVector(DeltaTime, GetVelocity());
	RotateActorPitchToVector(DeltaTime, GetVelocity());
	RollActorTowardsLocation(DeltaTime, GrappledActor->GetActorLocation());
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
		bGrappleEnd = true;
	}
}

bool ASteikemannCharacter::IsGrappling() const
{
	return bGrapple_Swing || bGrapple_PreLaunch || bGrapple_Launch;
}

void ASteikemannCharacter::Click_Attack()
{
	if (!bAttackPress && bCanAttack) 
	{
		bAttackPress = true;
		
		if (!bAttacking)
		{
			bAttacking = true;

			/* Selve angrepet*/
			{
				PRINTLONG("Attacking");
				bCanAttack = false;
					AttackCollider->SetHiddenInGame(false);	// For Debugging
				AttackCollider->SetGenerateOverlapEvents(true);
				AttackCollider->SetRelativeScale3D(AttackColliderScale);

				GetWorldTimerManager().SetTimer(THandle_AttackDuration, this, &ASteikemannCharacter::Deactivate_Attack, TimeAttackIsActive, false);
				GetWorldTimerManager().SetTimer(THandle_AttackReset,	this, &ASteikemannCharacter::Stop_Attack,		TimeBetweenAttacks, false);
			}
		}
	}
}

void ASteikemannCharacter::UnClick_Attack()
{
	bAttackPress = false;
}

void ASteikemannCharacter::Stop_Attack()
{
	PRINTLONG("Can attack again");
	bCanAttack = true;
}

void ASteikemannCharacter::Deactivate_Attack()
{
	PRINTLONG("Deactivate Attack");
	bAttacking = false;
	AttackCollider->SetHiddenInGame(true);	// For Debugging
	AttackCollider->SetGenerateOverlapEvents(false);
	AttackCollider->SetRelativeScale3D(FVector(0, 0, 0));
}

void ASteikemannCharacter::OnAttackColliderBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this)
	{

		IAttackInterface* Interface = Cast<IAttackInterface>(OtherActor);
		if (Interface) {
			// Burde sjekke om den kan bli angrepet i det hele tatt. 
			const bool b{ Interface->GetCanBeSmackAttacked() };
			
			if (b)
			{
				//PRINTPARLONG("Attacking: %s", *OtherActor->GetName());

				FVector Direction{ OtherActor->GetActorLocation() - GetActorLocation() };
				Direction = Direction.GetSafeNormal2D();
				float angle = FMath::DegreesToRadians(45.f);
				Direction = (cosf(angle) * Direction) + (sinf(angle) * FVector::UpVector);
				Interface->Recieve_SmackAttack_Pure(Direction, 1000.f);
			}
		}
	}
}

void ASteikemannCharacter::Do_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength)
{
}

void ASteikemannCharacter::Recieve_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength)
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

	RotateActorYawToVector(DeltaTime, GrappledActor->GetActorLocation() - GetActorLocation());
	RotateActorPitchToVector(DeltaTime, GrappledActor->GetActorLocation() - GetActorLocation());
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
