// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageDummyAIController.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Character.h"

void ADamageDummyAIController::RandomMovement()
{
	if (myNavSystem != nullptr)
	{
		if (GetPawn() != nullptr)
		{
			FNavLocation Result;
			if (myNavSystem->GetRandomReachablePointInRadius(GetPawn()->GetActorLocation(), 600, Result))
			{
				MoveToLocation(Result);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("GET PAWN RETURNED NULL"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RANDOM MOVEMENT FAILED, NAV SYSTEM NULL"));
	}
}

void ADamageDummyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	myNavSystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());

}

void ADamageDummyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);

	RandomMovement();
}
