// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseCharWaterFloatComponent.h"

#include "../DebugMacros.h"

#include "Components/CapsuleComponent.h"
#include "GameFrameWork/Character.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "../GameplayTags.h"
#include "../StaticActors/WaterPuddle.h"
#include "BaseClasses/AbstractClasses/AbstractCharacter.h"

// Sets default values for this component's properties
UBaseCharWaterFloatComponent::UBaseCharWaterFloatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UBaseCharWaterFloatComponent::BeginPlay()
{
	Super::BeginPlay();
	m_Owner = Cast<ABaseCharacter>(GetOwner());
	if (m_Owner) {
		m_Owner->GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this,	&UBaseCharWaterFloatComponent::OnOwnerCapsuleOverlapWithWater);
		m_Owner->GetCapsuleComponent()->OnComponentEndOverlap.AddDynamic(this,		&UBaseCharWaterFloatComponent::OnOwnerCapsuleEndOverlapWithWater);
		m_CharMovement = m_Owner->GetCharacterMovement();
		m_OwnerGravity = m_Owner->m_BaseGravityZ;
	}
}


// Called every frame
void UBaseCharWaterFloatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bIsFloatingInWater && m_CharMovement) {
		FloatingInWater(DeltaTime);
	}
}

void UBaseCharWaterFloatComponent::FloatingInWater(float DeltaTime)
{
	if (GetOwner()->GetActorLocation().Z > WaterLevel) return;
	
	float zmulti{ 1.f };
	float zdiff = WaterLevel - GetOwner()->GetActorLocation().Z;
	float velZ = m_CharMovement->Velocity.Z;

	/**
	* zmulti based on distance to from water level down to actor location
	* Negative Gaussian - 0,1,0
	*/
	float zmulti_strength = 2.f;
	zmulti = -FMath::Exp(-FMath::Pow(zdiff, 2) * 0.01) * zmulti_strength + 1.f + zmulti_strength;

	/**
	* Floating Upwards
	* Strength varies based on the objects speed
	*/
	float velm{ 1.f };
	if (velZ >= 0.f) {
		/**
		* When close to the surface
		* Positive Gaussian - 1,0,1
		*/
		if (zdiff <= WL_CloseToSurface) {
			velm = FMath::Exp(-FMath::Pow(velZ, 2) * 0.001f);
		}
		/**
		* Getting closer to the surface, linear interpolation between the functions
		*/
		else if (zdiff > WL_CloseToSurface && zdiff <= WL_CloseToSurface_Lerplength) {
			float one = FMath::Exp(-FMath::Pow(velZ, 2) * 0.001f);
			float two = FMath::Min(FMath::Pow(zdiff, 2) / 600.f, 2.f) - FMath::Min(FMath::Pow(velZ, 2) / 2000.f, 1.8f);
			float t = zdiff / WL_CloseToSurface_Lerplength;
			velm = one * t + (1.f - t) * two;
		}
		/**
		* Is far down in the water. Exponential strength with a ceiling.
		* To not overdo the effect when far down, the value is subtracted by the velocity.Z of the object
		*/
		else {
			velm = FMath::Min(FMath::Pow(zdiff, 2) / 600.f, 2.f) - FMath::Min(FMath::Pow(velZ, 2) / 2000.f, 1.8f);
		}
	}
	/**
	* Going down
	* Negative Gaussian
	*/
	else {
		velm = -FMath::Exp(-FMath::Pow(velZ, 2) * 0.01f) + 2.f;
	}
	m_VelMulti = FMath::FInterpTo(m_VelMulti, velm, DeltaTime, 9.f);

	m_CharMovement->AddForce(FVector(0, 0, -m_CharMovement->GetGravityZ() * m_CharMovement->Mass * zmulti * m_VelMulti));

	/**
	* Horizontal velocity change
	* Negative Gaussian
	*/
	FVector vel = m_CharMovement->Velocity;
	FVector negativeVel = vel * -1.f;
	negativeVel.X *= -FMath::Exp(-FMath::Pow(vel.X, 2) * 0.01f) * WL_HorizontalSlowDownStrength + WL_HorizontalSlowDownStrength + 1.f;
	negativeVel.Y *= -FMath::Exp(-FMath::Pow(vel.Y, 2) * 0.01f) * WL_HorizontalSlowDownStrength + WL_HorizontalSlowDownStrength + 1.f;
	m_CharMovement->AddForce(negativeVel * m_CharMovement->Mass);

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
		//m_Owner->Delegate_WaterPuddleEnter.ExecuteIfBound();
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
	if (ITag->HasMatchingGameplayTag(Tag::WaterPuddle())){
		bIsFloatingInWater = false;
	}

}



