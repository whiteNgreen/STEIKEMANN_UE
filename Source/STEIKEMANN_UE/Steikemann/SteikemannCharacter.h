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

#pragma region Basic_Movement
public:/*                      Basic Movement                           */

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

	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
		bool bJumping{};
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
#pragma endregion //Basic_Movement


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
	AActor* GrappledActor { nullptr };

	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_Available;
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHookRange{ 2000.f };

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
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Swing", meta = (AllowPrivateAcces = "true"))
		//float GrapplingHook_InitialBoost{ 1000.f };
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_PreLaunch_Timer_Length UMETA(DisplayName = "PreLaunch Timer")  { 0.25f };
	float GrappleDrag_PreLaunch_Timer{};

	/* Interpolation speed of the camera rotation during grapplehook Drag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_Camera_InterpSpeed UMETA(DisplayName = "Interpolation Speed") { 3.f };

	/* Pitch adjustment for the camera rotation during the Pre_Launch of Grapple Drag  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_Camera_PitchPoint UMETA(DisplayName = "Pitch Point") { 20.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_MaxSpeed UMETA(DisplayName = "Max Speed") { 2000.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_MinRadiusDistance UMETA(DisplayName = "Min Radius Distance") { 50.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_Update_TimeMultiplier UMETA(DisplayName = "Time Multiplier") { 2.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
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
	bool IsGrappling();
#pragma endregion //GrappleHook
};
