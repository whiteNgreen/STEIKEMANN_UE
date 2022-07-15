// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharacter.h"
#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"
#include "Components/PoseableMeshComponent.h"

/* Gamepad support */
#include "RawInput.h"
#include "RawInputFunctionLibrary.h"
#include "IInputDeviceModule.h"
#include "IInputDevice.h"
#include "GenericPlatform/GenericApplicationMessageHandler.h"


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

	/* Gamepad support */
	FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &ASteikemannCharacter::ListenForControllerChange);
}

/* Gamepad support */
void ASteikemannCharacter::ListenForControllerChange(bool isConnected, int32 useless, int32 uselessIndex)
{
	IRawInput* RawInput = static_cast<IRawInput*>(static_cast<FRawInputPlugin*>(&FRawInputPlugin::Get())->GetRawInputDevice().Get());
	RawInput->QueryConnectedDevices();

	OnControllerConnection();
}

void ASteikemannCharacter::AnyKey(FKey key)
{
	//PRINTPARLONG("%s", *key.GetDisplayName().ToString());

	Gamepad_ChangeTimer = 0.0f;
	FString s = key.GetDisplayName().ToString();
	FString t;
	for (int i = 0; i < s.Len(); i++)
	{
		if (i > 6) { break; }

		t += s[i];
	}
	t = t.ToLower();

	GamepadType incommingPad{};
	FString c;
	if (t == "gamepad"){
		incommingPad = Xbox;
		c = "Xbox";
		//bDontChangefromXboxPad = true;
	}
	else if (t == "generic"){
		incommingPad = Playstation;
		c = "Playstation";
	}
	else  {
		incommingPad = MouseandKeyboard;
		c = "Mouse and Keyboard";
	}
	
	if (incommingPad != CurrentGamepadType && bCanChangeGamepad)
	{
		CurrentGamepadType = incommingPad;	
		//PRINTPARLONG("Change gamepad to : %s", *c);
	}
}

void ASteikemannCharacter::AnyKeyRelease(FKey key)
{
	Gamepad_ChangeTimer = 0.0f;
	FString s = key.GetDisplayName().ToString();
	FString t;
	for (int i = 0; i < s.Len(); i++)
	{
		if (i > 6) { break; }

		t += s[i];
	}
	t = t.ToLower();

	GamepadType incommingPad{};
	FString c;
	if (t == "gamepad") {
		incommingPad = Xbox;
		c = "Xbox";
		//bDontChangefromXboxPad = false;
	}
	else if (t == "generic") {
		incommingPad = Playstation;
		c = "Playstation";
	}
	else {
		incommingPad = MouseandKeyboard;
		c = "Mouse and Keyboard";
	}
}

// Called when the game starts or when spawned
void ASteikemannCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	FCoreDelegates::OnControllerConnectionChange.AddUObject(this, &ASteikemannCharacter::ListenForControllerChange);
	/* Gamepad support */
	IRawInput* RawInput = static_cast<IRawInput*>(static_cast<FRawInputPlugin*>(&FRawInputPlugin::Get())->GetRawInputDevice().Get());
	if (RawInput != nullptr)
	{
		RawInput->QueryConnectedDevices();
		OnControllerConnection();
	}
	
	//APlayerController* player = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	//player->EnableInput(player);

	MovementComponent = Cast<USteikemannCharMovementComponent>(GetCharacterMovement());

	GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;


}

