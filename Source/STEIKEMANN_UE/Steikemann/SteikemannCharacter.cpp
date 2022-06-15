// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharacter.h"
#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"

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

	/* Gamepad Ticks */
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

	//PRINTPAR("Velocity: %f", GetCharacterMovement()->Velocity.Size());
	PRINTPAR("Gravity: %f", GetCharacterMovement()->GravityScale);

	DetectPhysMaterial();

	if (!IsGrappling())
	{
		LineTraceToGrappleableObject();
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
	}

	
	/* Jump */
	if (bJumping){
		if (JumpKeyHoldTime < fJumpTimerMax){
			JumpKeyHoldTime += DeltaTime;
		}
		else{
			bAddJumpVelocity = false;
		}
	}


	/* Grapplehook */
	if ((bGrapple_Swing && !bGrappleEnd) || (bGrapple_PreLaunch && !bGrappleEnd)) 
	{
		if (!bGrapple_PreLaunch) {
			//GrappleHook_Swing_RotateCamera(DeltaTime);
			Update_GrappleHook_Swing();
			bGrapple_Launch = false;
		}
		else {
			GrappleHook_Drag_RotateCamera(DeltaTime);
			if (!bGrapple_Launch) {
				Initial_GrappleHook_Drag(DeltaTime);
			}
			else {
				Update_GrappleHook_Drag(DeltaTime);
			}
		}
	}
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
	PlayerInputComponent->BindAxis("Look Up/Down PS4", this,		&ASteikemannCharacter::LookUpAtRateDualshock);
	PlayerInputComponent->BindAxis("Turn Right/Left PS4", this,	&ASteikemannCharacter::TurnAtRateDualshock);

		/* Jump */
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASteikemannCharacter::Jump).bConsumeInput = true;
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::StopJumping).bConsumeInput = true;
	PlayerInputComponent->BindAction("Jump Dualshock", IE_Pressed, this, &ASteikemannCharacter::JumpDualshock).bConsumeInput = true;
	PlayerInputComponent->BindAction("Jump Dualshock", IE_Released, this, &ASteikemannCharacter::StopJumping).bConsumeInput = true;

	/* Bounce */
	PlayerInputComponent->BindAction("Bounce", IE_Pressed, this, &ASteikemannCharacter::Bounce).bConsumeInput = true;
	PlayerInputComponent->BindAction("Bounce", IE_Released, this, &ASteikemannCharacter::Stop_Bounce).bConsumeInput = true;


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
		if (GrappledActor)
		{
			IGrappleTargetInterface::Execute_Hooked(GrappledActor);
		}

		if (JumpCurrentCount == 2) { JumpCurrentCount--; }
	}
}

void ASteikemannCharacter::Stop_Grapple_Swing()
{
	bGrapple_Swing = false;
	if (GrappledActor && IsGrappling())
	{
		IGrappleTargetInterface::Execute_UnTargeted(GrappledActor);
	}
}

void ASteikemannCharacter::Start_Grapple_Drag()
{
	bGrapple_PreLaunch = true;
	if (GrappledActor)
	{
		IGrappleTargetInterface::Execute_Hooked(GrappledActor);
	}
	if (JumpCurrentCount == 2) { JumpCurrentCount--; }
}

