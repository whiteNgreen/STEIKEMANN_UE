// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharWaterFloatComponent.h"

#include "../DebugMacros.h"

#include "Components/CapsuleComponent.h"
#include "GameFrameWork/Character.h"
#include "GameFrameWork/CharacterMovementComponent.h"
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
		m_Owner->GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this,	&UBaseCharWaterFloatComponent::OnOwnerCapsuleOverlapWithWater);
		m_Owner->GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this,		&UBaseCharWaterFloatComponent::OnOwnerCapsuleEndOverlapWithWater);
		m_CharMovement = m_Owner->GetCharacterMovement();
	}
}


// Called every frame
void UBaseCharWaterFloatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (bIsFloatingInWater && m_CharMovement) {
		FloatingInWater();
	}
}

void UBaseCharWaterFloatComponent::FloatingInWater()
{
	PRINTPAR("VelocityZ: %f", m_CharMovement->Velocity.Z);
	float ZbelowWaterLevel = (WaterLevel + WaterLevelAdditional) - m_Owner->GetActorLocation().Z;
	float Zwaterleveldivide = -ZbelowWaterLevel / WaterLevelDivide;

	//float VelDivide = m_CharMovement->Velocity.Z / VelZdivideAmount;
	//float VelMultiplied = VelDivide > 0.f ? Gaussian(VelDivide, 2, 1.f) : VelDivide * VelDivide * 0.4f + 1.f;
	//PRINTPAR("Gaussian: %f", VelMultiplied);
	float a = 2.f;
	PRINTPAR("GravityZ: %f", -1.f * (m_Owner->m_BaseGravityZ * Bouyancy * (Gaussian(Zwaterleveldivide) * ((a - 1.f) / a))));
	m_CharMovement->AddForce(FVector(0, 0, -1.f * (m_Owner->m_BaseGravityZ * Bouyancy)));
	//m_CharMovement->AddForce(FVector(0, 0, -1.f * (m_Owner->m_BaseGravityZ * (ZbelowWaterLevel * Bouyancy / 100.f) + m_Owner->m_BaseGravityZ * VelMultiplied)));
}

void UBaseCharWaterFloatComponent::OnOwnerCapsuleOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == GetOwner()) return;
	if (!OtherComp->IsA(UBoxComponent::StaticClass())) return;
	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	/**
	* Enter water puddle
	*/
	if (ITag->HasMatchingGameplayTag(Tag::WaterPuddle()))
	{
		bIsFloatingInWater = true;
		WaterLevel = OtherActor->GetActorLocation().Z;
		m_Owner->Delegate_WaterPuddleEnter.ExecuteIfBound();
		m_CharMovement->AddImpulse(FVector(m_CharMovement->Velocity * -0.8f), true);
	}
}

void UBaseCharWaterFloatComponent::OnOwnerCapsuleEndOverlapWithWater(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == GetOwner()) return;
	if (!OtherComp->IsA(UBoxComponent::StaticClass())) return;
	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	/**
	* Exit water puddle
	*/
	if (ITag->HasMatchingGameplayTag(Tag::WaterPuddle()))
	{
		bIsFloatingInWater = false;
		m_Owner->Delegate_WaterPuddleExit.ExecuteIfBound();
	}

}



