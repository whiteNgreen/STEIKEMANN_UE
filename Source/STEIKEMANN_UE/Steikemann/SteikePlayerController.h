// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SteikePlayerController.generated.h"


UENUM(BlueprintType)
enum class EInputType : uint8
{
	MouseNKeyboard,
	Gamepad
};

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API ASteikePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* pawn) override;

	UPROPERTY(BlueprintReadWrite)
		class ASteikemannCharacter* PlayerCharacter{ nullptr };
	UPROPERTY(BlueprintReadWrite)
		EInputType m_EInputType;
	UFUNCTION(BlueprintCallable)
		void CheckInputType(FKey key);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void ChangedInputType();

};
