// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../Interfaces/AttackInterface.h"
#include "../Interfaces/CameraGuideInterface.h"
#include "../DebugMacros.h"
#include "Camera/CameraShakeBase.h"
#include "SteikeAnimInstance.h"
#include "GameplayTagAssetInterface.h"
//#include "../GameplayTags.h"
#include "../StaticActors/Collectible.h"

#include "SteikemannCharacter.generated.h"

#define GRAPPLE_HOOK ECC_GameTraceChannel1
#define ECC_PogoCollision ECC_GameTraceChannel2

class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;


UCLASS()
class STEIKEMANN_UE_API ASteikemannCharacter : public ACharacter, 
	public IGrappleTargetInterface,
	public IAttackInterface,
	public IGameplayTagAssetInterface,
	public ICameraGuideInterface
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

	/* testing */
	//FGameplayTag Tag_Player;

	FGameplayTag Tag_Enemy;
	FGameplayTag Tag_EnemyAubergineDoggo;
	FGameplayTag Tag_GrappleTarget;
	FGameplayTag Tag_GrappleTarget_Static;
	FGameplayTag Tag_GrappleTarget_Dynamic;


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


	TWeakObjectPtr<class USteikemannCharMovementComponent> MovementComponent;
	/* Returns the custom MovementComponent. A TWeakPtr<class USteikemannCharMovementComponent> */
	TWeakObjectPtr<class USteikemannCharMovementComponent> GetMoveComponent() const { return MovementComponent; }

	USteikeAnimInstance* SteikeAnimInstance{ nullptr };

	void AssignAnimInstance(USteikeAnimInstance* AnimInstance) { SteikeAnimInstance = AnimInstance; }
	USteikeAnimInstance* GetAnimInstance() const { return SteikeAnimInstance; }

	APlayerController* PlayerController{ nullptr };
	APlayerController* GetPlayerController() const { return PlayerController; }

	/*
		GameplayTags
	*/
	
	FGameplayTagContainer GameplayTags;
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameplayTags")
		//FGameplayTag* Player;

	UFUNCTION(BlueprintCallable, Category = GameplayTags)
		virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; return; }

#pragma region Audio
	UPROPERTY(EditAnywhere, Category = "Audio")
		UAudioComponent* Component_Audio{ nullptr };


#pragma endregion //Audio



#pragma region ParticleEffects

	/* ------------------- Particle Effects ------------------- */
	UPROPERTY(EditAnywhere, Category = "Particle Effects")
		UNiagaraComponent* Component_Niagara{ nullptr };

	/* Create tmp niagara component */
	UNiagaraComponent* CreateNiagaraComponent(FName Name, USceneComponent* Parent, FAttachmentTransformRules AttachmentRule, bool bTemp = false);

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

	#pragma region Crouch
		UNiagaraComponent* NiComp_CrouchSlide{ nullptr };

		UPROPERTY(EditAnywhere, Category = "Particle Effects|Crouch")
			UNiagaraSystem* NS_CrouchSlide{ nullptr };

	#pragma endregion //Crouch

#pragma endregion //ParticleEffects



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

#pragma region CameraGuide
	

	//UPROPERTY(EditAnywhere, Category = "Camera", meta = (UIMin = "0", UIMax = "1"))
	float CameraGuide_Pitch{ 0.f };

	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "500"))
			float CameraGuide_Pitch_MIN		UMETA(DisplayName = "Pitch At Min") { 100.f };

	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "5000"))
			float CameraGuide_Pitch_MAX		UMETA(DisplayName = "Pitch At Max") { 500.f };

	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "2"))
			float CameraGuide_ZdiffMultiplier		UMETA(DisplayName = "Zdiff Multiplier") { 1.f };
			
	/* Maximum distance for pitch adjustment */
	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "10000"))
			float CameraGuide_Pitch_DistanceMAX		UMETA(DisplayName = "Max Distance") { 2000.f };

	/* Minimum distance for pitch adjustment */
	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "10000"))
			float CameraGuide_Pitch_DistanceMIN		UMETA(DisplayName = "Min Distance") { 100.f };

	EPointType CurrentCameraGuide;
	EPointType PreviousCameraGuide;
	float Base_CameraBoomLength;
	bool bCamLerpBackToPosition{};

	float CameraGuideAlpha{};

	UPROPERTY(EditAnywhere, Category = "Camera|Default", meta = (UIMin = "0", UIMax = "1"))
		float Default_CameraGuideAlpha{ 0.1f };

	/* ---- GUIDING CAMERA -----
	* Every type of automatic camera guide interaction within one function
	* Volume
	* Automatic during movement
	* */
	virtual void GuideCamera(float DeltaTime) override;
	float Internal_SplineInputkey{};
	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Spline", meta = (UIMin = "0", UIMax = "10"))
		float SplineLerpSpeed{ 10.f };
	virtual void SetSplineInputkey(const float SplineKey) override { Internal_SplineInputkey = SplineKey; }

