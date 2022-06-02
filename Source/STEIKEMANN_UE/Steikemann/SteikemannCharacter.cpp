// Fill out your copyright notice in the Description page of Project Settings.


#include "../Steikemann/SteikemannCharacter.h"
#include "../Steikemann/SteikemannCharMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Kismet/GameplayStatics.h"



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
}

// Called when the game starts or when spawned
void ASteikemannCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	MovementComponent = Cast<USteikemannCharMovementComponent>(GetCharacterMovement());

	GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
}

// Called every frame
void ASteikemannCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DetectPhysMaterial();

	if (!IsGrappling())
	{
		LineTraceToGrappleableObject();
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
	}

	//if (IsGrappling())
	//if ((bGrapple_Swing && !bGrappleEnd)) 
	if ((bGrapple_Swing && !bGrappleEnd) || (bGrapple_PreLaunch && !bGrappleEnd)) 
	{
		if (!bGrapple_PreLaunch) {
			Update_GrappleHook_Swing();
			bGrapple_Launch = false;
		}
		else {
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

	/* Basic Movement */
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &ASteikemannCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &ASteikemannCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this,		&APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this,	&ASteikemannCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this,		&APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up/Down Gamepad", this,		&ASteikemannCharacter::LookUpAtRate);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ASteikemannCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ASteikemannCharacter::StopJumping);


	/* GrappleHook */
	PlayerInputComponent->BindAction("GrappleHook_Swing", IE_Pressed, this,	  &ASteikemannCharacter::Start_Grapple_Swing);
	PlayerInputComponent->BindAction("GrappleHook_Swing", IE_Released, this,  &ASteikemannCharacter::Stop_Grapple_Swing);

	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Pressed, this,	  &ASteikemannCharacter::Start_Grapple_Drag);
	PlayerInputComponent->BindAction("GrappleHook_Drag", IE_Released, this,	  &ASteikemannCharacter::Stop_Grapple_Drag);


}

void ASteikemannCharacter::Start_Grapple_Swing()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Yellow, FString::Printf(TEXT("Start grapple Swing")));
	if (bGrapple_Available)
	{
		bGrapple_Swing = true;
		Initial_GrappleHook_Swing();
		if (GrappledActor)
		{
			IGrappleTargetInterface::Execute_Hooked(GrappledActor);
		}
	}
}

void ASteikemannCharacter::Stop_Grapple_Swing()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("End grapple Swing")));
	bGrapple_Swing = false;
	if (GrappledActor && IsGrappling())
	{
		IGrappleTargetInterface::Execute_UnTargeted(GrappledActor);
	}
}

void ASteikemannCharacter::Start_Grapple_Drag()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Silver, FString::Printf(TEXT("Start Grapple Drag")));
	bGrapple_PreLaunch = true;
	if (GrappledActor)
	{
		IGrappleTargetInterface::Execute_Hooked(GrappledActor);
	}
}

void ASteikemannCharacter::Stop_Grapple_Drag()
{
	GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, FString::Printf(TEXT("End Grapple Drag")));
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

	//DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0, 0, 2.f);

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

void ASteikemannCharacter::MoveForward(float Value)
{
	float movement = Value;
	if (bSlipping)
		movement *= 0.1;

	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, movement);
	}
}

void ASteikemannCharacter::MoveRight(float Value)
{
	float movement = Value;
	if (bSlipping)
		movement *= 0.1;

	if ((Controller != nullptr) && (Value != 0.0f))
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

void ASteikemannCharacter::Jump()
{
	bPressedJump = true;
	JumpKeyHoldTime = 0.0f;
}

void ASteikemannCharacter::StopJumping()
{
	bPressedJump = false;
	ResetJumpState();
}

void ASteikemannCharacter::CheckJumpInput(float DeltaTime)
{
	JumpCurrentCountPreJump = JumpCurrentCount;

	if (GetCharacterMovement())
	{
		if (bPressedJump)
		{
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

void ASteikemannCharacter::Initial_GrappleHook_Swing()
{
	//FVector radius = GrappleHit.GetActor()->GetActorLocation() - GetActorLocation();
	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
	GrappleRadiusLength = radius.Size();

	//FVector newVelocity = FVector::CrossProduct(radius, (FVector::CrossProduct(FVector::DownVector, radius)));
	FVector newVelocity = FVector::CrossProduct(radius, (FVector::CrossProduct(GetCharacterMovement()->Velocity, radius)));
	FVector Velocity = GetCharacterMovement()->Velocity;
	float L = Velocity.Size();
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, FString::Printf(TEXT("L: %f"), L));
	newVelocity = (GrapplingHook_InitialBoost / newVelocity.Size()) * newVelocity;

	GetCharacterMovement()->Velocity = newVelocity;
}

void ASteikemannCharacter::Update_GrappleHook_Swing()
{
	FVector currentVelocity = GetCharacterMovement()->Velocity;
	if (currentVelocity.Size() > 0) {
		//FVector radius = GrappleHit.GetActor()->GetActorLocation() - GetActorLocation();
		FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();
		float fRadius = radius.Size();
		GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Black, FString::Printf(TEXT("Radius Length: %f"), GrappleRadiusLength - fRadius));
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + radius, FColor::Green, false, -1, 0, 4.f);

		/* Adjust actor location to match the initial length from the grappled object */
		if (fRadius > GrappleRadiusLength) {
			//float L = GrappleRadiusLength / fRadius;
			float L = (fRadius / GrappleRadiusLength) - 1;
			FVector adjustment = radius * L;
			SetActorRelativeLocation(GetActorLocation() + adjustment, false, nullptr, ETeleportType::TeleportPhysics);

			//{	// Debug lines and text
			//	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + adjustment, FColor::Yellow, false, -1, 0, 8.f);
			//	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Black, FString::Printf(TEXT("Adjustment Length: %f"), L));
			//	GEngine->AddOnScreenDebugMessage(-1, 0, FColor::Black, FString::Printf(TEXT("Adjustment: %s"), *adjustment.ToString()));
			//}
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
	//FVector radius = GrappleHit.GetActor()->GetActorLocation() - GetActorLocation();
	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();

	if (GrappleDrag_PreLaunch_Timer >= 0) {
		GrappleDrag_PreLaunch_Timer -= DeltaTime;
		GetCharacterMovement()->Velocity *= 0;
	}
	else {
		bGrapple_Launch = true;
		GrappleDrag_PreLaunch_Timer = GrappleDrag_PreLaunch_Timer_Length;
		GrappleDrag_CurrentSpeed = GrappleDrag_Initial_Speed;
	}
}

void ASteikemannCharacter::Update_GrappleHook_Drag(float DeltaTime)
{
	//FVector radius = GrappleHit.GetActor()->GetActorLocation() - GetActorLocation();
	FVector radius = GrappledActor->GetActorLocation() - GetActorLocation();

	// Sett velocity til å gå mot grappled object. 
	FVector newVelocity = (GrappleDrag_CurrentSpeed / radius.Size()) * radius;
	DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + newVelocity, FColor::Red, false, 0, 0, 4.f);
	GrappleDrag_CurrentSpeed += GrappleDrag_Acceleration_Speed;

	if (radius.Size() > 50) {
		GetCharacterMovement()->Velocity = newVelocity;
	}
	else {
		bGrapple_Launch = false;
		bGrappleEnd = true;
	}
}

bool ASteikemannCharacter::IsGrappling()
{
	return bGrapple_Swing || bGrapple_PreLaunch || bGrapple_Launch;
}
