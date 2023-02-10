// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../AbstractClasses/AbstractCharacter.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../Interfaces/AttackInterface.h"
#include "../Interfaces/CameraGuideInterface.h"
#include "../DebugMacros.h"
#include "Camera/CameraShakeBase.h"
#include "SteikeAnimInstance.h"
#include "GameplayTagAssetInterface.h"
#include "../StaticActors/Collectible.h"
#include "../WallDetectionComponent.h"

#include "../Delegates_Shared.h"

#include "SteikemannCharacter.generated.h"

#define GRAPPLE_HOOK ECC_GameTraceChannel1
#define ECC_PogoCollision ECC_GameTraceChannel2

DECLARE_DELEGATE(FAttackActionBuffer)

//class UNiagaraSystem;
//class UNiagaraComponent;
class USoundBase;

UENUM()
enum class EMovementInput : int8
{
	Open,
	Locked,
	PeriodLocked
};
UENUM()
enum class EState : int8
{
	STATE_None, // Used when leaving a state and reevaluating the next state

	// Default States
	STATE_OnGround,
	STATE_InAir,
	
	// Advanced States
	STATE_OnWall,
	STATE_Attacking,
	STATE_Grappling
};

UENUM()
enum class EAirState : int8
{
	AIR_None,
	AIR_Freefall,
	AIR_Jump,
	AIR_Pogo,
	AIR_PostScoopJump
};

UENUM() 
enum class EPogoType : int8
{
	POGO_None,
	POGO_Passive,
	POGO_Active,
	POGO_Groundpound,

	POGO_Leave
};

UENUM()
enum class EGrappleState : int8
{
	None,

	Pre_Launch,
	Post_Launch,
	Leave
};
UENUM()
enum class EGrappleType : int8
{
	None, 

	Static,
	Static_StuckEnemy_Air,
	Static_StuckEnemy_Ground,
	
	Dynamic_Air,
	Dynamic_Ground
};

UENUM()
enum class EAttackState : int8
{
	None,

	Smack,
	Scoop,
	GroundPound

	,Post_GroundPound
	,Post_Buffer
};

UENUM()
enum class EPromptState : int8
{
	None,
	WithingArea,
	InPrompt
};


UCLASS()
class STEIKEMANN_UE_API ASteikemannCharacter : public ABaseCharacter, 
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


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/* The Raw InputVector */
	UPROPERTY(BlueprintReadOnly)
		FVector InputVectorRaw;
	/* Input vector rotated to match the playercontrollers rotation */
	UPROPERTY(BlueprintReadOnly)
		FVector m_InputVector;

