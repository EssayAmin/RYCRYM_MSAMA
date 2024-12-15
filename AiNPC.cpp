// Fill out your copyright notice in the Description page of Project Settings.


#include "AiNPC.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "MyProject3Character.h"
#include "GameFramework/Character.h"
#include "Perception/AIPerceptionComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/PlayerController.h"
#include "DamageDummy.h"
#include "TimerManager.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "Perception/AISense_Sight.h"
#include "NavigationSystem.h"


AAiNPC::AAiNPC()
{


    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

    PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));


    if (SightConfig)
    {
        SightConfig->SightRadius = 1000.0f;
        SightConfig->LoseSightRadius = 1200.0f;
        SightConfig->PeripheralVisionAngleDegrees = 90.0f;
        SightConfig->SetMaxAge(5.0f);

        SightConfig->DetectionByAffiliation.bDetectEnemies = true;
        SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
        SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

        PerceptionComponent->ConfigureSense(*SightConfig);
        PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    }

    PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AAiNPC::OnTargetDetected);

    DetectedPlayer = nullptr;
    bIsWandering = true;

    AttackRange = 200.0f;

}


void AAiNPC::BeginPlay()
{
    Super::BeginPlay();

    Wander();
}

void AAiNPC::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (ClosestPlayer )
    {
        if (bIsWandering)
        {
            bIsWandering = false;
            GetWorld()->GetTimerManager().ClearTimer(WanderTimerHandle); // Stop wandering timer
        }

        DistanceToPlayer = FVector::Dist(ClosestPlayer->GetActorLocation(), GetPawn()->GetActorLocation());

        if (DistanceToPlayer <= AttackRange)
        {
            if (!GetWorld()->GetTimerManager().IsTimerActive(AttackTimerHandle))
            {
                GetWorld()->GetTimerManager().SetTimer(AttackTimerHandle, this, &AAiNPC::AttackPlayer, 3.0f, true);
            }
            else
            {
                
            }
        }
        else
        {
            // Move closer to the player if out of range
            MoveToActor(ClosestPlayer, 100.f);
        }
    }

    

}

void AAiNPC::Wander()
{
    if (!bIsWandering)
    {
        return; // Don't wander if wandering is disabled
    }

   
    APawn* ControlledPawn = GetPawn();
    if (ControlledPawn)
    {
        FVector Origin = ControlledPawn->GetActorLocation();
        FVector RandomPoint = UKismetMathLibrary::RandomPointInBoundingBox(Origin, FVector(500.0f, 500.0f, 0.0f));
        MoveToTarget(RandomPoint);
    }

    GetWorld()->GetTimerManager().SetTimer(WanderTimerHandle, this, &AAiNPC::Wander, 3.0f, false);
}

void AAiNPC::MoveToTarget(FVector TargetLocation)
{
    MoveToLocation(TargetLocation);
}

void AAiNPC::OnTargetDetected(AActor* Actor, FAIStimulus Stimulus)
{

    AMyProject3Character* PlayerCharacter = Cast<AMyProject3Character>(Actor); // Replace with your player character class
    if (!PlayerCharacter)
    {
        return; // If the detected actor is not the player, do nothing
    }
    
    if (Stimulus.WasSuccessfullySensed())
    {
        // Add player to the list if detected
        DetectedPlayers.AddUnique(Actor);
    }
    else
    {
        // Remove player if no longer sensed
        DetectedPlayers.Remove(Actor);
    }

    
    FollowClosestPlayer();
}

void AAiNPC::FollowClosestPlayer()
{
    float MinDistance = FLT_MAX;
    ClosestPlayer = nullptr;

    for (AActor* Player : DetectedPlayers)
    {
        if (Player && GetPawn())
        {
            float Distance = FVector::Dist(GetPawn()->GetActorLocation(), Player->GetActorLocation());
            if (Distance < MinDistance)
            {
                MinDistance = Distance;
                ClosestPlayer = Player;
            }
        }
    }

    if (ClosestPlayer)
    {
        MoveToActor(ClosestPlayer, 100.0f); // Follow the closest player
    }
    else
    {
        ClosestPlayer = nullptr; // Player no longer sensed
        if (!bIsWandering)
        {
            bIsWandering = true; // Resume wandering
            Wander();
        }

        
    }
}

void AAiNPC::AttackPlayer()
{
    

    if (ClosestPlayer)
    {
       

        UE_LOG(LogTemp, Warning, TEXT("Doing Doing"));
        DistanceToPlayer = FVector::Dist(ClosestPlayer->GetActorLocation(), GetPawn()->GetActorLocation());
        if (DistanceToPlayer > AttackRange)
        {
            UE_LOG(LogTemp, Warning, TEXT("Returning"));
            ClearAttackTimer();
            return;
        }

        // Cast to your character class and play the attack animation
        ADamageDummy* AiCharacter = Cast<ADamageDummy>(GetPawn());
        if (AiCharacter && HasAuthority())
        {
            UE_LOG(LogTemp, Warning, TEXT("Server is triggering this"));

            UE_LOG(LogTemp, Warning, TEXT("ADamageDummy Cast Successful. Animating..."));
            AiCharacter->ResetHitActors();

            AiCharacter->PlayAttackAnimation();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Cast to ADamageDummy failed or GetPawn() returned NULL!"));

            AActor* ControlledPawn = GetPawn();
            if (ControlledPawn)
            {
                UE_LOG(LogTemp, Warning, TEXT("Controlled Pawn: %s"), *ControlledPawn->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("GetPawn() returned NULL!"));
            }

            UE_LOG(LogTemp, Error, TEXT("Cast to ADamageDummy failed!"));
        }

    }
}

void AAiNPC::ClearAttackTimer()
{
    GetWorld()->GetTimerManager().ClearTimer(AttackTimerHandle);
    UE_LOG(LogTemp, Warning, TEXT("Attack timer cleared in AI Controller."));
}