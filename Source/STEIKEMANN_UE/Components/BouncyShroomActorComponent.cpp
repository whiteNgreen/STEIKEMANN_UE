// Fill out your copyright notice in the Description page of Project Settings.


#include "BouncyShroomActorComponent.h"
#include "../DebugMacros.h"

#include "Components/CapsuleComponent.h"
#include "GameFrameWork/Character.h"
#include "../GameplayTags.h"
#include "../StaticActors/BouncyShroom.h"
#include "../AbstractClasses/AbstractCharacter.h"

// Sets default values for this component's properties
UBouncyShroomActorComponent::UBouncyShroomActorComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBouncyShroomActorComponent::BeginPlay()
{
	Super::BeginPlay();

	m_Owner = Cast<ABaseCharacter>(GetOwner());
	ACharacter* charOwner = Cast<ACharacter>(m_Owner);
	if (charOwner) {
		charOwner->GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &UBouncyShroomActorComponent::OnOwnerCapsuleHitShroom);
	}
}


// Called every frame
void UBouncyShroomActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UBouncyShroomActorComponent::OnOwnerCapsuleHitShroom(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == GetOwner()) return;

	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	if (ITag->HasMatchingGameplayTag(Tag::BouncyShroom()))
	{
		//if (!HitComponent->IsA(UBoxComponent::StaticClass())) return;
		PRINTPARLONG(2.f, "BOUNCE: %s -> %s", *m_Owner->GetName(), *OtherActor->GetName());

		ABouncyShroom* Shroom = Cast<ABouncyShroom>(OtherActor);
		FVector direction;
		float strength = BounceStrength;
		//Shroom->GetBounceInfo(NormalImpulse, direction, strength);
		if (Shroom->GetBounceInfo(NormalImpulse, direction, strength)) 
		{
			m_Owner->ShroomBounce(direction, strength);

			PRINTPARLONG(2.f, "BOUNCE: %s", *m_Owner->GetName());
			DrawDebugLine(GetWorld(), m_Owner->GetActorLocation(), m_Owner->GetActorLocation() + NormalImpulse * 200.f, FColor::Blue, false, 3.f, 0, 3.f);
			DrawDebugLine(GetWorld(), m_Owner->GetActorLocation(), m_Owner->GetActorLocation() + direction * 100.f, FColor::Orange, false, 3.f, -1, 5.f);
		}
	}
}

