// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "AiNPC.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT3_API AAiNPC : public AAIController
{
	GENERATED_BODY()

public:
    AAiNPC();


protected:
    virtual void BeginPlay() override;

private:
    bool bIsTimerActive = false;

    UPROPERTY()
    TArray<AActor*> DetectedPlayers;

    AActor* ClosestPlayer;

public:
    virtual void Tick(float DeltaTime) override;

    void ClearAttackTimer();

    // Sight configuration
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UAISenseConfig_Sight* SightConfig;

    // Handle random wandering
    FTimerHandle WanderTimerHandle;

    // Current target
    AActor* PlayerActor;
    bool bPlayerDetected;

    // AI Behaviors
    void Wander();
    void MoveToTarget(FVector TargetLocation);

    // Perception event
    UFUNCTION()
    void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

    void FollowClosestPlayer();

    UPROPERTY()
    AActor* DetectedPlayer; // Keeps track of the detected player

    bool bIsWandering; // Flag to check if wandering

    UFUNCTION()
    void AttackPlayer(); // Function to attack the player

    UPROPERTY(EditDefaultsOnly, Category = "Attack")
    float AttackRange; // Range within which the AI can attack

    float DistanceToPlayer;

    FTimerHandle AttackTimerHandle; // Timer for attack interval
	
};
