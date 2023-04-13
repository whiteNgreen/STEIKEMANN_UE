// Fill out your copyright notice in the Description page of Project Settings.


#include "CorruptionTendril.h"
#include "CorruptionWall.h"
#include "Components/TimelineComponent.h"

ACorruptionTendril::ACorruptionTendril()
{
	TLComp_Pulse = CreateDefaultSubobject<UTimelineComponent>("TL Pulse");
}

void ACorruptionTendril::BeginPlay()
{
	Super::BeginPlay();
	FOnTimelineFloatStatic fs;
	fs.BindUObject(this, &ACorruptionTendril::TL_Pulse);
	TLComp_Pulse->AddInterpFloat(Curve_Pulse, fs);

	FOnTimelineEventStatic es;
	es.BindUObject(this, &ACorruptionTendril::TL_Pulse_End);
	TLComp_Pulse->SetTimelineFinishedFunc(es);
}
