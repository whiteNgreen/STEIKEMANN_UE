// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../DebugMacros.h"

#include "SteikemannCharacter.generated.h"

#define GRAPPLE_HOOK ECC_GameTraceChannel1

enum GamepadType
{
	Xbox,
	Playstation,
	MouseandKeyboard
};

UCLASS()
class STEIKEMANN_UE_API ASteikemannCharacter : public ACharacter, 
	public IGrappleTargetInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASteikemannCharacter(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class USpringArmComponent* CameraBoom{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UCameraComponent* Camera{ nullptr };

	//class USteikemannCharMovementComponent* MovementComponent{ nullptr };
	TWeakObjectPtr<class USteikemannCharMovementComponent> MovementComponent;

#pragma region Gamepads

	UPROPERTY(BlueprintReadWrite)
		bool bCanChangeGamepad{};
	UPROPERTY(BlueprintReadWrite)
		bool bDontChangefromXboxPad{};
	/* Timer to ensure that it doesn't accidentaly change gamepad type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gamepads")
		float Gamepad_ChangeTimerLength{ 1.f }; 
	float Gamepad_ChangeTimer{};


	UPROPERTY(EditAnywhere, Category = "Gamepads|Dualshock")
		float DS_LeftStickDrift{ 0.06f };	
	UPROPERTY(EditAnywhere, Category = "Gamepads|Dualshock")
		float DS_RightStickDrift{ 0.08f };

	GamepadType CurrentGamepadType{ MouseandKeyboard };

	void ListenForControllerChange(bool isConnected, int32 useless, int32 uselessIndex);
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Controller Events")
		void OnControllerConnection();

	UFUNCTION(BlueprintCallable)
	void AnyKey(FKey key);
	UFUNCTION(BlueprintCallable)
	void AnyKeyRelease(FKey key);
#pragma endregion //Gamepads

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Variables", meta = (AllowPrivateAcces = "true"))
	bool bSlipping;

	void DetectPhysMaterial();

	/* The Raw InputVector */
	FVector InputVectorRaw;
	/* Input vector rotated to match the playercontrollers rotation */
	FVector InputVector;

#pragma region Basic_Movement
public:/* ------------------- Basic Movement ------------------- */

	UPROPERTY(EditAnywhere, Category = "Movement|Walk/Run", meta = (AllowPrivateAcces = "true"))
	float TurnRate{ 50.f };

	void MoveForward(float value);
	void MoveRight(float value);
	void TurnAtRate(float rate);
	void LookUpAtRate(float rate);

	void MoveForwardDualshock(float value);
	void MoveRightDualshock(float value);
	void TurnAtRateDualshock(float rate);
	void LookUpAtRateDualshock(float rate);

	bool bActivateJump{};
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
		bool bJumping{};
	UPROPERTY(BlueprintReadOnly)
		bool bCanEdgeJump{};
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
		bool bAddJumpVelocity{};
	/* How long the jump key can be held to add upwards velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
		float fJumpTimerMax UMETA(DisplayName = "JumpHoldTimer") { 0.2f };
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
		float fJumpTimer{};

	void Jump() override;
	void JumpDualshock();
	void StopJumping() override;
	void CheckJumpInput(float DeltaTime) override;


	bool CanDoubleJump() const;
	bool IsJumping() const;

	bool IsFalling() const;
	bool IsOnGround() const;

#pragma endregion //Basic_Movement

#pragma region Bounce
	/* ------------------------ Bounce --------------------- */

	//UPROPERTY(BlueprintReadOnly, Category = "Movement|Bounce")
	bool bBounceClick{};
	bool bBounce{};

	/* The max length player can activate bounce */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Bounce")
		float BounceCheckLength{ 100.f };

	void Bounce();
	void Stop_Bounce();
#pragma endregion //Bounce

#pragma region Dash
	/* ------------------------ Dash --------------------- */

	bool bDashClick{};
	bool bDash{};

	FVector DashDirection{};

	/* How long the dash action lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
		float DashTime{ 0.5f };
	/* How far the character will dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
		float DashLength{ 1000.f };


	void Dash();
	void Stop_Dash();

	bool IsDashing() const;

#pragma endregion //Dash

#pragma region Wall Jump
	/* ------------------------ Wall Jump --------------------- */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Jump")
		float WallJump_DetectionLength UMETA(DisplayName = "Detection Length") { 100.f };
	/* The maximum time the character can hold on to the wall they stick to during wall jump */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Jump")
		float WallJump_MaxStickTimer UMETA(DisplayName = "Max Sticking Time") { 1.f };
	float WallJump_StickTimer{};

	/* Time until character can stick to wall again */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Wall Jump")
		float WallJump_MaxNonStickTimer UMETA(DisplayName = "No Stick Timer") { 1.f };
	float WallJump_NonStickTimer{};


	bool bStickingToWall{};
	bool bFoundStickableWall{};
	bool bCanStickToWall{ true };
	FVector StickingSpot{};

	FVector Wall_Normal{};
	bool WallJump_DetectNearbyWall();

	bool IsStickingToWall() const;
	bool IsOnWall() const;

#pragma endregion //Wall Jump

#pragma region GrappleHook
public: /* ------------------------ Grapplehook --------------------- */
		/*                     GrappleTargetInterface                 */
	void Targeted() {}
	virtual void TargetedPure() override {}

	void UnTargeted() {}
	virtual void UnTargetedPure() override {}

	void Hooked() {}
	virtual void HookedPure() override {}

	void UnHooked() {}
	virtual void UnHookedPure() override {}


	/*                    Native Variables and functions             */
	UPROPERTY(BlueprintReadOnly)
		TWeakObjectPtr<AActor> GrappledActor{ nullptr };


	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_Available;
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHookRange{ 2000.f };
	
	/* The onscreen aiming location */
	FVector2D AimingLocation;
	/* Shows the debug aiming reticle */
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Targeting")
		bool bShowAimingLocaiton_Debug{};
	/* The added percentage of the screens height that is added to the aiming location. A higher number turns it closer to the
		middle, with a lower number further up. 0 directly to the middle */
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Targeting")
		float GrappleAimYChange_Base UMETA(DisplayName = "GrappleAimYDifference") { 4.f };
	float GrappleAimYChange{};

	bool LineTraceToGrappleableObject();
	UFUNCTION()
	void Start_Grapple_Swing();
	void Stop_Grapple_Swing();
	UFUNCTION()
	void Start_Grapple_Drag();
	void Stop_Grapple_Drag();


	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_Swing;
	
	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_PreLaunch;
	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_Launch;
	
	/* Time it takes for half a rotation around the GrappledActor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Swing", meta = (AllowPrivateAcces = "true"))
		float GrappleHook_SwingTime{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Swing", meta = (AllowPrivateAcces = "true"))
		float GrappleHook_Swing_MaxSpeed{ 2000.f };
	/* Initial length between actor and grappled object */
	float GrappleRadiusLength{};

	/* Grapplehook swing */
	UFUNCTION(BlueprintCallable)
	void Initial_GrappleHook_Swing();
	void Update_GrappleHook_Swing();
	void GrappleHook_Drag_RotateCamera(float DeltaTime);

	/* How long the player will be held in the air before being launched towards the grappled actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_PreLaunch_Timer_Length UMETA(DisplayName = "PreLaunch Timer")  { 0.25f };

	float GrappleDrag_PreLaunch_Timer{};

	/* Interpolation speed of the camera rotation during grapplehook Drag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_InterpSpeed UMETA(DisplayName = "Interpolation Speed") { 3.f };

	/* Pitch adjustment for the camera rotation during the Pre_Launch of Grapple Drag  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_PitchPoint UMETA(DisplayName = "Pitch Point") { 20.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_MaxSpeed UMETA(DisplayName = "Max Speed") { 2000.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_MinRadiusDistance UMETA(DisplayName = "Min Radius Distance") { 50.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_Update_TimeMultiplier UMETA(DisplayName = "Time Multiplier") { 2.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_Update_Time_MIN_Multiplier UMETA(DisplayName = "Minimum Time Multiplier") { 2.f };

	float GrappleDrag_CurrentSpeed{};

	/* Grapplehook drag during swing */
	UFUNCTION(BlueprintCallable)
	void Initial_GrappleHook_Drag(float DeltaTime);
	void Update_GrappleHook_Drag(float DeltaTime);
	void GrappleHook_Swing_RotateCamera(float DeltaTime);

	UPROPERTY(BlueprintReadWrite)
	bool bGrappleEnd{};

	UFUNCTION(BlueprintCallable)
	bool IsGrappling() const;
#pragma endregion //GrappleHook
};
