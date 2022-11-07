// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"
#include "../Interfaces/CameraGuideInterface.h"
#include "../GameplayTags.h"
#include "../DebugMacros.h"

/* Always last */
#include "CameraGuidingVolume.generated.h"



UCLASS()
class STEIKEMANN_UE_API ACameraGuidingVolume : public AActor,
	public IGameplayTagAssetInterface
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

	//UFUNCTION(BlueprintCallable)
	//	void RefreshComponents();

public:	 	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		USceneComponent* Root{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class UBoxComponent* CameraVolume{ nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Focus")
		EFocusType CameraFocus;

	UPROPERTY(EditAnywhere, Category = "TESTING")
		bool bTesting{ true };
	UPROPERTY(EditAnywhere, Category = "TESTING", meta = (EditCondition = "!bTesting", EditConditionHides))	// Gjemmer T basert på om bTesting er false eller true
		float T{ 1.f };

	/* Focus Points */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
		class UPrimitiveComponent* FocusComponent{ nullptr };
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		//class UBoxComponent* PointFocus{ nullptr };
	//UPROPERTY(EditAnywhere, Category = "Components")
		//class USplineComponent* SplineFocus{ nullptr };



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FocusPoint")
		FocusPoint Point;

	UFUNCTION()
		void OnVolumeBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


	/* GameplayTags */
	FGameplayTagContainer GameplayTags;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override{ TagContainer = GameplayTags; }

};
