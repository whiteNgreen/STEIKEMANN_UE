// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "DialoguePrompt.generated.h"

UCLASS()
class STEIKEMANN_UE_API ADialoguePrompt : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		USceneComponent* Root;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UBoxComponent* Volume;
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

public:
	UFUNCTION()
		void OnVolumeBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintImplementableEvent)
		void ShowPrompt();
	UFUNCTION(BlueprintImplementableEvent)
		void EndPrompt();
};
