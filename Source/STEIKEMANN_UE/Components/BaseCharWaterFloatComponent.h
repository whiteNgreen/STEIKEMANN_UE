// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BaseCharWaterFloatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class STEIKEMANN_UE_API UBaseCharWaterFloatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBaseCharWaterFloatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


public:
	/* How far below the water level is considered close to the surface */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WL_CloseToSurface{ 6.f };
	/** 
	* How far below the water level will it begin to lerp towards being close to the surface 
	* Lerping between the two methods of Gaussian(close to surface) and exponential 
	*	methods of adding force.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WL_CloseToSurface_Lerplength{ 12.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WL_HorizontalSlowDownStrength{ 4.f };

	void FloatingInWater(float DeltaTime);

	UFUNCTION()
		void OnOwnerCapsuleOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnOwnerCapsuleEndOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	class ABaseCharacter* m_Owner;
	class UCharacterMovementComponent* m_CharMovement;
	bool bIsFloatingInWater{};
	float WaterLevel{};
	float m_OwnerGravity;

	float m_VelMulti{};
};
