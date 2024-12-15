// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAnimNotifyState.h"
#include "MyProject3Character.h"
#include "Net/UnrealNetwork.h"
#include "DamageDummy.h"
#include "DrawDebugHelpers.h"

void UMyAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
    AMyProject3Character* Character = Cast<AMyProject3Character>(MeshComp->GetOwner());
    if (Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("SwordTrace Notify Begin"));
    }
}

void UMyAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
    AMyProject3Character* Character = Cast<AMyProject3Character>(MeshComp->GetOwner());
    if (Character)
    {
    
         Character->PerformSwordAttack();
      
        
    }

    ADamageDummy* Character1 = Cast<ADamageDummy>(MeshComp->GetOwner());
    if (Character1)
    {

        Character1->PerformSwordAttack();

    }

}

void UMyAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    AMyProject3Character* Character = Cast<AMyProject3Character>(MeshComp->GetOwner());
    if (Character)
    {
        UE_LOG(LogTemp, Warning, TEXT("SwordTrace Notify End"));
    }
}
