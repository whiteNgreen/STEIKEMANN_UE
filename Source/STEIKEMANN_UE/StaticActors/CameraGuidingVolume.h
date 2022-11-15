// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Base/BaseStaticActor.h"
#include "../Interfaces/CameraGuideInterface.h"

/* Always last */
#include "CameraGuidingVolume.generated.h"



UCLASS()
class STEIKEMANN_UE_API ACameraGuidingVolume : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACameraGuidingVolume();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:	 	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class UBoxComponent* CameraVolume{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Focus")
		EFocusType CameraFocus;


	/* Focus Points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		class UPrimitiveComponent* FocusComponent{ nullptr };


	UPROPERTY(EditInstanceOnly, Category = "FocusPoint")
		FocusPoint Point;

	UFUNCTION()
		void OnVolumeBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

};