// Called every frame
void ASteikemannCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Testing around with applying PlayWorldCameraShake each frame
	//static float time{};
	//time += DeltaTime;
	//UGameplayStatics::PlayWorldCameraShake(GetWorld(), MYShake, GetActorLocation(), 0.f, 2000.f, 5.f);

	/* Rotate Inputvector to match the playercontroller */
	{
		FRotator Rot = GetControlRotation();
		InputVector = InputVector.RotateAngleAxis(Rot.Yaw, FVector(0, 0, 1));
	}

	/*		Gamepad Ticks		*/
	//bDontChangefromXboxPad ? PRINT("True") : PRINT("False");
	switch (CurrentGamepadType)
	{
	case Xbox:
		//PRINT("Xbox");
		break;
	case Playstation:
		//PRINT("Dualshock");
		break;
	case MouseandKeyboard:
		//PRINT("Mouse and Keyboard");
		break;

	default:
		break;
	}
	//PRINTPAR("%f", Gamepad_ChangeTimer);
	if (Gamepad_ChangeTimer < Gamepad_ChangeTimerLength) {
		Gamepad_ChangeTimer += DeltaTime;
		bCanChangeGamepad = false;
	}
	else {
		bCanChangeGamepad = true;
	}

	/*		Resets Rotation Pitch and Roll		*/
	if (IsFalling() || MovementComponent->IsWalking()) {
		ResetActorRotationPitchAndRoll(DeltaTime);
	}
	//IsFalling() ? PRINT("Falling: True") : PRINT("Falling: False");

	DetectPhysMaterial();


	/*		Activate grapple targeting		*/
	if (!IsGrappling())
	{
		LineTraceToGrappleableObject();
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
		GrappleHookMesh->SetVisibility(false);
	}


	
	/*		Jump		*/
	if (bJumping){
		//static float Timer{};
		//if (Timer > 0.1f) {
		//	Timer = 0.f;
		//	bJumping = false;
		//	bAddJumpVelocity = false;
		//}
		//Timer += DeltaTime;

		if (JumpKeyHoldTime < fJumpTimerMax){
			JumpKeyHoldTime += DeltaTime;
		}
		else{
			bAddJumpVelocity = false;
		}
	}
	PostEdge_JumpTimer += DeltaTime;
	if (GetCharacterMovement()->IsWalking()) { PostEdge_JumpTimer = 0.f; }
	//IsJumping() ? PRINT("Jumping: True") : PRINT("Jumping: False");

	

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


	//PRINTPAR("Velocity: %f", GetVelocity().Size());
		/* Hjelper å se hvor dashen går hen */
		//DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (DashDirection * 3000), FColor::Red, false, 0, 0, 5.f);
		//PRINTPAR("Rot: %s", *GetControlRotation().ToString());




	/*		Wall Jump		*/
	if (MovementComponent->IsFalling() && !IsGrappling()) {
		/* Venter 0.25 sekunder før den skal kunne fortsette med å søke etter nære vegger */
		static float PostWallJumpTimer{};
		if (MovementComponent->bWallJump) {
			PostWallJumpTimer += DeltaTime;
			if (PostWallJumpTimer > 0.25f) {
				PostWallJumpTimer = 0.f;
				bFoundStickableWall = WallJump_DetectNearbyWall();
			}
		}
		else {
			PostWallJumpTimer = 0.f;
			bFoundStickableWall = WallJump_DetectNearbyWall();
		}
		PRINTPAR("PostWallTimer %f", PostWallJumpTimer);
	}
	if (!bFoundStickableWall) {
		bCanStickToWall = false;
		WallJump_StickTimer = 0.f;
		MovementComponent->bStickingToWall = false;
	};
	
	bStickingToWall = MovementComponent->bStickingToWall;
	if (bStickingToWall) 
	{
		if (JumpCurrentCount == 2) { JumpCurrentCount = 1; }	// Resets DoubleJump
		if (WallJump_StickTimer < WallJump_MaxStickTimer) {
			WallJump_StickTimer += DeltaTime;
		}
		else {
			bStickingToWall = false;
			bCanStickToWall = false;

		}
	}
	else { WallJump_StickTimer = 0.f; }

	if (!bCanStickToWall)
	{
		if (WallJump_NonStickTimer < WallJump_MaxNonStickTimer) {
			WallJump_NonStickTimer += DeltaTime;
		}
		else{
			bCanStickToWall = true;
		}
	}
	else { WallJump_NonStickTimer = 0.f; }

	PRINTPAR("StickTimer: %f", WallJump_StickTimer);
	PRINTPAR("NonStickTimer: %f", WallJump_NonStickTimer);


	bFoundStickableWall ? PRINT("bFoundStickableWall: True") : PRINT("bFoundStickableWall: False");
	bStickingToWall ? PRINT("bStickingToWall: True") : PRINT("bStickingToWall: False");
	bCanStickToWall ? PRINT("bCanStickToWall: True") : PRINT("bCanStickToWall: False");

}

