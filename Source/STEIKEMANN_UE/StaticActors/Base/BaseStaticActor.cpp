// Fill out your copyright notice in the Description page of Project Settings.


#include "../../StaticActors/Base/BaseStaticActor.h"

// Sets default values
ABaseStaticActor::ABaseStaticActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

// Called when the game starts or when spawned
void ABaseStaticActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABaseStaticActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TimerManager.Tick(DeltaTime);
}


