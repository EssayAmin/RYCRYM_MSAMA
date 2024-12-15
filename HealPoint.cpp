// Fill out your copyright notice in the Description page of Project Settings.


#include "HealPoint.h"
#include "Blueprint/UserWidget.h" 
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SphereComponent.h"
#include "MyProject3Character.h"

// Sets default values
AHealPoint::AHealPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	HealPoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealPoint"));
	HealPoint->SetupAttachment(RootComponent);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(HealPoint);
	DetectionSphere->SetSphereRadius(100.0f);
	DetectionSphere->SetCollisionProfileName("Trigger");
	DetectionSphere->SetGenerateOverlapEvents(true);
}

// Called when the game starts or when spawned
void AHealPoint::BeginPlay()
{
	Super::BeginPlay();
	
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &AHealPoint::OnOverlapBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &AHealPoint::OnOverlapEnd);
}

// Called every frame
void AHealPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AHealPoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	if (OtherActor && OtherActor->IsA(AMyProject3Character::StaticClass()))
	{
		// Begin attacking the player
		UE_LOG(LogTemp, Warning, TEXT("Healing Player"));

		AMyProject3Character* PlayerCharacter = Cast<AMyProject3Character>(OtherActor);

		if (PlayerCharacter)
		{
			float HealAmount = 100.f; // Set how much to heal

			PlayerCharacter->ApplyAOEHealing(HealAmount, 60.f, GetActorLocation());

			UE_LOG(LogTemp, Warning, TEXT("Healing Player for %f health"), HealAmount);
		}


	}

}

void AHealPoint::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {


}