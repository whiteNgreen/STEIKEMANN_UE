// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../DebugMacros.h"
#include "Camera/CameraShakeBase.h"
#include "SteikeAnimInstance.h"

#include "SteikemannCharacter.generated.h"

#define GRAPPLE_HOOK ECC_GameTraceChannel1

class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;


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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	class UPoseableMeshComponent* GrappleHookMesh{ nullptr };


	TWeakObjectPtr<class USteikemannCharMovementComponent> MovementComponent;

	USteikeAnimInstance* SteikeAnimInstance{ nullptr };

	void AssignAnimInstance(USteikeAnimInstance* AnimInstance) { SteikeAnimInstance = AnimInstance; }
	USteikeAnimInstance* GetAnimInstance() const { return SteikeAnimInstance; }

#pragma region Audio
	UPROPERTY(EditAnywhere, Category = "Audio")
		UAudioComponent* Component_Audio{ nullptr };


#pragma endregion //Audio

#pragma region ParticleEffects

	/* ------------------- Particle Effects ------------------- */
	UPROPERTY(EditAnywhere, Category = "Particle Effects")
		UNiagaraComponent* Component_Niagara{ nullptr };

	/* Temporary niagara components created when main component is busy */
	TArray<UNiagaraComponent*> TempNiagaraComponents;

	#pragma region Landing
		/* ------------------- PE: Landing ------------------- */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|Land")
			UNiagaraSystem* NS_Land{ nullptr };

		UFUNCTION(BlueprintCallable)
			void NS_Land_Implementation(const FHitResult& Hit);

		/* The amount of particles that will spawn determined by the characters landing velocity, times this multiplier */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|Land")
			float NSM_Land_ParticleAmount		UMETA(DisplayName = "Particle Amount Multiplier") { 0.5f };
		/* The speed of the particles will be determined by the characters velocity when landing, times this multiplier */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|Land")
			float NSM_Land_ParticleSpeed		UMETA(DisplayName = "Particle Speed Multiplier") { 0.5f };

		#pragma endregion //Landing

	#pragma region OnWall
		/* ------------------- PE: OnWall ------------------- */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|WallJump")
			UNiagaraSystem* NS_WallSlide{ nullptr };
		/* The amount of particles per second the system should emit */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|WallJump")
			float NS_WallSlide_ParticleAmount	UMETA(DisplayName = "WallSlide ParticleAmount") { 1000.f };
	#pragma endregion //OnWall


#pragma endregion //ParticleEffects


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/* The Raw InputVector */
	FVector InputVectorRaw;
	/* Input vector rotated to match the playercontrollers rotation */
	FVector InputVector;

#pragma region Slipping

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Variables", meta = (AllowPrivateAcces = "true"))
	bool bSlipping;

	void DetectPhysMaterial();

#pragma endregion //Slipping

#pragma region Camera
	/* ------------------- Camera Shakes ------------------- */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TSubclassOf<UCameraShakeBase> MYShake;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CameraShakes|Jump")
		TSubclassOf<UCameraShakeBase> Jump_Land;

	UFUNCTION(BlueprintCallable)
		void PlayCameraShake(TSubclassOf<UCameraShakeBase> shake, float falloff);

#pragma endregion //Camera

#pragma region Basic_Movement
public:/* ------------------- Basic Movement ------------------- */

	UPROPERTY(EditAnywhere, Category = "Movement|Walk/Run", meta = (AllowPrivateAcces = "true"))
	float TurnRate{ 50.f };

	void MoveForward(float value);
	void MoveRight(float value);
	void TurnAtRate(float rate);
	void LookUpAtRate(float rate);


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
	/* How long after walking off an edge the player is still allowed to jump */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump")
		float PostEdge_JumpTimer_Length{ 0.3f };
	float PostEdge_JumpTimer{};
	bool bCanPostEdgeJump{};

	void Landed(const FHitResult& Hit) override;

	void Jump() override;
	void StopJumping() override;
	void CheckJumpInput(float DeltaTime) override;

	/* Animation activation */
	UFUNCTION(BlueprintImplementableEvent)
		void Activate_Jump();

	bool CanDoubleJump() const;
	bool IsJumping() const;

	bool IsFalling() const;
	bool IsOnGround() const;


