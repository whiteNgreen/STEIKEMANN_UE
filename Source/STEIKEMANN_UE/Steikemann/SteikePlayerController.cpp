// Fill out your copyright notice in the Description page of Project Settings.


#include "SteikePlayerController.h"
#include "SteikemannCharacter.h"
#include "../DebugMacros.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/NavigationConfig.h"

void ASteikePlayerController::CheckInputType(FKey key)
{
	PRINTPARLONG(2.f, "Key: %s", *key.GetDisplayName().ToString());
	EInputType NewInputType = EInputType::MouseNKeyboard;
	if (key.IsGamepadKey()) {
		NewInputType = EInputType::Gamepad;
	}
	if (NewInputType != m_EInputType) {
		m_EInputType = NewInputType;
		ChangedInputType();
	}
}

void ASteikePlayerController::BeginPlay()
{
	Super::BeginPlay();

	//if (IsLocalPlayerController())
	//{
	//	TSharedRef<FNavigationConfig> Navigation = MakeShared<FNavigationConfig>();
	//	Navigation->bKeyNavigation = false;
	//	Navigation->bTabNavigation = false;
	//	Navigation->bAnalogNavigation = false;
	//	FSlateApplication::Get().SetNavigationConfig(Navigation);
	//}
}

void ASteikePlayerController::OnPossess(APawn* pawn)
{
	Super::OnPossess(pawn);
	PlayerCharacter = Cast<ASteikemannCharacter>(pawn);
	if (PlayerCharacter)
	{
		PlayerCharacter->SteikePlayerController = this;
	}
}




