// Fill out your copyright notice in the Description page of Project Settings.


#include "TutorialPrompt.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "../GameplayTags.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ATutorialPrompt::ATutorialPrompt()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;
	Volume = CreateDefaultSubobject<UBoxComponent>("Volume");
	Volume->SetupAttachment(Root);
}

// Called when the game starts or when spawned
void ATutorialPrompt::BeginPlay()
{
	Super::BeginPlay();
	
	Volume->OnComponentBeginOverlap.AddDynamic(this, &ATutorialPrompt::OnVolumeBeginOverlap);
	Volume->OnComponentEndOverlap.AddDynamic(this, &ATutorialPrompt::OnVolumeEndOverlap);
}

// Called every frame
void ATutorialPrompt::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATutorialPrompt::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherComp->IsA(UCapsuleComponent::StaticClass())) return;
	if (OtherActor == m_Player)
	{
		ShowPrompt();
		return;
	}

	auto ITag = Cast<IGameplayTagAssetInterface>(OtherActor);
	if (!ITag) return;

	FGameplayTagContainer tags;
	ITag->GetOwnedGameplayTags(tags);
	if (tags.HasTag(Tag::Player()))
	{
		m_Player = OtherActor;
		ShowPrompt();
		return;
	}
}

void ATutorialPrompt::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor != m_Player) return;
	if (!OtherComp->IsA(UCapsuleComponent::StaticClass())) return;
	
	EndPrompt();
}

void ATutorialPrompt::ShowPrompt()
{
	switch (m_PromptType)
	{
	case ETutorialPromptType::Jump:			Prompt_Jump();			break;
	case ETutorialPromptType::DoubleJump:	Prompt_DoubleJump();	break;
	case ETutorialPromptType::Smack:		Prompt_Smack();			break;
	case ETutorialPromptType::Scoop:		Prompt_Scoop();			break;
	case ETutorialPromptType::GrappleHook:	Prompt_Grapplehook();	break;
	case ETutorialPromptType::Groundpound:	Prompt_Groundpound();	break;
	default:													break;
	}
}

