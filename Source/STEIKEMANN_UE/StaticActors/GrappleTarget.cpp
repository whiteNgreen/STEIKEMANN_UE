// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/GrappleTarget.h"

// Sets default values
AGrappleTarget::AGrappleTarget()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGrappleTarget::BeginPlay()
{
	Super::BeginPlay();
	
	GrappleTargetType = Tag_GrappleTarget_Static;

	TagsContainer.AddTag(GrappleTargetType);
}

// Called every frame
void AGrappleTarget::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AGrappleTarget::TargetedPure()
{
	Execute_Targeted(this);
}

void AGrappleTarget::UnTargetedPure()
{
	Execute_UnTargeted(this);
}

void AGrappleTarget::InReach_Pure()
{
	Execute_InReach(this);
}

void AGrappleTarget::OutofReach_Pure()
{
	Execute_OutofReach(this);
}



void AGrappleTarget::HookedPure()
{
	Execute_Hooked(this);
}

void AGrappleTarget::UnHookedPure()
{
	Execute_UnHooked(this);
}

