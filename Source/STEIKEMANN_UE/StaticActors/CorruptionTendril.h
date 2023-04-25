// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "CorruptionTendril.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API ACorruptionTendril : public ABaseStaticActor
{
	GENERATED_BODY()

public:
	ACorruptionTendril();
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void TendrilPulse_Start();
	UFUNCTION(BlueprintImplementableEvent)
		void DestroyTendril_Start(FVector CoreLocation);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
		void DestroyTendril_End();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectedMeatWall")
		bool bIsUsedToDestroyMeatWall{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectedMeatWall", meta = (EditCondition = "bIsUsedToDestroyMeatWall", EditConditionHides))
		TArray<class ACorruptionWall*> ConnectedWalls;

protected:
	virtual void BeginPlay() override;

public: // Pulse Timeline
	UPROPERTY(BlueprintReadWrite)
		class UTimelineComponent* TLComp_Pulse;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UCurveFloat* Curve_Pulse;
	UFUNCTION(BlueprintImplementableEvent)
		void TL_Pulse(float value);
	UFUNCTION(BlueprintImplementableEvent)
		void TL_Pulse_End();

public: // Death/Fade Timeline
	UPROPERTY(BlueprintReadWrite)
		class UTimelineComponent* TLComp_Fade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UCurveFloat* Curve_Fade;
	UFUNCTION(BlueprintImplementableEvent)
		void TL_Fade(float value);
	UFUNCTION(BlueprintImplementableEvent)
		void TL_Fade_End();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ConnectedMeatWall", meta = (EditCondition = "bIsUsedToDestroyMeatWall", EditConditionHides))
		float ActivateMeatWall_FadeTimer{ 10.f };
};