#pragma endregion //Basic_Movement
	
#pragma region Crouch

	void Start_Crouch();
	void Stop_Crouch();

#pragma endregion //Crouch

#pragma region OnWall
	/* How far from the player walls will be detected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float WallDetectionRange		UMETA(DisplayName = "Wall Detection Range") { 200.f };
	/* If the player is within this length from the wall, WallJump / WallSlide mechanics are enabled. Should NOT be lower then Wall Detection Range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float WallJump_ActivationRange	UMETA(DisplayName = "Wall-Jump/Slide Activation Length") { 80.f };
	/* Activate ledgegrab if a wall + ledge is detected and the player is within this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float LedgeGrab_ActivationRange		UMETA(DisplayName = " Ledgegrab Activation Length") { 120.f };

	FVector Wall_Normal{};
	FHitResult WallHit{};
	bool DetectNearbyWall();
	bool bFoundWall{};

	FVector FromActorToWall{};
	float ActorToWall_Length{};


	#pragma region Wall Jump
		/* ------------------------ Wall Jump --------------------- */

		/* The maximum time the character can hold on to the wall they stick to during wall jump */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall|Wall Jump")
			float WallJump_MaxStickTimer UMETA(DisplayName = "Max Sticking Time") { 1.f };
		float WallJump_StickTimer{};

		/* Time until character can stick to wall again */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall|Wall Jump")
			float WallJump_MaxNonStickTimer UMETA(DisplayName = "No Stick Timer") { 0.5f };
		float WallJump_NonStickTimer{};

		float InputAngleToForward{};

		bool bStickingToWall{};
		bool bFoundStickableWall{};
		bool bCanStickToWall{ true };
		bool bOnWallActive{ true };
		FVector StickingSpot{};

		void SetActorLocation_WallJump(float DeltaTime);

		/* Is currently sticking to a wall */
		bool IsStickingToWall() const;
		/* Is in contact with a wall and slowing down */
		bool IsOnWall() const;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall|Wall Jump|Animation")
			float OnWall_InterpolationSpeed{ 10.f };

	#pragma endregion //Wall Jump

	#pragma region LedgeGrab

		UPROPERTY(BlueprintReadOnly)
			bool bFoundLedge{};
		UPROPERTY(BlueprintReadOnly)
			bool bIsLedgeGrabbing{};
		//UPROPERTY(BlueprintReadOnly)
			//bool bShouldLedgeGrabNextFrame{};


		bool IsLedgeGrabbing() const { return bIsLedgeGrabbing; }

		void Do_LedgeGrab(float DeltaTime);

		/* How far above ifself the character will be able to grab a ledge */
		//UPROPERTY(EditAnywhere, Category = "Movement|OnWall|LedgeGrab")
			//float LedgeGrab_GrabLength				UMETA(DisplayName = "GrabLength")	{ 100.f };
		/* Vertical Grab length */
		UPROPERTY(EditAnywhere, Category = "Movement|OnWall|LedgeGrab")
			float LedgeGrab_VerticalGrabLength		UMETA(DisplayName = "Vertical GrabLength") { 100.f };
		/* Horizontal Grab length */
		UPROPERTY(EditAnywhere, Category = "Movement|OnWall|LedgeGrab")
			float LedgeGrab_HorizontalGrabLength	UMETA(DisplayName = "Horizontal GrabLength") { 100.f };
		/* How far below from the ledge will the character hold itself */
		UPROPERTY(EditAnywhere, Category = "Movement|OnWall|LedgeGrab")
			float LedgeGrab_HoldLength				UMETA(DisplayName = "HoldLength")	{ 100.f };

		/* The positional interpolation alpha for each frame between the actors location and the intended ledgegrab location */
		UPROPERTY(EditAnywhere, Category = "Movement|OnWall|LedgeGrab")
			float PositionLerpAlpha{ 0.5f };

		FHitResult LedgeHit{};
		FVector LedgeLocation{};
		float LengthToLedge{};
		FVector PlayersLedgeLocation{};
		bool DetectLedge(FVector& Out_IntendedPlayerLedgeLocation, const FHitResult& In_WallHit, FHitResult& Out_Ledge, float Vertical_GrabLength, float Horizontal_GrabLength);
	
		void MoveActorToLedge(float DeltaTime);

	#pragma endregion //LedgeGrab

