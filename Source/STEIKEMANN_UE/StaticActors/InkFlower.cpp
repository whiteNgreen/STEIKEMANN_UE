// Fill out your copyright notice in the Description page of Project Settings.


#include "InkFlower.h"
#include "../Steikemann/SteikemannCharacter.h"
#include "Components/CapsuleComponent.h"

AInkFlower::AInkFlower()
{
	SphereCollider = CreateDefaultSubobject<USphereComponent>("SphereCollider");
	SphereCollider->SetupAttachment(Root);
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("Mesh");
	Mesh->SetupAttachment(Root);
	BeakMesh = CreateDefaultSubobject<USkeletalMeshComponent>("BeakMesh");
	BeakMesh->SetupAttachment(Mesh);
	CollectibleCollider = CreateDefaultSubobject<USphereComponent>("CollectibleCollider");
	CollectibleCollider->SetupAttachment(BeakMesh);
}

void AInkFlower::BeginPlay()
{
	Super::BeginPlay();
	GTagContainer.AddTag(Tag::InkFlower());

	CollectibleCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollectibleCollider->OnComponentBeginOverlap.AddDynamic(this, &AInkFlower::OnCollectibleBeginOverlap);

}

void AInkFlower::SpawnInkCollectible()
{
	CollectibleCollider->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SphereCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void AInkFlower::OnCollectibleBeginOverlap(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (OtherActor == this) return;
	IGameplayTagAssetInterface* ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	/**
	* Only collide with player
	*/
	if (ITag->HasMatchingGameplayTag(Tag::Player()) && OtherComp->IsA(UCapsuleComponent::StaticClass())) {
		PlayerCollectInk(OtherActor);

		BeakMesh->SetVisibility(false);
		CollectibleCollider->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		FTimerHandle h;
		GetWorldTimerManager().SetTimer(h, [this]() { Destroy(); }, 1.5f, false);
	}
}

void AInkFlower::PlayerCollectInk(AActor* player)
{
	Collected();
	auto p = Cast<ASteikemannCharacter>(player);
	if (p)
		p->Pickup_InkFlower();
}

void AInkFlower::Gen_ReceiveAttack(const FVector Direction, const float Strength, EAttackType AType, const float Delaytime)
{
	if (!CanBeAttacked()) return;
	if (AType == EAttackType::Environmental) {
		Anim_Hit();
		return;
	}
	bAICanBeDamaged = false;

	Anim_Open();
}
