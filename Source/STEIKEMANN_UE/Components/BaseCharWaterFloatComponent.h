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
	/**
	* How quicky the owning pawn will float up to water level. 
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float Bouyancy{ 0.8f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WaterLevelAdditional{ 50.f };
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		float WaterLevelDivide{ 200.f };
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		//float VelZdivideAmount{ 200.f };

	void FloatingInWater();

	UFUNCTION()
		void OnOwnerCapsuleOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnOwnerCapsuleEndOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	class ABaseCharacter* m_Owner;
	class UCharacterMovementComponent* m_CharMovement;
	bool bIsFloatingInWater{};
	float WaterLevel{};
};
