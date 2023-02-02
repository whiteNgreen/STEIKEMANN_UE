// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TutorialPrompt.generated.h"

UENUM(BlueprintType)
enum class ETutorialPrompt : uint8
{
	Jump,
	DoubleJump,

	Smack,
	Scoop,

	GrappleHook,

	Groundpound
};

UCLASS()
class STEIKEMANN_UE_API ATutorialPrompt : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		USceneComponent* Root;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UBoxComponent* Volume;
public:	
	// Sets default values for this actor's properties
	ATutorialPrompt();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	AActor* m_Player{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		ETutorialPrompt m_PromptType;
public:
	UFUNCTION()
		void OnVolumeBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ShowPrompt();
	UFUNCTION(BlueprintImplementableEvent)
		void EndPrompt();


	UFUNCTION(BlueprintImplementableEvent)
		void Prompt_Jump();
	UFUNCTION(BlueprintImplementableEvent)
		void Prompt_DoubleJump();
	UFUNCTION(BlueprintImplementableEvent)
		void Prompt_Smack();
	UFUNCTION(BlueprintImplementableEvent)
		void Prompt_Scoop();
	UFUNCTION(BlueprintImplementableEvent)
		void Prompt_Groundpound();
	UFUNCTION(BlueprintImplementableEvent)
		void Prompt_Grapplehook();
};
