// Fill out your copyright notice in the Description page of Project Settings.


#include "../StaticActors/GrappleTarget.h"
#include "../GameplayTags.h"

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
	
	//GrappleTargetType = &Tag_GrappleTarget_Static;
	//GrappleType = &Tag_GrappleTarget_Static;

	//Tag_Player = FGameplayTag::RequestGameplayTag("Pottit");
	//Tag_Enemy = FGameplayTag::RequestGameplayTag("Enemy");
	//Tag_EnemyAubergineDoggo = FGameplayTag::RequestGameplayTag("Enemy.AubergineDoggo");
	//Tag_GrappleTarget = FGameplayTag::RequestGameplayTag("GrappleTarget");
	//Tag_GrappleTarget_Static = FGameplayTag::RequestGameplayTag("GrappleTarget.Static");
	//Tag_GrappleTarget_Dynamic = FGameplayTag::RequestGameplayTag("GrappleTarget.Dynamic");

	TagsContainer.AddTag(GetTag_GrappleTarget_Static());
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