void ASteikemannCharacter::Stop_Grapple_Drag()
{
	bGrapple_PreLaunch = false;
	bGrapple_Launch = false;
	bGrapple_Swing = false;
	bGrappleEnd = false;
	if (GrappledActor && IsGrappling())
	{
		IGrappleTargetInterface::Execute_UnTargeted(GrappledActor);
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

void ASteikemannCharacter::MoveForward(float value)
{
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
	if (IsGrappling()) { return; }

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
	bJumping = false;
	bAddJumpVelocity = true;
	bCanEdgeJump = false;
	ResetJumpState();
}

void ASteikemannCharacter::CheckJumpInput(float DeltaTime)
{
	JumpCurrentCountPreJump = JumpCurrentCount;

	if (GetCharacterMovement())
	{
		if (bPressedJump)
		{
			/* If player walks off edge with no jumpcount */
			if (GetCharacterMovement()->IsFalling() && JumpCurrentCount == 0)
			{
				bCanEdgeJump = true;
				JumpCurrentCount += 2;
				bAddJumpVelocity = GetCharacterMovement()->DoJump(bClientUpdating);
			}

			// If this is the first jump and we're already falling,
			// then increment the JumpCount to compensate.
			const bool bFirstJump = JumpCurrentCount == 0;
			if (bFirstJump && GetCharacterMovement()->IsFalling())
			{
				JumpCurrentCount++;
			}
			if (CanDoubleJump() && GetCharacterMovement()->IsFalling())
			{
				GetCharacterMovement()->DoJump(bClientUpdating);
				JumpCurrentCount++;
			}

			const bool bDidJump = CanJump() && GetCharacterMovement()->DoJump(bClientUpdating);
			if (bDidJump)
			{
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
}

/* Aiming system for grapplehook */
bool ASteikemannCharacter::LineTraceToGrappleableObject()
{
	FVector Loc{};
	FVector Direction{};
	AActor* Grappled{ nullptr };

	FVector2D ViewPortSize = GEngine->GameViewport->Viewport->GetSizeXY();

	FVector2D ViewPortLocation = FVector2D(ViewPortSize.X / 2, ViewPortSize.Y = 0);
	float Step = ViewPortLocation.X / 10;
	float BaseX = (ViewPortLocation.X / 2) + Step;

	FVector2D View = ViewPortLocation + FVector2D(Step*2, 0);

	for (int i = 0; i < 12; i++)
	{
		FHitResult Hit;
		View.X = ViewPortLocation.X + (Step * 2);

		for (int k = 0; k < 3; k++)
		{
			UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(GetWorld(), 0), View, Loc, Direction);
			View += FVector2D(-Step*2, 0);

			FCollisionQueryParams Params = FCollisionQueryParams(FName(""), false, this);
			bGrapple_Available = GetWorld()->LineTraceSingleByChannel(Hit, Loc, Loc + (Direction * GrappleHookRange), GRAPPLE_HOOK, Params);
			if (bGrapple_Available) { break; }
		}
		View += FVector2D(0, Step);

		
		if (bGrapple_Available)
		{
			if (GrappledActor && GrappledActor != Hit.GetActor())
			{
				IGrappleTargetInterface::Execute_UnTargeted(GrappledActor);
				break;
			}

			Grappled = Hit.GetActor();
			break;

		}
		/* Hvordan skrive Interface casts til andre c++ klasser */
		//IGrappleTargetInterface* Interface = Cast<IGrappleTargetInterface>(Hit.GetActor());
		//if (Interface)
		//{
		//	Interface->TargetedPure();
		//	Interface->Execute_Targeted(Hit.GetActor());
		//}
	}
	if (Grappled && !IsGrappling())
	{
		IGrappleTargetInterface::Execute_Targeted(Grappled);
	}
	else if (!bGrapple_Available && (!Grappled || Grappled != GrappledActor))
	{
		if (GrappledActor)
		{
			IGrappleTargetInterface::Execute_UnTargeted(GrappledActor);
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
			PRINTLONG("Bounce");
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

void ASteikemannCharacter::Initial_GrappleHook_Swing()
{
	if (!GrappledActor) { return; }

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

void ASteikemannCharacter::Update_GrappleHook_Swing()
{
	if (!GrappledActor) { return; }

	FVector currentVelocity = GetCharacterMovement()->Velocity;
	if (currentVelocity.Size() > 0) {
		FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
		float fRadius = radius.Size();
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + radius, FColor::Green, false, -1, 0, 4.f);

		/* Adjust actor location to match the initial length from the grappled object */
		if (fRadius > GrappleRadiusLength) {
			float L = (fRadius / GrappleRadiusLength) - 1;
			FVector adjustment = radius * L;
			SetActorRelativeLocation(GetActorLocation() + adjustment, false, nullptr, ETeleportType::TeleportPhysics);
		}

		/* New Velocity */
		FVector newVelocity = FVector::CrossProduct(radius, (FVector::CrossProduct(currentVelocity, radius)));
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + newVelocity, FColor::Purple, false, -1, 0, 4.f);
		/* New Velocity in relation to the downward axis */
		FVector backWards = FVector::CrossProduct(radius, (FVector::CrossProduct(FVector::DownVector, radius)));
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + backWards, FColor::Blue, false, -1, 0, 4.f);

		newVelocity = (currentVelocity.Size() / newVelocity.Size()) * newVelocity;

		GetCharacterMovement()->Velocity = newVelocity;	// Setter nye velocity
	}
}

void ASteikemannCharacter::Initial_GrappleHook_Drag(float DeltaTime)
{
	if (!GrappledActor){ return; }


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
	if (!GrappledActor) { return; }
	
	if (GetCharacterMovement()->GetMovementName() == "Walking")
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
	FVector newVelocity = radius;
	newVelocity.Normalize();

	/* Base the time multiplier with the distance to the grappled actor */
	float D = GrappleDrag_Update_TimeMultiplier / ((radius.Size() - GrappleDrag_MinRadiusDistance) / 1000.f);
	D = FMath::Max(D, GrappleDrag_Update_Time_MIN_Multiplier);
	PRINTPAR("D: %f", D);

	GrappleDrag_CurrentSpeed = FMath::FInterpTo(GrappleDrag_CurrentSpeed, GrappleDrag_MaxSpeed, DeltaTime, D);
	PRINTPAR("Speed: %f", GrappleDrag_CurrentSpeed);

	newVelocity *= GrappleDrag_CurrentSpeed;

		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + newVelocity, FColor::Red, false, 0, 0, 4.f);


	if (radius.Size() > GrappleDrag_MinRadiusDistance) {
		
		GetCharacterMovement()->Velocity = newVelocity;
	}
	else {
		bGrapple_Launch = false;
		bGrappleEnd = true;
		PRINTPARLONG("Drag OutSpeed: %f", GrappleDrag_CurrentSpeed);
	}
}

bool ASteikemannCharacter::IsGrappling()
{
	return bGrapple_Swing || bGrapple_PreLaunch || bGrapple_Launch;
}

void ASteikemannCharacter::GrappleHook_Drag_RotateCamera(float DeltaTime)
{
	if (!GrappledActor) { return; }

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

void ASteikemannCharacter::GrappleHook_Swing_RotateCamera(float DeltaTime)
{
	if (!GrappledActor) { return; }

	// Finn ut av quaterniums FQuat for rotering. Vinkler fungerer ikke ordentlig. 

	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();

	FVector currentVelocity = GetCharacterMovement()->Velocity;
	FRotator Vel = currentVelocity.Rotation();
	//FQuat V = Vel.Quaternion();

	FVector con = GetControlRotation().Vector();
	FRotator controllerRotation = con.Rotation();
	//FQuat C = controllerRotation.Quaternion();

	float e = FVector::DotProduct(currentVelocity, con);
	float t = currentVelocity.Size() * con.Size();
	float dotprod = acosf(e / t) * (180 / PI);
	PRINTPAR("dotprod: %f", dotprod);
	
	float YawTo = Vel.Yaw - controllerRotation.Yaw;
	PRINTPAR("YawTo: %f", YawTo);

	//if (YawTo < 0) { dotprod *= -1; }

	//float YawRotate = FMath::FInterpTo(0.f, YawTo, DeltaTime, 10.f);
	float YawRotate = FMath::FInterpTo(0.f, dotprod, DeltaTime, 10.f);
	AddControllerYawInput(YawRotate);
}
