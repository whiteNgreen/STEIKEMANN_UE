// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/CameraGuidingVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ACameraGuidingVolume::ACameraGuidingVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bRunConstructionScriptOnDrag = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("ROOT"));
	RootComponent = Root;

	CameraVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Root Volume"));
	CameraVolume->SetupAttachment(Root);

	//PointFocus = CreateDefaultSubobject<UBoxComponent>(TEXT("PointFocus"));
	//PointFocus->SetupAttachment(RootComponent);

	//SplineFocus = CreateDefaultSubobject<USplineComponent>(TEXT("SplineFocus"));
	//SplineFocus->SetupAttachment(RootComponent);
}



// Called when the game starts or when spawned
void ACameraGuidingVolume::BeginPlay()
{
	Super::BeginPlay();

	CameraVolume->OnComponentBeginOverlap.AddDynamic(this, &ACameraGuidingVolume::OnVolumeBeginOverlap);
	CameraVolume->OnComponentEndOverlap.AddDynamic(this, &ACameraGuidingVolume::OnVolumeEndOverlap);
	
	GameplayTags.AddTag(TAG_CameraVolume());
	//GameplayTags.AddTag(FGameplayTag::RequestGameplayTag("CameraVolume"));
}

// Called every frame
void ACameraGuidingVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ACameraGuidingVolume::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor != this)
	{
		UCapsuleComponent* cap = Cast<UCapsuleComponent>(OtherComp);
		if (cap) {
			ICameraGuideInterface* CamI = Cast<ICameraGuideInterface>(OtherActor);
			if (CamI){
				Point.ParentObj = this;	// ptr til root objektet eller til component?
				switch (CameraFocus)
				{
				case EFocusType::FOCUS_Point:
					Point.FocusBox = Cast<UBoxComponent>(FocusComponent);
					Point.ComponentType = EFocusType::FOCUS_Point;
					break;
				case EFocusType::FOCUS_Spline:
					Point.FocusSpline = Cast<USplineComponent>(FocusComponent);
					Point.ComponentType = EFocusType::FOCUS_Spline;
					Point.SplineInputKey = Point.FocusSpline->FindInputKeyClosestToWorldLocation(OtherActor->GetActorLocation());
					CamI->SetSplineInputkey(Point.SplineInputKey);
					break;
				default:
					break;
				}

				Point.ComponentLocation = FocusComponent->GetComponentLocation();
				CamI->AddCameraGuide(Point);
			}
		}
	}
}

void ACameraGuidingVolume::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != this){
		UCapsuleComponent* cap = Cast<UCapsuleComponent>(OtherComp);
		if (cap) {
			ICameraGuideInterface* CamI = Cast<ICameraGuideInterface>(OtherActor);
			if (CamI) {
				CamI->RemoveCameraGuide(this);
			}
		}
	}
}

