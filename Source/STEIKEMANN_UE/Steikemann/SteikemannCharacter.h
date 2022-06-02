// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Interfaces/GrappleTargetInterface.h"

#include "SteikemannCharacter.generated.h"

#define GRAPPLE_HOOK ECC_GameTraceChannel1



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

	//TUniquePtr<class USteikemannCharMovementComponent> MovementComponent;
	//TWeakObjectPtr<class USteikemannCharMovementComponent> MovementComponent;
	class USteikemannCharMovementComponent* MovementComponent{ nullptr };

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



private:
	UPROPERTY(EditAnywhere, Category = "Movement|Walk/Run", meta = (AllowPrivateAcces = "true"))
	float TurnRate{ 50.f };

	void MoveForward(float value);
	void MoveRight(float value);
	void TurnAtRate(float rate);
	void LookUpAtRate(float rate);

	void Jump() override;
	void StopJumping() override;
	void CheckJumpInput(float DeltaTime) override;
public:
	bool CanDoubleJump() const;

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
	UPROPERTY(Editanywhere, BlueprintReadWrite, Category = "Movement|GrappleHook")
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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Swing", meta = (AllowPrivateAcces = "true"))
		float GrapplingHook_InitialBoost{ 1000.f };
	/* Initial length between actor and grappled object */
	float GrappleRadiusLength;

	/* Grapplehook swing */
	UFUNCTION(BlueprintCallable)
	void Initial_GrappleHook_Swing();
	void Update_GrappleHook_Swing();


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_PreLaunch_Timer_Length{ 0.5f };
	float GrappleDrag_PreLaunch_Timer{};
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_Initial_Speed{ 500.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Grappling Hook|Drag", meta = (AllowPrivateAcces = "true"))
		float GrappleDrag_Acceleration_Speed{ 10.f };

	float GrappleDrag_CurrentSpeed{};

	/* Grapplehook drag during swing */
	UFUNCTION(BlueprintCallable)
	void Initial_GrappleHook_Drag(float DeltaTime);
	void Update_GrappleHook_Drag(float DeltaTime);

	UPROPERTY(BlueprintReadWrite)
	bool bGrappleEnd{};

	UFUNCTION(BlueprintCallable)
	bool IsGrappling();
};
