// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseStaticActor.h"
#include "WaterPuddle.generated.h"

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API AWaterPuddle : public ABaseStaticActor
{
	GENERATED_BODY()
	
public:
	AWaterPuddle();
protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UBoxComponent* WaterCollision;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UStaticMeshComponent* Mesh;

public:

};