#pragma region Prompt Area
	/* Player within prompt area */
	UPROPERTY(BlueprintReadOnly)
		EPromptState m_PromptState = EPromptState::None;
	FVector m_PromptLocation;
	class ADialoguePrompt* m_PromptActor{ nullptr };

	UPROPERTY(BlueprintReadOnly)
		bool bCameraLerpBack_PostPrompt{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera|Prompt")
		float CameraLerpSpeed_Prompt{ 2.f };
	float m_CameraLerpAlpha_PostPrompt{};
	
	void EnterPromptArea(ADialoguePrompt* promptActor, FVector promptLocation);
	void LeavePromptArea();

	bool ActivatePrompt();
	bool ExitPrompt();
#pragma endregion // Prompt Area

	EMovementInput m_EMovementInputState = EMovementInput::Open;

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
#pragma region Material
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Materialss", meta = (DisplayPriority = "1"))
		class UMaterialParameterCollection* MPC_Player;
	void Material_UpdateParameterCollection_Player(float DeltaTime);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materialss", meta = (DisplayPriority = "2"))
		UCurveFloat* Curve_DecalAlpha;
#pragma endregion //Material
#pragma region ParticleEffects

	/* ------------------- Particle Effects ------------------- */
	UPROPERTY(EditAnywhere, Category = "Particle Effects")
		UNiagaraComponent* Component_Niagara{ nullptr };

	/* Create tmp niagara component */
	//UNiagaraComponent* CreateNiagaraComponent(FName Name, USceneComponent* Parent = nullptr, FAttachmentTransformRules AttachmentRule = FAttachmentTransformRules::SnapToTargetIncludingScale, bool bTemp = false);


	#pragma region Landing
		/* ------------------- PE: Landing ------------------- */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|Land")
			UNiagaraSystem* NS_Land{ nullptr };

		UFUNCTION(BlueprintCallable)
			void NS_Land_Implementation(const FHitResult& Hit);

		/* The amount of particles that will spawn determined by the characters landing velocity, times this multiplier */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|Land")
			float NSM_Land_ParticleAmount		/*UMETA(DisplayName = "Particle Amount Multiplier") */{ 0.5f };
		/* The speed of the particles will be determined by the characters velocity when landing, times this multiplier */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|Land")
			float NSM_Land_ParticleSpeed		/*UMETA(DisplayName = "Particle Speed Multiplier")*/ { 0.5f };

		#pragma endregion //Landing

	#pragma region OnWall
		/* ------------------- PE: OnWall ------------------- */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|WallJump")
			UNiagaraSystem* NS_WallSlide{ nullptr };
		/* The amount of particles per second the system should emit */
		UPROPERTY(EditAnywhere, Category = "Particle Effects|WallJump")
			float NS_WallSlide_ParticleAmount	/*UMETA(DisplayName = "WallSlide ParticleAmount")*/ { 1000.f };
	#pragma endregion //OnWall

	#pragma region Attack
	UNiagaraComponent* NiagaraComp_Attack{ nullptr };
	UPROPERTY(EditAnywhere, Category = "Particle Effects|Attack")
		UNiagaraSystem* NS_AttackContact{ nullptr };

	#pragma endregion //Attack

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

	FTransform m_CameraTransform;
	bool LerpCameraBackToBoom(float DeltaTime);


#pragma region CameraGuide
	

	//UPROPERTY(EditAnywhere, Category = "Camera", meta = (UIMin = "0", UIMax = "1"))
	float CameraGuide_Pitch{ 0.f };

	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "500"))
			float CameraGuide_Pitch_MIN		/*UMETA(DisplayName = "Pitch At Min")*/ { 100.f };

	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "5000"))
			float CameraGuide_Pitch_MAX		/*UMETA(DisplayName = "Pitch At Max")*/ { 500.f };

	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "2"))
			float CameraGuide_ZdiffMultiplier		/*UMETA(DisplayName = "Zdiff Multiplier") */{ 1.f };
			
	/* Maximum distance for pitch adjustment */
	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "10000"))
			float CameraGuide_Pitch_DistanceMAX		/*UMETA(DisplayName = "Max Distance") */{ 2000.f };

	/* Minimum distance for pitch adjustment */
	UPROPERTY(EditAnywhere, Category = "Camera|Volume|Pitch", meta = (UIMin = "0", UIMax = "10000"))
			float CameraGuide_Pitch_DistanceMIN		/*UMETA(DisplayName = "Min Distance") */{ 100.f };

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


	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic", meta = (UIMin = "0", UIMax = "1"))
		float GrappleDynamic_DefaultAlpha{ 0.3f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic", meta = (UIMin = "0", UIMax = "90"))
		float GrappleDynamic_MaxYaw{ 30.f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic", meta = (UIMin = "0", UIMax = "1"))
		float GrappleDynamic_YawAlpha{ 0.3f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic", meta = (UIMin = "0", UIMax = "1"))
		float GrappleDynamic_MaxPitch{ 0.5f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic", meta = (UIMin = "0", UIMax = "1"))
		float GrappleDynamic_PitchAlpha{ 0.2f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic", meta = (UIMin = "-1", UIMax = "1"))
		float GrappleDynamic_DefaultPitch{ 0.2f };


	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic|Pitch", meta = (UIMin = "0", UIMax = "1500"))
		float GrappleDynamic_Pitch_DistanceMIN	 { 100.f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic|Pitch", meta = (UIMin = "0", UIMax = "500"))
		float GrappleDynamic_Pitch_MIN{ 100.f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic|Pitch", meta = (UIMin = "0", UIMax = "5000"))
		float GrappleDynamic_Pitch_MAX{ 100.f };
	UPROPERTY(EditAnywhere, Category = "Camera|Mechanic|GrappleDynamic|Pitch", meta = (UIMin = "0", UIMax = "2"))
		float GrappleDynamic_ZdiffMultiplier{ 1.f };

	float InitialGrappleDynamicZ{};

	float GrappleDynamic_SLerpAlpha{};
	void GuideCameraTowardsVector(FVector vector, float alpha);
	void GuideCameraPitch(float z, float alpha);
	float GuideCameraPitchAdjustmentLookAt(FVector LookatLocation, float MinDistance, float MaxDistance, float PitchAtMin, float PitchAtMax, float ZdiffMultiplier);

	void GrappleDynamicGuideCamera(float deltatime);

#pragma endregion //CameraGuide

#pragma endregion //Camera

#pragma region Basic_Movement
public:	// States
	EState GetState() const { return m_EState; }
	void SetState(EState state) { m_EState = state; }
	//void ReevaluateState();
	UFUNCTION(BlueprintCallable)
		void ResetState();
	void SetDefaultState();

	virtual void AllowActionCancelationWithInput() override;
private:
	EState m_EState = EState::STATE_OnGround;
	EAirState m_EAirState = EAirState::AIR_None;
	float m_BaseGravity{};

public:/* ------------------- Basic Movement ------------------- */
public:
	UPROPERTY(EditAnywhere, Category = "Movement|Walk/Run", meta = (AllowPrivateAcces = "true"))
	float TurnRate{ 50.f };

	bool BreakMovementInput(float value);

	void MoveForward(float value);
	void MoveRight(float value);
	virtual void AddControllerYawInput(float Val) override;
	virtual void AddControllerPitchInput(float Val) override;
	void TurnAtRate(float rate);
	void LookUpAtRate(float rate);

	FTimerHandle TH_MovementPeriodLocked;
	FPostLockedMovement PostLockedMovementDelegate;
	void LockMovementForPeriod(float time, TFunction<void()> lambdaCall = nullptr);

	bool bActivateJump{};
	UPROPERTY(BlueprintReadOnly, Category = "Movement|Jump", meta = (AllowPrivateAccess = "true"))
		bool bJumping{};
	UPROPERTY(BlueprintReadOnly)
		bool bCanEdgeJump{};

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
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Jump")
		//float PostScoop_JumpTime{ 0.3f };

	void Landed(const FHitResult& Hit) override;

	void Jump() override;
	void JumpRelease();

	void Jump_OnGround();
	void Jump_DoubleJump();
	void Jump_Undetermined();

	/* Animation activation */
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Activate_Jump();
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Activate_DoubleJump();
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Land();

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

	/* Cancels the currently running Animation montage */
	UFUNCTION(BlueprintImplementableEvent)
		void CancelAnimation();
	void CancelAnimationMontageIfMoving(TFunction<void()> lambdaCall);
#pragma endregion //Basic_Movement

#pragma region Pogo
private:
	EPogoType m_EPogoType = EPogoType::POGO_None;
	AActor* m_PogoTarget{ nullptr };

public:
	/* The strength of the pogo bounce */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Passive")
		float PB_LaunchStrength_Z_Passive{ 1300.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Passive")
		float PB_LaunchStrength_MultiXY_Passive{ 500.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Active")
		float PB_LaunchStrength_Active{ 1800.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Groundpound")
		float PB_LaunchStrength_Groundpound{ 2500.f };

	// Detection
	/* Extra contingency length checked between the player and the enemy they are falling towards, before the PogoBounce is called */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce")
		float PB_TargetLengthContingency{ 50.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce")
		float PB_Max2DTargetDistance{ 100.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Active", meta = (DisplayPriority = "2"))
		float PB_ActiveDetection_CapsuleZLocation{ 100.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Active", meta = (PrioDisplayPriorityrity = "3"))
		float PB_ActiveDetection_CapsuleHalfHeight{ 100.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Active", meta = (PrioDisplayPriorityrity = "4"))
		float PB_ActiveDetection_CapsuleRadius{ 70.f };

	// Minimum time the pogo state lasts - Will disable some mechanics while in that state
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Passive", meta = (DisplayPriority = "1"))
		float PB_StateTimer_Passive{ 0.1f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Active", meta = (DisplayPriority = "1"))
		float PB_StateTimer_Active{ 0.3f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Groundpound", meta = (DisplayPriority = "1"))
		float PB_StateTimer_Groundpound{ 0.4f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Passive")
		float PB_InputMulti_Passive{ 0.6f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Active")
		float PB_InputMulti_Active{ 0.3f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|PogoBounce|Groundpound")
		float PB_InputMulti_Groundpound{ 0.05f };
	
private: // Within Collision bools
	//EPogoTickCheck m_EPogoTickCheck = EPogoTickCheck::PB_Tick_None;
	//bool bPB_TickCheck_Passive{};
	//bool bPB_TickCheck_Groundpound{};
	//bool bPB_PassiveLaunched{};
	bool bPB_Groundpound_PredeterminedPogoHit{};

	FTimerHandle TH_PB_ExitHandle; // Timer handle holding exit time. For validating buffering of PB_Active inputs
	FTimerHandle TH_Pogo;

public:	// Target detection
	bool PB_TargetBeneath();
	bool PB_ValidTargetDistance(const FVector OtherActorLocation);

	bool PB_Active_TargetDetection();

private:  
	void PB_Pogo();
	void PB_EnterPogoState(float time);

	bool PB_Passive_IMPL(AActor* OtherActor);
	void PB_Launch_Passive();

	void PB_Active_IMPL();
	void PB_Launch_Active();

	bool PB_Groundpound_IMPL(AActor* OtherActor);
	bool PB_Groundpound_Predeterminehit();
	void PB_Launch_Groundpound();

	void PB_Exit();

	bool ValidLengthToCapsule(FVector HitLocation, FVector capsuleLocation, float CapsuleHeight, float CapsuleRadius);

public: // Animation
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Pogo_Passive();

#pragma endregion //Pogo
	
#pragma region Crouch		
public:
	bool bPressedCrouch{};
	bool bIsCrouchWalking{};
	bool IsCrouchWalking() const { return bIsCrouchWalking; }
	bool IsCrouching() const { return bIsCrouched; }

	/* Regular Crouch */
	/* Crouch slide will only start if the player is walking with a speed above this */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float Crouch_WalkToSlideSpeed  /*UMETA(DisplayName = "Walk To Crouch Slide Speed") */{ 400.f };
	void Start_Crouch();
	void Stop_Crouch();

	/* -------------- SLIDE ------------- */
	bool bPressedSlide{};
	
	void Click_Slide();
	void UnClick_Slide();

	/* How long will the crouch slide last */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float CrouchSlide_Time  /*UMETA(DisplayName = "Crouch Slide Time")*/ { 0.5f };

	/* How long before a new crouchslide can begin */
	UPROPERTY(EditAnywhere, Category = "Movement|Crouch")
		float Post_CrouchSlide_Time  /*UMETA(DisplayName = "Crouch Slide Wait Time")*/ { 0.5f };

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
public:
	void ReceiveCollectible(ECollectibleType type);

	UPROPERTY(BlueprintReadWrite, Category = "Collectibles")
		int CollectibleCommon{};
	UPROPERTY(BlueprintReadWrite, Category = "Collectibles")
		int CollectibleCorruptionCore{};

	/* Array of hazard actors whose collision the player is still within */
	TArray<AActor*> CloseHazards;

	UFUNCTION()
		void OnCapsuleComponentBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnCapsuleComponentEndOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Damage", meta = (UIMin = "1", UIMax = "10"))
		int Health{ 3 };
	int MaxHealth{};

	FTimerHandle THDamageBuffer;
	bool bPlayerCanTakeDamage{ true };
	/* Time player is invincible after taking damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Damage", meta = (UIMin = "0.0", UIMax = "2.0"))
		float DamageInvincibilityTime{ 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Damage", meta = (UIMin = "0.0", UIMax = "4000.0"))
		float SelfDamageLaunchStrength{ 1000.f };

	void GainHealth(int amount);
	//void PTakeDamage(int damage, FVector launchdirection);
	void PTakeDamage(int damage, AActor* otheractor, int i = 0);

	UFUNCTION(BlueprintImplementableEvent)
		void Anim_TakeDamage();
	
	bool bIsDead{};
	//bool IsDead() const { return bIsDead; }

	FDeath DeathDelegate;
	FDeath DeathDelegate_Land;
	void Death();
	void Death_Deathzone();
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Death();

	FTransform StartTransform;
	class APlayerRespawn* Checkpoint{ nullptr };
	void Respawn();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health & Damage", meta = (UIMin = "0.0", UIMax = "4000.0"))
		float RespawnTimer{ 2.f };

#pragma endregion //Collectibles & Health

/* ---------------------------------- ON WALL ----------------------------------- */
#pragma region OnWall
public:// Capsule
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		bool bWDC_Debug{};

	UWallDetectionComponent* WallDetector{ nullptr };
	
	// Wall Decetion
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_Capsule_Radius{ 40.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_Capsule_Halfheight{ 90.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float Wall_HeightCriteria{ 20.f };
	// On Wall
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection|WallJump")
		float WDC_Length{ 40.f };
	// Ledge Grab
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection|Ledge")
		float LedgeGrab_Height{ 100.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection|Ledge")
		float LedgeGrab_Inwards{ 50.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection|Ledge")
		FVector LedgeGrab_ActorZOffset {};

public: // OnWall
	FTimerHandle TH_OnWall_Cancel;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		float OnWall_CancelTimer{ 0.5f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		float OnWall_HangTime{ 0.5f };

public: // Walljump
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|Walljump")
		float OnWall_Reset_OnWallJump_Timer{ 1.f };

	/* How long after a regular jump before OnWall mechanics are activated */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		float OnWallActivation_PostJumpingOnGround{ 0.5f };
	/* After grapplehooking to a stuck enemy, disable wall jump for 'time' + time to stuck enemy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		float OnWallActivation_PostStuckEnemyGrappled{ 0.5f };

public:	// Ledge grab


	
public: // General
	EOnWallState m_WallState = EOnWallState::WALL_None;

	// Input direction to the wall 
	float WallInputDirection{};	
	void SetWallInputDirection();

	void ExitOnWall(EState state);
public: // Is funcitons
	bool IsOnWall() const;
	bool IsLedgeGrabbing() const;

public: // Animation and Particle effects
	bool Anim_IsOnWall() const;
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_OnWallContact();

private:
	Wall::WallData m_Walldata;
	Wall::WallData m_WallJumpData;
	Wall::LedgeData m_Ledgedata;

	void DrawDebugArms(const float& InputAngle);

	bool Validate_Ledge(FHitResult& hit);
	void Initial_LedgeGrab();
	void LedgeGrab();

	bool ValidateWall();

	void OnWall_IMPL(float deltatime);
	void OnWall_Drag_IMPL(float deltatime, float velocityZ);

	void ExitOnWall_GROUND();

	void CancelOnWall();

#pragma endregion //OnWall

	/* ----------------------- Actor Rotation Functions ---------------------------------- */
	void ResetActorRotationPitchAndRoll(float DeltaTime);
	void RotateActorYawToVector(FVector AimVector, float DeltaTime = 0);
	void RotateActorPitchToVector(FVector AimVector, float DeltaTime = 0);
		void RotateActorYawPitchToVector(FVector AimVector, float DeltaTime = 0);	//Old
	void RollActorTowardsLocation(FVector Point, float DeltaTime = 0);

	// Other Functions

/* -------------------------------- GRAPPLEHOOK ----------------------------- */
#pragma region GrappleHook
	//NEW--------------------------------------------
public:
	void RightTriggerClick();
	void RightTriggerUn_Click();
	void GH_SetGrappleType(IGameplayTagAssetInterface* ITag, IGrappleTargetInterface* IGrapple);

	UFUNCTION(BlueprintCallable)
		bool IsGrappling() const { return m_EState == EState::STATE_Grappling; }
	UFUNCTION(BlueprintCallable)
		bool Is_GH_PreLaunch() const { return IsGrappling() && m_EGrappleState == EGrappleState::Pre_Launch; }
public:	// Launch Functions
	void GH_PreLaunch();
	void GH_PreLaunch_Static(void(ASteikemannCharacter::* LaunchFunction)(), IGrappleTargetInterface* IGrapple);
	void GH_PreLaunch_Dynamic(IGrappleTargetInterface* IGrapple, bool OnGround);

	void GH_Launch_Static();
	void GH_Launch_Static_StuckEnemy();

	void GH_Stop();

	
public:	// Animation functions
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Grapple_Start();
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Grapple_Middle();
	UFUNCTION(BlueprintImplementableEvent)
		void Anim_Grapple_End();
	void Anim_Grapple_End_Pure();
	void GH_StopControlRig();

	// For the control rig 
	FVector GH_GetTargetLocation() const;
	bool bGH_LerpControlRig{};
	virtual void StartAnimLerp_ControlRig() override;

public:
	UPROPERTY(BlueprintReadOnly)
		FVector Active_GrappledActor_Location{};
private:
	EGrappleState m_EGrappleState = EGrappleState::None;
	EGrappleType m_EGrappleType = EGrappleType::None;
	TWeakObjectPtr<AActor> GrappledActor{ nullptr };
	TWeakObjectPtr<AActor> Active_GrappledActor{ nullptr };
	TWeakObjectPtr<AActor> GrappledEnemy{ nullptr };

public:	// UPROPERTY Variables
	// How long movement input will be disabled after pulling a dynamic target free from being stuck
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GH_PostPullingTargetFreeTime{ 0.5f };
	//-----------------------------------------------

public: 
	/* ------- GrappleTargetInterface ------ */
	virtual void TargetedPure() override {}

	virtual void UnTargetedPure() override {}

	virtual void HookedPure() override {}
	virtual void HookedPure(const FVector InstigatorLocation, bool OnGround, bool PreAction = false) override {}

	virtual void UnHookedPure() override {}

	/* ------- Native Variables and functions -------- */
	void LeftTriggerClick();
	void LeftTriggerUn_Click();


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
		float GrappleAimYChange_Base /*UMETA(DisplayName = "GrappleAimYDifference") */{ 4.f };
	float GrappleAimYChange{};

	void GH_GrappleAiming();

	FTimerHandle TH_Grapplehook_Start;
	FTimerHandle TH_Grapplehook_Pre_Launch;
	FTimerHandle TH_Grapplehook_End_Launch;
	
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_LaunchSpeed{ 2000.f };

	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_Threshhold{ 500.f };

	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_DividingFactor{ 2.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleHook_PostLaunchTimer /*UMETA(DisplayName = "Post Launch Timer")*/ { 1.f };


	/* How long the player will be held in the air before being launched towards the grappled actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook")
		float GrappleDrag_PreLaunch_Timer_Length /*UMETA(DisplayName = "PreLaunch Timer")*/  { 0.25f };


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Grappling Hook|StuckEnemy")
		float GrappleHook_Time_ToStuckEnemy /*UMETA(DisplayName = "Time To Stuck Enemy")*/ { 0.3f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Grappling Hook|StuckEnemy")
		float GrappleHook_AboveStuckEnemy /*UMETA(DisplayName = "Z Above Stuck Enemy")*/ { 50.f };

	/* -- GRAPPLE CAMERA VARIABLES -- */
	/* Interpolation speed of the camera rotation during grapplehook Drag */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_InterpSpeed			/*UMETA(DisplayName = "Interpolation Speed")	*/	{ 3.f };

	/* Pitch adjustment for the camera rotation during the Pre_Launch of Grapple Drag  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag|Camera Rotation")
		float GrappleDrag_Camera_PitchPoint				/*UMETA(DisplayName = "Pitch Point")	*/			{ 20.f };

#pragma endregion //GrappleHook

/* ----------------------------------------- ATTACKS ----------------------------------------------- */
#pragma region Attacks
	#pragma region General
	EAttackState m_EAttackState = EAttackState::None;
	FTimerHandle TH_BufferAttack;
	FAttackActionBuffer Delegate_AttackBuffer;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|AttackContact")
		float AttackContactTimer{ 0.3f };
	FAttackContactDelegate AttackContactDelegate;

	/*	*	When attacking with the staff keep a list of actors hit during the attack
		*	The 'void Attack Contact Function' will only be called once per actor	
		*	This array is cleaned in 'void StopAttack'								*/
	TArray<AActor*> AttackContactedActors;
	void AttackContact(AActor* instigator, AActor* target);
	void AttackContact_Particles(FVector location, FQuat direction);

	// Time removed from
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|SmackAttack")
		float SmackAttack_GH_TimerRemoval{ 0.1f };

	UFUNCTION(BlueprintCallable)
		void Activate_AttackCollider() override;

	UFUNCTION(BlueprintCallable)
		void Deactivate_AttackCollider() override;

	void StartAttackBufferPeriod() override;
	void ExecuteAttackBuffer() override;
	void EndAttackBufferPeriod() override;

	void BufferDelegate_Attack(void(ASteikemannCharacter::* func)());

	bool CanBeAttacked() override;

	FVector AttackDirection{};

	FVector AttackColliderScale{};

	void Click_Attack();
	void UnClick_Attack();


	UFUNCTION(BlueprintImplementableEvent)
		void AttackSmack_Start();
	UFUNCTION(BlueprintCallable)
		void AttackSmack_Start_Pure();
	void AttackSmack_Start_Ground_Pure();


	UFUNCTION(BlueprintCallable)
		void Stop_Attack();

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
		float SmackAttack_Anticipation_Rate		/*UMETA(DisplayName = "1. Smack Anticipation Rate")*/ { 4.5f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|SmackAttack")
		float SmackAttack_Action_Rate			/*UMETA(DisplayName = "2. Smack Action Rate")*/ { 5.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|SmackAttack")
		float SmackAttack_Reaction_Rate			/*UMETA(DisplayName = "3. Smack Reaction Rate")*/ { 2.f };
	

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|ScoopAttack")
		float ScoopAttack_Anticipation_Rate		/*UMETA(DisplayName = "1. Scoop Anticipation Rate")*/ { 10.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|ScoopAttack")
		float ScoopAttack_Action_Rate			/*UMETA(DisplayName = "2. Scoop Action Rate")*/ { 7.f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|BasicAttacks|ScoopAttack")
		float ScoopAttack_Reaction_Rate			/*UMETA(DisplayName = "3. Scoop Reaction Rate")*/ { 2.f };
	#pragma endregion //General
	
	#pragma region SmackAttack
	bool bAttackPress{};

	/* SMACK DIRECTION 
	 *  0. None of the below
	 *  1. Based on input 
	 *  2. Based on camera direction
	 *  3. Mixture of both 
	 * Currently no aiming method outside of this */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|SmackDirection", meta = (UIMin = "0", UIMax = "3"))
		uint8 SmackDirectionType{ 1 };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|SmackDirection", meta = (UIMin = "1.0", UIMax = "4.0", EditCondition = "SmackDirectionType == 2 || SmackDirectionType == 3", EditConditionHides))
		float SmackDirection_CameraMultiplier	/*UMETA(DisplayName = "Camera Multiplier")*/ { 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|SmackDirection", meta = (UIMin = "1.0", UIMax = "4.0", EditCondition = "SmackDirectionType == 1 || SmackDirectionType == 3", EditConditionHides))
		float SmackDirection_InputMultiplier	/*UMETA(DisplayName = "Input Multiplier")*/ { 1.f };

	
	/* The angle from the ground the enemy will be smacked. 0 degrees: Is parallel to the ground. 90 degrees: Is directly upwards */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float SmackUpwardAngle{ 30.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float SmackAttackStrength{ 1500.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks", meta = (UIMin = "0", UIMax = "1"))
		float SmackAttack_InputAngleMultiplier			/*UMETA(DisplayName = "Input Angle Multiplier")*/ { 0.2 };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks", meta = (UIMin = "0", UIMax = "1"))
		float SmackAttack_InputStrengthMultiplier		/*UMETA(DisplayName = "Input Strength Multiplier")*/ { 0.2 };

	/* ---- Moving Character During SmackAttack ---- */
	/* How far the character will move forward during Smack Attack. Happens during The Action when the collider is active */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		float SmackAttackMovementLength{ 100.f };

	bool bSmackAttackMoveCharacter{};
	void SmackAttackMoveCharacter(float DeltaTime);

	// Combo
	int AttackComboCount{};
	UFUNCTION(BlueprintImplementableEvent)
		void ComboAttack(int combo);
	void ComboAttack_Pure();

	UFUNCTION(BlueprintCallable)
		void Activate_SmackAttack();
	UFUNCTION(BlueprintCallable)
		void Deactivate_SmackAttack();

	bool IsSmackAttacking() const;

	bool bCanBeSmackAttacked{ true };


	void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;
	void Receive_SmackAttack_Pure(const FVector& Direction, const float& AttackStrength) override;

	bool GetCanBeSmackAttacked() const override { return bCanBeSmackAttacked; }
	void ResetCanBeSmackAttacked() override { bCanBeSmackAttacked = true; }

	#pragma endregion //SmackAttack

	#pragma region ScoopAttack
	bool bClickScoopAttack{};

	UFUNCTION(BlueprintImplementableEvent)
		void Start_ScoopAttack();
	UFUNCTION(BlueprintCallable)
		void Start_ScoopAttack_Pure();
	void Start_ScoopAttack_Ground_Pure();
	void Click_ScoopAttack();
	void UnClick_ScoopAttack();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		float ScoopJump_Hangtime{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks|Movement")
		float ScoopJump_Canceled_Hangtime{ 0.2f };
	FHeightReached HeightReachedDelegate;
	FTimerHandle TH_ScoopJumpGravityEnable;
	AActor* ScoopedActor{ nullptr };
	float Jump_HeightToReach{};
	bool HasReachedPostScoopedJumpHeight() const;
	void PostScoopJump();
	void Disable_PostScoopJumpGravity();

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

	/* How far above the player will the scooped target go */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float ScoopHeight{ 200.f };
	/*	*	How much time per 100 units should the enemy take. 
		*	Goes into the physics calculation of the Impulse launch on the enemy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float ScoopHeightTimeRatio{ 0.15f };

	/* Length along the players forward vector the target will be scooped towards */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|BasicAttacks")
		float ScoopForwardLength{ 100.f };

	void Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;
	//void Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength) override;

	#pragma endregion //ScoopAttack

	#pragma region GroundPound

	bool bGroundPoundPress{};
	UFUNCTION(BlueprintCallable)
		bool IsGroundPounding() const { return m_EState == EState::STATE_Attacking && m_EAttackState == EAttackState::GroundPound; }

	void Click_GroundPound();
	void UnClick_GroundPound();

	/* Movement */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GroundPound")
		float GP_PrePoundAirtime{ 0.3f };
	/* How fast, visually, the player will launch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|GroundPound")
		float GP_VisualLaunchStrength{ 2500.f };
	/* The launch strength the enemies will recieve */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|GroundPound")
		float GP_LaunchStrength{ 2500.f };
	/* How long the movement will be locked after landing with ground pound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|GroundPound")
		float GP_MovementPeriodLocked{ 1.f };
	FTimerHandle THandle_GPHangTime;


	void Launch_GroundPound();

	UFUNCTION(BlueprintImplementableEvent)
		void Anim_GroundPound_Initial();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void Anim_GroundPound_Land_Ground();

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
