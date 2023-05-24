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
	if (bInReach) return;
	bInReach = true;
	Execute_InReach(this);
}

void AGrappleTarget::OutofReach_Pure()
{
	if (!bInReach) return;
	bInReach = false;
	Execute_OutofReach(this);
}



void AGrappleTarget::HookedPure()
{
	bInReach = true;
	Execute_Hooked(this);
}

void AGrappleTarget::UnHookedPure()
{
	Execute_UnHooked(this);
}

