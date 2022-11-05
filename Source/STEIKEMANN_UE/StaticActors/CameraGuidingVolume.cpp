// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/CameraGuidingVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"

// Sets default values
ACameraGuidingVolume::ACameraGuidingVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RootVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("Root Volume"));
	RootVolume->SetupAttachment(RootComponent);

	PointFocus = CreateDefaultSubobject<UBoxComponent>(TEXT("PointFocus"));
	PointFocus->SetupAttachment(RootComponent);

	//SplineFocus = CreateDefaultSubobject<USplineComponent>(TEXT("SplineFocus"));
	//SplineFocus->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ACameraGuidingVolume::BeginPlay()
{
	Super::BeginPlay();

	RootVolume->OnComponentBeginOverlap.AddDynamic(this, &ACameraGuidingVolume::OnVolumeBeginOverlap);
	RootVolume->OnComponentEndOverlap.AddDynamic(this, &ACameraGuidingVolume::OnVolumeEndOverlap);
	
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
		ICameraGuideInterface* CamI = Cast<ICameraGuideInterface>(OtherActor);
		if (CamI){
			Point.Obj = this;	// ptr til root objektet eller til component?
			Point.Priority = 0;
			Point.Weight = 1.f;
			Point.Location = PointFocus->GetComponentLocation();
			CamI->AddCameraGuide(Point);
		}
	}
}

void ACameraGuidingVolume::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != this){
		ICameraGuideInterface* CamI = Cast<ICameraGuideInterface>(OtherActor);
		if (CamI){
			CamI->RemoveCameraGuide(this);
		}
	}
}

