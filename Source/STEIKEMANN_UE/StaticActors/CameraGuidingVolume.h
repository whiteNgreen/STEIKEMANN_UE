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

UENUM(BlueprintType)
enum class EFocus : uint8
{
	FOCUS_None		UMETA(DisplayName = "None"),
	FOCUS_Point		UMETA(DisplayName = "Point"),
	FOCUS_Line		UMETA(DisplayName = "Line"),
	FOCUS_Curve		UMETA(DisplayName = "Curve")
};

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


public:	 	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class UBoxComponent* RootVolume{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera Focus")
		EFocus CameraFocus;

	/* Focus Points */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		class UBoxComponent* PointFocus{ nullptr };
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
		//class USplineComponent* SplineFocus{ nullptr };

	FocusPoint Point;

	UFUNCTION()
		void OnVolumeBeginOverlap(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
		void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);


	/* GameplayTags */
	FGameplayTagContainer GameplayTags;
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override{ TagContainer = GameplayTags; }

};
