// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "MyProject3Character.h"
#include "Kismet/GameplayStatics.h"
#include "DamageDummyAIController.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT3_API ADamageDummyAIController : public AAIController
{
	GENERATED_BODY()

public:
	class UNavigationSystemV1* myNavSystem;
	void RandomMovement();


protected:

	void OnPossess(APawn* InPawn) override;

	void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
};