#pragma endregion //CameraGuide

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
	//UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
	//	bool bAddJumpVelocity{};
	///* How long the jump key can be held to add upwards velocity */
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
	//	float fJumpTimerMax UMETA(DisplayName = "JumpHoldTimer") { 0.2f };
	//UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
	//	float fJumpTimer{};

	/* How long after walking off an edge the player is still allowed to jump */
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump")
		float PostEdge_JumpTimer_Length{ 0.3f };
	float PostEdge_JumpTimer{};
	bool bCanPostEdgeRegularJump{};

	/* The angle from the Upwards axis the jump direction will go towards input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
		float DoubleJump_AngleFromUp{ 60.f };

	/* What the jump strength will be multiplied by when double jumping */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
		float DoubleJump_MultiplicationFactor{ 0.6f };

	/* How long, post double jump, the character is held in the air (Z direction) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
		float Jump_HeightHoldTimer{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
		float JumpHeightHold_VelocityInterpSpeed{ 5.f };

	void Landed(const FHitResult& Hit) override;

	void Jump() override;
	void StopJumping() override;
	void JumpRelease();
	void CheckJumpInput(float DeltaTime) override;

	UFUNCTION(BlueprintImplementableEvent)
		/* Animation activation */
		void Anim_Activate_Jump();

	bool CanDoubleJump() const;
	bool IsJumping() const;

	bool IsFalling() const;
	bool IsOnGround() const;

	//bool CanJump() const override;

	/* 
	 * -------------------- New Jump : Cartoony --------------------------- 
	*/
	bool bJumpClick{};

	/* The strength of the Jump */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump|NewJump")
		float JumpStrength{ 2500.f };

	/* If the jump button is released, how fast will the character slow down? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump|NewJump")
		float JumpPrematureSlowDownTime{ 0.2f };

	/* How far through the jump, percentage wise, can the player go. Before releasing the button and still get the full jump height? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump|NewJump")
		float JumpFullPercentage{ 0.8f };

	/* How long should the character stay at max height? */
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump|NewJump")
		//float Jump_TopFloatTime{ 1.f };



	/*
	* -------------------- Player Pogo Jumping on enemy --------------------------
	*/
	/* The strength of the pogo bounce */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce")
		float PogoBounceStrength{ 2000.f };

	/* Extra contingency length checked between the player and the enemy they are falling towards, before the PogoBounce is called */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce")
		float PogoContingency{ 50.f };


	void CheckIfEnemyBeneath(const FHitResult& Hit);
	UFUNCTION(BlueprintCallable)
		bool CheckDistanceToEnemy(const FHitResult& Hit);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce")
		float PogoInputDirectionMultiplier{ 0.1f };

	UFUNCTION(BlueprintCallable)
		void PogoBounce(const FVector& EnemyLocation);

#pragma endregion //Basic_Movement
	
	/* Includes all actions related to the crouch button */
