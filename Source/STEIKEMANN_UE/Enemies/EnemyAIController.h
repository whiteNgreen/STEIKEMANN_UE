// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"


UENUM()
enum class ESmallEnemyAIState
{
	Idle,
	Attack 
};

/**
 * 
 */
UCLASS()
class STEIKEMANN_UE_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:	// Components
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UBehaviorTreeComponent* BTComponent{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UBlackboardComponent* BBComponent{ nullptr };
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		class UPawnSensingComponent* PSComponent{ nullptr };

public: // Assets
	UPROPERTY(EditAnywhere)
		class UBehaviorTree* BTIdle{ nullptr };
	UPROPERTY(EditAnywhere)
		class UBehaviorTree* BTAttack{ nullptr };

	UPROPERTY(EditAnywhere)
		class UBlackboardData* BB{ nullptr };

public:	// Functions
	AEnemyAIController();

	virtual void Tick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	void SetState_Idle();
	void SetState_Attack();
	

public: // Variables
	ESmallEnemyAIState AIState;
	UPROPERTY(BlueprintReadWrite)
		bool bFollowPlayer{};
};
