// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "DialoguePrompt.generated.h"

UENUM(BlueprintType)
enum class ECameraLerp : uint8
{
	None,
	First,
	Second
};

UCLASS()
class STEIKEMANN_UE_API ADialoguePrompt : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		USceneComponent* Root;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		USceneComponent* Prompt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UBoxComponent* Volume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UCameraComponent* Camera_One;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UCameraComponent* Camera_Two;

public:
	// Sets default values for this actor's properties
	ADialoguePrompt();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	AActor* m_Player{ nullptr };
	UPROPERTY(BlueprintReadOnly)
		bool bPlayerWithinVolume{};
	UPROPERTY(BlueprintReadOnly)
		UCameraComponent* m_PlayerCamera{ nullptr };

	UPROPERTY(BlueprintReadOnly)
		ECameraLerp m_ECameraLerp;
	UPROPERTY(BlueprintReadOnly)
		float CameraLerpSpeed{ 1.f };
	UPROPERTY(BlueprintReadOnly)
		uint8 m_PromptIndex{0};

public:	// Functions for activating prompt
	UFUNCTION()
		void OnVolumeBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintImplementableEvent)
		void ShowPrompt();
	UFUNCTION(BlueprintImplementableEvent)
		void EndPrompt();

	/* Common changes when going from one prompt index to another */
	UFUNCTION(BlueprintImplementableEvent)
		void PromptChange();
	void PromptChange_Pure();
public:	// Functions called by player
	bool GetNextPromptState(ASteikemannCharacter* player, int8 promptIndex = -1);

	UFUNCTION(BlueprintImplementableEvent)
		void ExitPrompt();
	void ExitPrompt_Pure();
};