// Called to bind functionality to input
void ASteikemannCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAction("AnyKey", IE_Pressed, this, &ASteikemannCharacter::AnyKey).bConsumeInput = true;
	PlayerInputComponent->BindAction("AnyKey", IE_Released, this, &ASteikemannCharacter::AnyKeyRelease).bConsumeInput = true;


	/* Basic Movement */
		/* Movement control */
			/* Gamepad and Keyboard */
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ASteikemannCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ASteikemannCharacter::MoveRight);
			/* Dualshock */
	PlayerInputComponent->BindAxis("Move Forward Dualshock", this, &ASteikemannCharacter::MoveForwardDualshock);
	PlayerInputComponent->BindAxis("Move Right / Left PS4", this, &ASteikemannCharacter::MoveRightDualshock);
		/* Looking control */
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this,		&APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this,		&APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this,	&ASteikemannCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up/Down Gamepad", this,		&ASteikemannCharacter::LookUpAtRate);
			/* Dualshock */
	//PlayerInputComponent->BindAxis("Look Up/Down PS4", this,		&ASteikemannCharacter::LookUpAtRateDualshock);
	//PlayerInputComponent->BindAxis("Turn Right/Left PS4", this,	&ASteikemannCharacter::TurnAtRateDualshock);
		/* Jump */
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASteikemannCharacter::Jump).bConsumeInput = true;
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::StopJumping).bConsumeInput = true;
	//PlayerInputComponent->BindAction("Jump Dualshock", IE_Pressed, this, &ASteikemannCharacter::JumpDualshock).bConsumeInput = true;
	//PlayerInputComponent->BindAction("Jump Dualshock", IE_Released, this, &ASteikemannCharacter::StopJumping).bConsumeInput = true;


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
	/* Skal regne ut Velocity.Z også basere falloff på hvor stor den er */
	UGameplayStatics::PlayWorldCameraShake(GetWorld(), shake, Camera->GetComponentLocation() + FVector(0, 0, 1), 0.f, 10.f, falloff);
}