#pragma region Crouch		
	
	bool bPressedCrouch{};
	bool bIsCrouchWalking{};
	bool IsCrouchWalking() const { return bIsCrouchWalking; }
	bool IsCrouching() const { return bIsCrouched; }

	/* Regular Crouch */
	/* Crouch slide will only start if the player is walking with a speed above this */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float Crouch_WalkToSlideSpeed  UMETA(DisplayName = "Walk To Crouch Slide Speed") { 400.f };
	void Start_Crouch();
	void Stop_Crouch();

	/* -------------- SLIDE ------------- */
	bool bPressedSlide{};
	
	void Click_Slide();
	void UnClick_Slide();

	/* How long will the crouch slide last */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float CrouchSlide_Time  UMETA(DisplayName = "Crouch Slide Time") { 0.5f };

	/* How long before a new crouchslide can begin */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float Post_CrouchSlide_Time  UMETA(DisplayName = "Crouch Slide Wait Time") { 0.5f };

	/* Initial CrouchSlide Speed */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float CrouchSlideSpeed{ 1000.f };

	/* The End CrouchSlide Speed will be multiplied by this value */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float EndCrouchSlideSpeedMultiplier{ 0.5f };

	bool bCanCrouchSlide{ true };
	bool bCrouchSliding{};
	bool IsCrouchSliding() const { return bCrouchSliding; }

	FTimerHandle CrouchSlide_TimerHandle{};
	FTimerHandle Post_CrouchSlide_TimerHandle{};

	void Start_CrouchSliding();
	void Stop_CrouchSliding();
	void Reset_CrouchSliding();

	/* CrouchJump && CrouchSlideJump */
	bool CanCrouchJump()		const { return IsCrouching() && IsCrouchWalking(); }
	bool CanCrouchSlideJump()	const { return IsCrouching() && IsCrouchSliding(); }

#pragma endregion //Crouch	

#pragma region Collectibles & Health

	void ReceiveCollectible(ECollectible type);

	UPROPERTY(BlueprintReadWrite, Category = "Collectibles")
		int CollectibleCommon{};
	UPROPERTY(BlueprintReadWrite, Category = "Collectibles")
		int CollectibleCorruptionCore{};

	UPROPERTY(BlueprintReadWrite, Category = "Health")
		int Health{ 3 };

	void GainHealth(int amount);

#pragma endregion //Collectibles & Health

/* ---------------------------------- ON WALL ----------------------------------- */
#pragma region OnWall

	/* How far from the player walls will be detected */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float WallDetectionRange		UMETA(DisplayName = "Wall Detection Range") { 200.f };
	
	/* If the player is within this length from the wall, WallJump / WallSlide mechanics are enabled. Should NOT be higher than Wall Detection Range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float WallJump_ActivationRange	UMETA(DisplayName = "Wall-Jump/Slide Activation Length") { 80.f };
	
	/* Activate ledgegrab if a wall + ledge is detected and the player is within this range */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float LedgeGrab_ActivationRange		UMETA(DisplayName = " Ledgegrab Activation Length") { 120.f };

	/* How far, including the capsule radius, should the character be from the wall during OnWall mechanics 
	 * Functions more as a contingency */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall")
		float OnWall_ExtraCharacterLengthFromWall UMETA(DisplayName = "Length from wall") { 10.f };

	void Do_OnWallMechanics(float DeltaTime);

	FVector Wall_Normal{};
	FHitResult WallHit{};
	bool DetectNearbyWall();
	bool bFoundWall{};


	FVector FromActorToWall{};
	float ActorToWall_Length{};

	/* The angle between the actors forward axis and the players input during OnWall Mechanics */
	float InputAngleToForward{};
	/* The angle between the actors forward axis and the players input during OnWall Mechanics */
	float InputDotProdToForward{};

	void CalcAngleFromActorForwardToInput();

	void ResetWallJumpAndLedgeGrab();

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

		bool bStickingToWall{};
		bool bFoundStickableWall{};
		bool bCanStickToWall{ true };
		bool bOnWallActive{ true };
		FVector StickingSpot{};

		void SetActorLocation_WallJump(float DeltaTime);

		bool bPostWallJump{};
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall|Wall Jump")
			float fPostWallJumpTimer{ 0.2f };

		/* WallJump activation range on Jump, different from the passive activation range of On_Wall mechanics */
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|OnWall|Wall Jump")
			float WallJump_JumpWallActivation{ 200.f };

		bool Jump_DetectWall();
		void WallJump_Reset();

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


		FHitResult LedgeHit{};
		FVector LedgeLocation{};
		float LengthToLedge{};
		FVector PlayersLedgeLocation{};
		bool DetectLedge(FVector& Out_IntendedPlayerLedgeLocation, const FHitResult& In_WallHit, FHitResult& Out_Ledge, float Vertical_GrabLength, float Horizontal_GrabLength);
	

		bool IsLedgeGrabbing() const { return bIsLedgeGrabbing; }

		bool Do_LedgeGrab(float DeltaTime);

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
		void MoveActorToLedge(float DeltaTime);

		void DrawDebugArms(const float& InputAngle);

	#pragma endregion //LedgeGrab

