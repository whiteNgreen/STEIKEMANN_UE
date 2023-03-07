// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharWaterFloatComponent.h"

#include "../DebugMacros.h"

#include "Components/CapsuleComponent.h"
#include "GameFrameWork/Character.h"
#include "../GameplayTags.h"
#include "../StaticActors/WaterPuddle.h"
#include "../AbstractClasses/AbstractCharacter.h"

// Sets default values for this component's properties
UBaseCharWaterFloatComponent::UBaseCharWaterFloatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBaseCharWaterFloatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	m_Owner = Cast<ABaseCharacter>(GetOwner());
	if (m_Owner) {
		m_Owner->GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &UBaseCharWaterFloatComponent::OnOwnerCapsuleOverlapWithWater);
		m_Owner->GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this, &UBaseCharWaterFloatComponent::OnOwnerCapsuleEndOverlapWithWater);
	}
}


// Called every frame
void UBaseCharWaterFloatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UBaseCharWaterFloatComponent::OnOwnerCapsuleOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == GetOwner()) return;
	if (!OtherComp->IsA(UBoxComponent::StaticClass())) return;
	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	if (ITag->HasMatchingGameplayTag(Tag::WaterPuddle()))
	{
		
	}
}

void UBaseCharWaterFloatComponent::OnOwnerCapsuleEndOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == GetOwner()) return;
	if (!OtherComp->IsA(UBoxComponent::StaticClass())) return;
	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	if (ITag->HasMatchingGameplayTag(Tag::WaterPuddle()))
	{

	}

}