void ASteikemannCharacter::MoveForward(float value)
{
	InputVectorRaw.X = value;

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

void ASteikemannCharacter::MoveForwardDualshock(float value)
{
	float V = (value - 0.5f) * 2;
	if (CurrentGamepadType == Playstation && (V >= DS_LeftStickDrift || V <= -DS_LeftStickDrift))
	{
		//PRINT("Playstation Forward");
		MoveForward(V);
	}
}

void ASteikemannCharacter::MoveRight(float value)
{
	InputVectorRaw.Y = value;

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

void ASteikemannCharacter::MoveRightDualshock(float value)
{
	float V = (value - 0.5f) * 2;
	if (CurrentGamepadType == Playstation && (V >= DS_LeftStickDrift || V <= -DS_LeftStickDrift))
	{
		//PRINT("Playstation Right");
		MoveRight(V);
	}
}

void ASteikemannCharacter::TurnAtRateDualshock(float rate)
{
	float V = (rate - 0.5f) * 2;
	if (CurrentGamepadType == Playstation && (V >= 0.05f || V <= -0.05f)) {
		TurnAtRate(V);
		//PRINTPAR("Turnright: %f", rate);
	}
}

void ASteikemannCharacter::LookUpAtRateDualshock(float rate)
{
	float V = (rate - 0.5f) * 2;
	if (CurrentGamepadType == Playstation && (V >= DS_RightStickDrift || V <= -DS_RightStickDrift)) {
		LookUpAtRate(V);
		//PRINTPAR("Lookup: %f", rate);
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

void ASteikemannCharacter::Jump()
{
	/* Don't Jump if player is Grappling */
	if (IsGrappling() && GetMovementComponent()->IsFalling()) { return; }
	//PRINTLONG("Jumping");

	bPressedJump = true;
	bJumping = true;	
	bAddJumpVelocity = (CanJump() || CanDoubleJump());
	
}

void ASteikemannCharacter::JumpDualshock()
{
	if (CurrentGamepadType == Playstation) {
		Jump();
	}
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
			/* If player is sticking to a wall */
			if (( MovementComponent->bStickingToWall || WallJump_DetectNearbyWall() ) && MovementComponent->IsFalling())
			{
				JumpCurrentCount = 1;
				bAddJumpVelocity = true;
				Activate_Jump();
				MovementComponent->WallJump(Wall_Normal);
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

void ASteikemannCharacter::ResetActorRotationPitchAndRoll(float DeltaTime)
{
	FRotator Rot = GetActorRotation();
	AddActorLocalRotation(FRotator(Rot.Pitch * -1.f, 0.f, Rot.Roll * -1.f));
}

void ASteikemannCharacter::RotateActorYawToVector(float DeltaTime, FVector AimVector)
{
	FVector Aim = AimVector;
	Aim.Normalize();
	FVector Forward = GetActorForwardVector();
	FVector Right = GetActorRightVector();
	FVector Up = FVector::UpVector;


	/*		Yaw Rotation		*/
	FVector AimXY = Aim;
	AimXY.Z = 0.f;
	AimXY.Normalize();

	float YawDotProduct = FVector::DotProduct(AimXY, Forward);
	float Yaw = FMath::RadiansToDegrees(acosf(YawDotProduct));

	/*		Check if yaw is to the right or left		*/
	float RightDotProduct = FVector::DotProduct(AimXY, Right);
	if (RightDotProduct < 0.f) { Yaw *= -1.f; }

	FRotator Rot{ 0.f, Yaw, 0.f };
	AddActorLocalRotation(Rot);
}

void ASteikemannCharacter::RotateActorYawPitchToVector(float DeltaTime, FVector AimVector)
{
	FVector Velocity = GetVelocity();	Velocity.Normalize();
	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;


	/*		Yaw Rotation		*/
	FVector VelocityXY = Velocity;
	VelocityXY.Z = 0.f;
	VelocityXY.Normalize();

	float YawDotProduct = FVector::DotProduct(VelocityXY, Forward);
	float Yaw = FMath::RadiansToDegrees(acosf(YawDotProduct));

	/*		Check if yaw is to the right or left		*/
	float RightDotProduct = FVector::DotProduct(VelocityXY, Right);
	if (RightDotProduct < 0.f) { Yaw *= -1.f; }



	/*		Pitch Rotation		*/
	FVector VelocityPitch = FVector(1.f, 0.f, Velocity.Z);
	VelocityPitch.Normalize();
	FVector ForwardPitch = FVector(1.f, 0.f, Forward.Z);
	ForwardPitch.Normalize();

	float PitchDotProduct = FVector::DotProduct(VelocityPitch, ForwardPitch);
	float Pitch = FMath::RadiansToDegrees(acosf(PitchDotProduct));

	/*		Check if pitch is up or down		*/
	float PitchDirection = Forward.Z - Velocity.Z;
	if (PitchDirection > 0.f) { Pitch *= -1.f; }


	/*		Adding Yaw and Pitch rotation to actor		*/
	FRotator Rot{ Pitch, Yaw, 0.f };
	SetActorRotation(Rot);
}

void ASteikemannCharacter::RollAroundPoint(float DeltaTime, FVector Point)
{
	/*		Roll Rotation		*/
	FVector TowardsPoint = Point - GetActorLocation();
	TowardsPoint.Normalize();

	FVector right = GetActorRightVector();
	float RollDotProduct = FVector::DotProduct(TowardsPoint, right);
	float Roll = FMath::RadiansToDegrees(acosf(RollDotProduct)) - 90.f;
	Roll *= -1.f;

	/*		Add Roll rotation to actor		*/
	AddActorLocalRotation(FRotator(0, 0, Roll));
}

/* Aiming system for grapplehook */
bool ASteikemannCharacter::LineTraceToGrappleableObject()
{
	//PRINT("LinetraceToGrappleableObject");

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
	/* Linetrace through the entire screen to get all grappletargets on screen */
	//for (int i{ 0 }; i <= 16; i++)
	//{
	//	DeprojectScreenLocation.Y = 0;

	//	for (int j{ 0 }; j <= 9; j++)
	//	{
	//		UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(GetWorld(), 0), DeprojectScreenLocation, DeprojectWorldLocation, DeprojectDirection);

	//		const bool b = GetWorld()->LineTraceMultiByChannel(MultiHit, DeprojectWorldLocation, DeprojectWorldLocation + (DeprojectDirection * GrappleHookRange), GRAPPLE_HOOK, Params);
	//		traces++;

	//		if (MultiHit.Num() > 0) 
	//		{ 
	//			for (const auto& it : MultiHit) {
	//				OnScreenActors.AddUnique(it.GetActor());
	//			}
	//		}
	//		if (b) { PRINT("Block hit is True"); }

	//		DeprojectScreenLocation.Y += Step;
	//	}

	//	DeprojectScreenLocation.X += Step;
	//}
	//PRINTPAR("Traces: %i", traces);

	TWeakObjectPtr<AActor> Grappled{ nullptr };
	
	/* Adjusting the GrappleAimYChange based on the playercontrollers pitch */
	FVector BackwardControllerPitch = GetControlRotation().Vector() * -1.f;
	BackwardControllerPitch *= FVector(0, 0, 1);

	/* Adjusts the aiminglocation based on controller pitch */
	//GrappleAimYChange = GrappleAimYChange_Base + (GrappleAimYChange_Base * (BackwardControllerPitch.Z));
	//if (GrappleAimYChange != 0.f) {
	//	AimingLocation = ViewPortSize / 2 - FVector2D(0.f, ViewPortSize.Y / GrappleAimYChange);
	//}
	//else {
	//	AimingLocation = ViewPortSize / 2;
	//}

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
	//PRINTPAR("Hits: %i", OnScreenActors.Num());

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

bool ASteikemannCharacter::WallJump_DetectNearbyWall()
{
	bool bHit{};
	FHitResult Hit;
	FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);

	/* Raytrace in a total of 8 directions around the player */
	
	/* Raytrace first in the Forward/Backwards axis and Right/Left */
	FVector Forward = GetControlRotation().Vector();
		Forward.Z = 0.f;
		Forward.Normalize();
	for (int i = 0; i < 4; i++)
	{
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Forward * WallJump_DetectionLength), FColor::Yellow, false, 0.f, 0, 4.f);
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + (Forward * WallJump_DetectionLength), ECC_Visibility, Params);
		Forward = Forward.RotateAngleAxis(90, FVector(0, 0, 1));
		if (bHit) { 
			StickingSpot = Hit.ImpactPoint; 
			Wall_Normal = Hit.Normal; 
			return bHit; 
		}
	}


	/* Then do raytrace of the 45 degree angle between the 4 previous axis */
	Forward = Forward.RotateAngleAxis(45.f, FVector(0, 0, 1));
	for (int i = 0; i < 4; i++)
	{
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + (Forward * WallJump_DetectionLength), FColor::Red, false, 0.f, 0, 4.f);
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), GetActorLocation() + (Forward * WallJump_DetectionLength), ECC_Visibility, Params);
		Forward = Forward.RotateAngleAxis(90, FVector(0, 0, 1));
		if (bHit) {
			StickingSpot = Hit.ImpactPoint;
			Wall_Normal = Hit.Normal;
			return bHit;
		}
	}

	//if (bHit) { StickingSpot = Hit.ImpactPoint; return bHit; }
	return bHit;
}

bool ASteikemannCharacter::IsStickingToWall() const
{
	return bStickingToWall;
}

bool ASteikemannCharacter::IsOnWall() const
{
	return MovementComponent->bWallSlowDown;
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

	RotateActorYawPitchToVector(DeltaTime, GetVelocity());
	RollAroundPoint(DeltaTime, GrappledActor->GetActorLocation());
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

	RotateActorYawPitchToVector(DeltaTime, GrappledActor->GetActorLocation());
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
