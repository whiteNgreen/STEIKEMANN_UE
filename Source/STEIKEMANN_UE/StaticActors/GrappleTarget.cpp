// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/GrappleTarget.h"
#include "../GameplayTags.h"

// Sets default values
AGrappleTarget::AGrappleTarget()
{

}

// Called when the game starts or when spawned
void AGrappleTarget::BeginPlay()
{
	Super::BeginPlay();
	
	GTagContainer.AddTag(Tag::GrappleTarget_Static());
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
	bOpen = true;
}

void AGrappleTarget::OutofReach_Pure()
{
	Execute_OutofReach(this);
	bOpen = false;
}



void AGrappleTarget::HookedPure()
{
	Execute_Hooked(this);
}

void AGrappleTarget::UnHookedPure()
{
	Execute_UnHooked(this);
}

