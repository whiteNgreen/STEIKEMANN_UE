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

	Prompt = CreateDefaultSubobject<USceneComponent>("Prompt");
	Prompt->SetupAttachment(Volume);

	Camera_One = CreateDefaultSubobject<UCameraComponent>("Camera_One");
	Camera_One->SetupAttachment(Volume);

	Camera_Two = CreateDefaultSubobject<UCameraComponent>("Camera_Two");
	Camera_Two->SetupAttachment(Volume);
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
		m_Player = OtherActor;
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

bool ADialoguePrompt::GetNextPromptState(ASteikemannCharacter* player, int8 promptIndex)
{
	bool returnBool{};
	if (promptIndex != -1) {
		m_PromptIndex = promptIndex;
	}
	PromptChange_Pure();
	switch (m_PromptIndex)
	{
	case 0:
		PRINTLONG("FIRST PROMPT");
		/* Save camera transform, to lerp back to the correct spot */
		player->m_CameraTransform = m_PlayerCamera->GetComponentTransform();	
		m_ECameraLerp = ECameraLerp::First;
		returnBool = true;
		break;
	case 1:
		PRINTLONG("SECOND PROMPT");
		m_ECameraLerp = ECameraLerp::Second;
		returnBool = true;
		break;
	case 2:
		PRINTLONG("LAST PROMPT");
		returnBool = true;
		break;
	case 3:
		m_ECameraLerp = ECameraLerp::None;
		player->ExitPrompt();
		return true;
	case 4:
		return false;
	default:
		break;
	}
	m_PromptIndex++;
	return returnBool;
}

void ADialoguePrompt::ExitPrompt_Pure()
{
	ExitPrompt();
	m_ECameraLerp = ECameraLerp::None;
	m_PromptIndex = 0;
}
