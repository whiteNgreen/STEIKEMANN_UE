// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/AttackInterface.h"
#include "../Interfaces/GrappleTargetInterface.h"
#include "../DebugMacros.h"
#include "GameplayTagAssetInterface.h"
#include "../WallDetectionComponent.h"
//#include "../GameplayTags.h"

#include "SmallEnemy.generated.h"

/************************ ENUMS *****************************/
UENUM()
enum class EGravityState : int8
{
	Default,
	LerpToDefault,

	None,
	LerpToNone,
	ForcedNone
};
// State
UENUM()
enum class EEnemyState : int8
{
	STATE_None,

	STATE_OnGround,
	STATE_InAir,

	STATE_OnWall
};
// Wall Mechanic
UENUM()
enum class EWall : int8
{
	WALL_None,

	WALL_Stuck,

	WALL_Leaving
};

UCLASS()
class STEIKEMANN_UE_API ASmallEnemy : public ACharacter,
	public IAttackInterface,
	public IGameplayTagAssetInterface,
	public IGrappleTargetInterface
{
	GENERATED_BODY()
#pragma region Base
public:
	// Sets default values for this character's properties
	ASmallEnemy();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
#pragma endregion //Base

#pragma region GameplayTags
	UPROPERTY(BlueprintReadOnly, Category = "GameplayTags")
		FGameplayTagContainer GameplayTags;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override { TagContainer = GameplayTags; return; }
#pragma endregion //GameplayTags

#pragma region States
public:	// STATES
	EEnemyState m_State = EEnemyState::STATE_None;
	EGravityState m_Gravity = EGravityState::Default;
	void SetDefaultState();

private: // Gravity
	float GravityScale;

#pragma endregion //States
	void RotateActorYawToVector(FVector AimVector, float DeltaTime = 0);

#pragma region Wall Mechanics
public:
	UWallDetectionComponent* WallDetector{ nullptr };
	EWall m_WallState = EWall::WALL_None;
public:
	void StickToWall();
	virtual bool IsStuck_Pure() override { return m_State == EEnemyState::STATE_OnWall; }

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall")
		bool bWDC_Debug{};


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_Capsule_Radius{ 40.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_Capsule_Halfheight{ 90.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|OnWall|WallDetection")
		float WDC_MinHeight{ 20.f };
private:
	Wall::WallData m_WallData;
	Wall::WallData m_WallJumpData;

#pragma endregion //Wall Mechanics


#pragma region GrappleHooked
public: 
	bool bCanBeGrappleHooked{ true };
	/* The internal cooldown before enemy can be grapplehooked again */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		float GrappleHookedInternalCooldown{ 0.5f };

	FTimerHandle Handle_GrappledCooldown;
	void ResetCanBeGrappleHooked() { bCanBeGrappleHooked = true; }

	/* Choice between the first and second grapplelaunch method */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook")
		bool bUseFirstGrappleLaunchMethod{ true };

	/* When grapplehooked by the player, launch towards them with this strength : 1st method */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook", meta = (EditCondition = "bUseFirstGrappleLaunchMethod", EditConditionHides))
		float GrappledLaunchStrength{ 1000.f };

	/* When grapplehooked by the player, launch them upwards of this angle : 1st method*/	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook", meta = (EditCondition = "bUseFirstGrappleLaunchMethod", EditConditionHides))
		float GrappledLaunchAngle{ 45.f };

	/* Time it should take to reach the Grappled Instigator : 2nd method */	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook", meta = (EditCondition = "!bUseFirstGrappleLaunchMethod", EditConditionHides))
		float GrappledLaunchTime{ 1.f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|GrappleHook", meta = (EditCondition = "!bUseFirstGrappleLaunchMethod", EditConditionHides))
		float GrappledZInstigator{ 50.f };

	/* ----- Grapple Interface ------ */
	virtual void TargetedPure() override;

	virtual void UnTargetedPure() override;

	virtual void InReach_Pure() override;

	virtual void OutofReach_Pure() override;

	virtual void HookedPure() override;
	virtual void HookedPure(const FVector InstigatorLocation, bool OnGround,bool PreAction = false) override;

	virtual void UnHookedPure() override;

	//virtual FGameplayTag GetGrappledGameplayTag_Pure() const override { return Enemy; }
#pragma endregion //GrappleHooked

#pragma region GettingSmacked
public:
	bool bCanBeSmackAttacked{ true };

	FTimerHandle THandle_GotSmackAttacked{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|SmackAttack")
		float SmackAttack_OnGroundMultiplication{ 0.1f };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|SmackAttack")
		float SmackAttack_InternalTimer{ 0.5f };

	/**
	*	Sets a timer before character can be damaged again 
	*	With respect to the specific handle it should use */
	void WaitBeforeNewDamage(FTimerHandle TimerHandle, float Time);	
	
	bool CanBeAttacked() override;

	void Do_SmackAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting SmackAttacked
	void Receive_SmackAttack_Pure(const FVector& Direction, const float& Strength) override;
	bool GetCanBeSmackAttacked() const override { return bCanBeSmackAttacked; }
	void ResetCanBeSmackAttacked() override { bCanBeSmackAttacked = true; }


	void Do_ScoopAttack_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override;	// Getting Scooped
	void Receive_ScoopAttack_Pure(const FVector& Direction, const float& Strength) override;

	void Do_GroundPound_Pure(IAttackInterface* OtherInterface, AActor* OtherActor) override {}
	void Receive_GroundPound_Pure(const FVector& PoundDirection, const float& GP_Strength) override;

#pragma endregion //GettingSmacked
};
