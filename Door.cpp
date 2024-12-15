// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"
#include "Blueprint/UserWidget.h" 
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/SphereComponent.h"
#include "DamageDummy.h"
#include "EngineUtils.h"
#include "MyProject3Character.h"

// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	DoorFrame = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorFrame"));
	DoorFrame->SetupAttachment(RootComponent);

	Door = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Door"));
	Door->SetupAttachment(DoorFrame);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(DoorFrame);
	DetectionSphere->SetSphereRadius(100.0f);
	DetectionSphere->SetCollisionProfileName("Trigger");
	DetectionSphere->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/UI/DoorWarning"));

	if (WidgetClassFinder.Succeeded())
	{
		WidgetClass = WidgetClassFinder.Class;
	}

}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();

	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ADoor::OnOverlapBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ADoor::OnOverlapEnd);

	for (TActorIterator<ADamageDummy> It(GetWorld()); It; ++It)
	{
		TargetDummy = *It;
		UE_LOG(LogTemp, Warning, TEXT("DamageDummy found!"));
		break; // Stop after finding the first one
	}
	
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TArray<AActor*> DamageDummies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ADamageDummy::StaticClass(), DamageDummies);

	if (DamageDummies.Num() == 0) // No DamageDummy found
	{
		UE_LOG(LogTemp, Warning, TEXT("No DamageDummy found in the level. Executing logic..."));

		Door->SetVisibility(false);
		Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		DetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		UE_LOG(LogTemp, Warning, TEXT("Door is now hidden because the target is cleared!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("DamageDummies still exist. Count: %d"), DamageDummies.Num());
	}

}

void ADoor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{

	if (WidgetClass)
	{
		CreatedWidget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
		AMyProject3Character* PlayerCharacter = Cast<AMyProject3Character>(OtherActor);

		if (CreatedWidget)
		{
			if (PlayerCharacter && PlayerCharacter->bTargetCleared)
			{
				Door->SetVisibility(false);
				Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(LogTemp, Warning, TEXT("Door is now hidden because the target is cleared!"));
			}
			else
			{
				CreatedWidget->AddToViewport();
				UE_LOG(LogTemp, Warning, TEXT("Widget added to viewport successfully!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create widget instance."));
		}
	}

}

void ADoor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {

	if (CreatedWidget)
	{
		CreatedWidget->RemoveFromViewport();
	}

}