#pragma endregion //OnWall




	/* ----------------------- Actor Rotation Functions ---------------------------------- */
	void ResetActorRotationPitchAndRoll(float DeltaTime);
	void RotateActorYawToVector(FVector AimVector, float DeltaTime = 0);
	void RotateActorPitchToVector(FVector AimVector, float DeltaTime = 0);
		void RotateActorYawPitchToVector(FVector AimVector, float DeltaTime = 0);	//Old
	void RollActorTowardsLocation(FVector Point, float DeltaTime = 0);


/* -------------------------------- GRAPPLEHOOK ----------------------------- */
#pragma region GrappleHook
public: 
	/* ------- GrappleTargetInterface ------ */
	virtual void TargetedPure() override {}

	virtual void UnTargetedPure() override {}

	virtual void HookedPure() override {}
	virtual void HookedPure(const FVector InstigatorLocation, bool PreAction = false) override {}

	virtual void UnHookedPure() override {}

	//virtual FGameplayTag GetGrappledGameplayTag_Pure() const override { return Player; }

	/* ------- Native Variables and functions -------- */
	void RightTriggerClick();
	void RightTriggerUn_Click();
	void LeftTriggerClick();
	void LeftTriggerUn_Click();

	UPROPERTY(BlueprintReadOnly)
		TWeakObjectPtr<AActor> GrappledActor{ nullptr };
	UPROPERTY(BlueprintReadOnly)
		TWeakObjectPtr<AActor> Active_GrappledActor{ nullptr };

	UPROPERTY(BlueprintReadOnly)
		FGameplayTag GpT_GrappledActorTag;

	/* The rope that goes from the player character to the grappled actor */
	FVector GrappleRope{};

	/* Collision sphere used for detecting nearby grappleable actors */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Grappling Hook")
		class USphereComponent* GrappleTargetingDetectionSphere{ nullptr };

	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHookRange{ 2000.f };

	TArray<AActor*> InReachGrappleTargets;

	UFUNCTION()
		void OnGrappleTargetDetectionBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnGrappleTargetDetectionEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	/* The added percentage of the screens height that is added to the aiming location. A higher number turns it closer to the
		middle, with a lower number further up. 0 directly to the middle */
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Targeting")
		float GrappleAimYChange_Base UMETA(DisplayName = "GrappleAimYDifference") { 4.f };
	float GrappleAimYChange{};

	void GetGrappleTarget();

	FTimerHandle TH_Grapplehook_Start;
	FTimerHandle TH_Grapplehook_End_Launch;

	UPROPERTY(BlueprintReadOnly)
		bool bIsGrapplehooking{};
	UPROPERTY(BlueprintReadOnly)
		bool bIsPostGrapplehooking{};

	void Initial_GrappleHook();
	void Start_GrappleHook();
	void Launch_GrappleHook();
	void Stop_GrappleHook();

	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_PreLaunch{};
	UPROPERTY(BlueprintReadOnly)
		bool bGrapple_Launch{};
	
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_LaunchSpeed{ 2000.f };

	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_Threshhold{ 500.f };

	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_DividingFactor{ 2.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_PostLaunchTimer UMETA(DisplayName = "Post Launch Timer") { 1.f };


	/* How long the player will be held in the air before being launched towards the grappled actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleDrag_PreLaunch_Timer_Length UMETA(DisplayName = "PreLaunch Timer")  { 0.25f };

	/* -- GRAPPLE CAMERA VARIABLES -- */
	/* Interpolation speed of the camera rotation during grapplehook Drag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_InterpSpeed			UMETA(DisplayName = "Interpolation Speed")		{ 3.f };

	/* Pitch adjustment for the camera rotation during the Pre_Launch of Grapple Drag  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_PitchPoint				UMETA(DisplayName = "Pitch Point")				{ 20.f };

	void GrappleHook_Drag_RotateCamera(float DeltaTime);
	void RotateActor_GrappleHook_Drag(float DeltaTime);

	bool bGrapplingStaticTarget{};
	bool bGrapplingDynamicTarget{};

	UFUNCTION(BlueprintCallable)
		bool IsGrappling() const;
	UFUNCTION(BlueprintCallable)
		bool IsPostGrapple() const { return bIsPostGrapplehooking; }		

#pragma endregion //GrappleHook

/* ----------------------------------------- ATTACKS ----------------------------------------------- */
#pragma region Attacks

	bool CanBeAttacked() override;

	FVector AttackDirection{};

	FVector AttackColliderScale{};

	void Click_Attack();
	void UnClick_Attack();


	UFUNCTION(BlueprintImplementableEvent)
		void Start_Attack();
	UFUNCTION(BlueprintCallable)
		void Start_Attack_Pure();

	UFUNCTION(BlueprintCallable)
		void Stop_Attack();

	UFUNCTION(BlueprintCallable)
		void Activate_AttackCollider();

	UFUNCTION(BlueprintCallable)
		void Deactivate_AttackCollider();

	UFUNCTION(BlueprintPure)
		bool DecideAttackType();

	void RotateToAttack();



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components|Attack")
		class UBoxComponent* AttackCollider{ nullptr };

	UFUNCTION()
		void OnAttackColliderBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Gen_Attack(IAttackInterface* OtherInterface, AActor* OtherActor, EAttackType& AType) override;

	/* ---- Moving Character During Shared Basic Attack Anticipation ---- */
	/* How far the character will move forward during the Shared Basic Attack Anticipation. Before the attack type is decided */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		float PreBasicAttackMovementLength{ 50.f };
	bool bPreBasicAttackMoveCharacter{};
	void PreBasicAttackMoveCharacter(float DeltaTime);


	/* -------- Animation Variables -------- */
	/* The speed of the anticipation to the regular attack. Which is shared between SmackAttack and ScoopAttack.
	 * At the end of this anticipation, 
	 * If the player still holds the attack button, the character will perform the scoop attack. 
	 *  Else if the button is not held at this time, the character will perform the regular SmackAttack */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|SmackAttack")
		float SmackAttack_Anticipation_Rate		UMETA(DisplayName = "1. Smack Anticipation Rate") { 4.5f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|SmackAttack")
		float SmackAttack_Action_Rate			UMETA(DisplayName = "2. Smack Action Rate") { 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|SmackAttack")
		float SmackAttack_Reaction_Rate			UMETA(DisplayName = "3. Smack Reaction Rate") { 2.f };
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|ScoopAttack")
		float ScoopAttack_Anticipation_Rate		UMETA(DisplayName = "1. Scoop Anticipation Rate") { 10.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|ScoopAttack")
		float ScoopAttack_Action_Rate			UMETA(DisplayName = "2. Scoop Action Rate") { 7.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|ScoopAttack")
		float ScoopAttack_Reaction_Rate			UMETA(DisplayName = "3. Scoop Reaction Rate") { 2.f };




	/* --------------------------------- SMACK ATTACK ----------------------------- */
	#pragma region SmackAttack


	bool bAttackPress{};

	UFUNCTION(BlueprintCallable)
		bool GetAttackPress() const { return bAttackPress; }
	/* When the button can be pressed again */
	bool bCanAttack{ true };
	/* Related to collider and locking movement */
	bool bAttacking{};
	bool IsAttacking() const { return bAttacking; }
	
	/* When TRUE the characters rotation is decided by the players input direction 
	 * When FALSE the characters rotation is decided by the direction of the camera */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		bool bSmackDirectionDecidedByInput{ true };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		bool bDisableMovementDuringAttack{ true };

	/* The angle from the ground the enemy will be smacked. 0 degrees: Is parallel to the ground. 90 degrees: Is directly upwards */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float SmackUpwardAngle{ 30.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float SmackAttackStrength{ 1500.f };

	/* ---- Moving Character During SmackAttack ---- */
	/* How far the character will move forward during Smack Attack. Happens during The Action when the collider is active */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		float SmackAttackMovementLength{ 100.f };

	bool bSmackAttackMoveCharacter{};
	void SmackAttackMoveCharacter(float DeltaTime);



	UFUNCTION(BlueprintCallable)
		void Activate_SmackAttack();
	UFUNCTION(BlueprintCallable)
		void Deactivate_SmackAttack();

	bool bIsSmackAttacking{};

	bool bCanBeSmackAttacked{ true };



	//void Do_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) override;
	void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;
	void Receive_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) override;

	bool GetCanBeSmackAttacked() const override { return bCanBeSmackAttacked; }
	void ResetCanBeSmackAttacked() override { bCanBeSmackAttacked = true; }

	#pragma endregion //SmackAttack

	/* ---------------------------- SCOOP ATTACK ---------------------- */
	#pragma region ScoopAttack

	bool bClickScoopAttack{};

	UFUNCTION(BlueprintImplementableEvent)
		void Start_ScoopAttack();
	UFUNCTION(BlueprintCallable)
		void Start_ScoopAttack_Pure();
	void Click_ScoopAttack();
	void UnClick_ScoopAttack();

	bool bIsScoopAttacking{};
	bool bHasbeenScoopLaunched{};

	/* ---- Moving Character During ScoopAttack ---- */
	/* How far the character will move forward during Scoop Attack. During Scoop Anticipation and Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		float ScoopAttackMovementLength{ 75.f };

	bool bScoopAttackMoveCharacter{};
	void ScoopAttackMoveCharacter(float DeltaTime);

	UFUNCTION(BlueprintCallable)
		void Activate_ScoopAttack();
	UFUNCTION(BlueprintCallable)
		void Deactivate_ScoopAttack();

	/* Whether or not the player character stay on the ground and only launch the enemy in the air during scoop		(true)  (Checked) 
	 * OR the player character will be launched in to the air with the enemy when using scoop attack				(false) (Un-checked) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		bool bStayOnGroundDuringScoop{ true }; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float ScoopStrength{ 5000.f };

	void Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;
	void Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength) override;

	#pragma endregion //ScoopAttack

	/* --------------------------- GROUND POUND -------------------------- */
	#pragma region GroundPound

	bool bGroundPoundPress{};
	bool bCanGroundPound{ true };
	bool bIsGroundPounding{};

	void Click_GroundPound();
	void UnClick_GroundPound();

	/* Movement */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GroundPound")
		float GP_PrePoundAirtime{ 0.3f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|GroundPound")
		float GP_LaunchStrength{ 2500.f };
	FTimerHandle THandle_GPHangTime;


	void Launch_GroundPound();

	/* Collider */
	/* Time it takes for the ground pound collider to expand to max size */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GroundPound")
		float GroundPoundExpandTime{ 0.5f };
	/* The size of the ground pound hitbox's radius */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GroundPound")
		float MaxGroundPoundRadius{ 300.f };
	float CurrentGroundPoundColliderSize{};

	FTimerHandle THandle_GPExpandTime{};
	FTimerHandle THandle_GPReset{};

	void Start_GroundPound();
	void Deactivate_GroundPound();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
		class USphereComponent* GroundPoundCollider{ nullptr };


	UFUNCTION(BlueprintCallable)
		void GroundPoundLand(const FHitResult& Hit);
	void ExpandGroundPoundCollider(float DeltaTime);

	void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor);
	void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength);

	#pragma endregion //GroundPound

#pragma endregion //Attacks
};