#pragma endregion //OnWall


	void ResetWallJumpAndLedgeGrab();

	void ResetActorRotationPitchAndRoll(float DeltaTime);
	void RotateActorYawToVector(float DeltaTime, FVector AimVector);
	void RotateActorPitchToVector(float DeltaTime, FVector AimVector);
		void RotateActorYawPitchToVector(float DeltaTime, FVector AimVector);	//Old
	void RollActorTowardsLocation(float DeltaTime, FVector Point);

	/* Returns the angle and direction*/
	//float AngleBetweenVectors(const FVector& FirstVec, const FVector& SecondVec);

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
	uint8 DashCounter{ 1 };

	FVector DashDirection{};

	/* How long the pre dash action lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
		float Pre_DashTime{ 0.1f };
	/* How long the dash action lasts */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
		float DashTime{ 0.5f };
	/* How far the character will dash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dash")
		float DashLength{ 1000.f };

	UFUNCTION(BlueprintImplementableEvent)
		void Activate_Dash();

	void Dash();
	void Stop_Dash();

	bool IsDashing() const;

#pragma endregion //Dash


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
		bool bGrapple_Available{};
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHookRange{ 2000.f };
	
	/* The onscreen aiming location */
	UPROPERTY(BlueprintReadOnly)
		FVector2D AimingLocation {};
	UPROPERTY(BlueprintReadOnly)
		FVector2D AimingLocationPercentage {};
	UPROPERTY(BlueprintReadOnly)
		FVector2D ViewPortSize {};

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
		bool bGrapple_Swing {};
	
	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_PreLaunch {};
	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_Launch {};
	
	/* Time it takes for half a rotation around the GrappledActor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Swing")
		float GrappleHook_SwingTime{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Swing")
		float GrappleHook_Swing_MaxSpeed{ 2000.f };
	/* Initial length between actor and grappled object */
	float GrappleRadiusLength{};

	/* Grapplehook swing */
	UFUNCTION(BlueprintCallable)
	void Initial_GrappleHook_Swing();
	void Update_GrappleHook_Swing(float DeltaTime);
	void RotateActor_GrappleHook_Swing(float DeltaTime);
	void GrappleHook_Swing_RotateCamera(float DeltaTime);	// Slightly dissorienting

	/* How long the player will be held in the air before being launched towards the grappled actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_PreLaunch_Timer_Length UMETA(DisplayName = "PreLaunch Timer")  { 0.25f };

	float GrappleDrag_PreLaunch_Timer{};

	/* Interpolation speed of the camera rotation during grapplehook Drag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_InterpSpeed			UMETA(DisplayName = "Interpolation Speed")		{ 3.f };

	/* Pitch adjustment for the camera rotation during the Pre_Launch of Grapple Drag  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_PitchPoint				UMETA(DisplayName = "Pitch Point")				{ 20.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_MaxSpeed						UMETA(DisplayName = "Max Speed")				{ 2000.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_MinRadiusDistance				UMETA(DisplayName = "Min Radius Distance")		{ 50.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_Update_TimeMultiplier			UMETA(DisplayName = "Time Multiplier")			{ 2.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag")
		float GrappleDrag_Update_Time_MIN_Multiplier	UMETA(DisplayName = "Minimum Time Multiplier")	{ 2.f };

	float GrappleDrag_CurrentSpeed{};

	/* Grapplehook drag during swing */
	UFUNCTION(BlueprintCallable)
	void Initial_GrappleHook_Drag(float DeltaTime);
	void Update_GrappleHook_Drag(float DeltaTime);
	void GrappleHook_Drag_RotateCamera(float DeltaTime);
	void RotateActor_GrappleHook_Drag(float DeltaTime);

	UPROPERTY(BlueprintReadWrite)
	bool bGrappleEnd{};

	UFUNCTION(BlueprintCallable)
	bool IsGrappling() const;
#pragma endregion //GrappleHook
};
