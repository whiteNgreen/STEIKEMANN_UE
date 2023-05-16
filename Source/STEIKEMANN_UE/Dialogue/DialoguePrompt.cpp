// Fill out your copyright notice in the Description page of Project Settings.


#include "DialoguePrompt.h"
#include "../DebugMacros.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "../GameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

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
	Prompt = CreateDefaultSubobject<USceneComponent>("Prompt");
	Prompt->SetupAttachment(Volume);
	PlayerTransform = CreateDefaultSubobject<UArrowComponent>("Player Transform");
	PlayerTransform->SetupAttachment(Root);
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

void ADialoguePrompt::SceneComponentLookAt(USceneComponent* Comp, USceneComponent* Target)
{
	if (!Comp || !Target) {
		return;
	}
	FVector Direction = FVector(Target->GetComponentLocation() - Comp->GetComponentLocation()).GetSafeNormal();
	FRotator Rotation = Direction.Rotation();
	Comp->SetWorldRotation(Rotation);
}

float ADialoguePrompt::LerpSceneComponentTransformToSceneComponent(USceneComponent* Comp, USceneComponent* Target, float LerpAlpha, float DeltaTime)
{
	if (!Comp || !Target) return 0.f;
	float alpha = FMath::Clamp(LerpAlpha + (DeltaTime * CameraLerpSpeed), 0.f, 1.f);
	FTransform t1 = Comp->GetComponentTransform();
	FTransform t2 = Target->GetComponentTransform();
	FTransform NewTransform; 
	NewTransform.Blend(t1, t2, alpha);
	Comp->SetWorldTransform(NewTransform);
	return alpha;
}

void ADialoguePrompt::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TFunction<void()> playerCollision = [this]() 
	{
		bPlayerWithinVolume = true;
		auto player = (ASteikemannCharacter*)m_Player;
		m_PlayerCamera = player->Camera;
		player->EnterPromptArea(this, Prompt->GetComponentLocation());
		CameraLerpSpeed = player->CameraLerpSpeed_Prompt;
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
		m_Player = Cast<ASteikemannCharacter>(OtherActor);
		playerCollision();
		return;
	}
}

void ADialoguePrompt::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != m_Player) return;
	if (!OtherComp->IsA(UCapsuleComponent::StaticClass())) return;
	auto player = (ASteikemannCharacter*)m_Player;
	player->LeavePromptArea();
	EndPrompt();
}

void ADialoguePrompt::PromptChange_Pure()
{
	PromptChange();
}

void ADialoguePrompt::GetNextPromptState_Pure(ASteikemannCharacter* player, int promptIndex)
{
	if (!player)
		return;
	if (promptIndex != -1) 
		m_PromptIndex_Internal = promptIndex;
	GetNextPromptState(player, m_PromptIndex_Internal);
	PromptChange_Pure();
	m_PromptIndex_Internal++;
}

FTransform ADialoguePrompt::GetPlayerPromptTransform() const
{
	return PlayerTransform->GetComponentTransform();
}


