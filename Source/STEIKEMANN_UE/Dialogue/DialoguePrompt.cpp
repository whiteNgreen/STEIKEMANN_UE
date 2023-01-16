// Fill out your copyright notice in the Description page of Project Settings.


#include "DialoguePrompt.h"
#include "../DebugMacros.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "../GameplayTags.h"
#include "Kismet/GameplayStatics.h"

#include "../Steikemann/SteikemannCharacter.h"


// Sets default values
ADialoguePrompt::ADialoguePrompt()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;

	Volume = CreateDefaultSubobject<UBoxComponent>("Volume");
	Volume->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void ADialoguePrompt::BeginPlay()
{
	Super::BeginPlay();

	Volume->OnComponentBeginOverlap.AddDynamic(this, &ADialoguePrompt::OnVolumeBeginOverlap);
	Volume->OnComponentEndOverlap.AddDynamic(this, &ADialoguePrompt::OnVolumeEndOverlap);
}

// Called every frame
void ADialoguePrompt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADialoguePrompt::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TFunction<void()> playerCollision = [this]() {
		bPlayerWithinVolume = true;
		auto player = (ASteikemannCharacter*)m_Player;
		m_PlayerCamera = player->Camera;
		ShowPrompt();
	};

	if (!OtherComp->IsA(UCapsuleComponent::StaticClass())) return;
	if (OtherActor == m_Player)
	{
		playerCollision();
		return;
	}

	auto ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	FGameplayTagContainer tags;
	ITag->GetOwnedGameplayTags(tags);
	if (tags.HasTag(Tag::Player()))
	{
		m_Player = OtherActor;
		playerCollision();
		return;
	}
}

void ADialoguePrompt::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != m_Player) return;
	if (!OtherComp->IsA(UCapsuleComponent::StaticClass())) return;

	EndPrompt();
}
