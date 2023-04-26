// Fill out your copyright notice in the Description page of Project Settings.


#include "CorruptionTendril.h"
#include "CorruptionWall.h"
#include "Components/TimelineComponent.h"

ACorruptionTendril::ACorruptionTendril()
{
	TLComp_Pulse = CreateDefaultSubobject<UTimelineComponent>("TL Pulse");
	TLComp_Fade = CreateDefaultSubobject<UTimelineComponent>("TL Fade");
}

void ACorruptionTendril::BeginPlay()
{
	Super::BeginPlay();

	FOnTimelineFloatStatic fs;
	FOnTimelineEventStatic es;
	// Pulse Timeline
	fs.BindUObject(this, &ACorruptionTendril::TL_Pulse);
	TLComp_Pulse->AddInterpFloat(Curve_Pulse, fs);
	es.BindUObject(this, &ACorruptionTendril::TL_Pulse_End);
	TLComp_Pulse->SetTimelineFinishedFunc(es);

	// Fade Timeline
	fs.BindUObject(this, &ACorruptionTendril::TL_Fade);
	TLComp_Fade->AddInterpFloat(Curve_Fade, fs);
	es.BindUObject(this, &ACorruptionTendril::TL_Fade_End);
	TLComp_Fade->SetTimelineFinishedFunc(es);
}
