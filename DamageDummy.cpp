// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageDummy.h"
#include "UHealthComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/BoxComponent.h"
#include "projectile.h"
#include "Blueprint/UserWidget.h"
#include "MyProject3Character.h"
#include "UHealthBarWidget.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/ProgressBar.h"
#include "DamageDummyAIController.h"
#include "AiNPC.h"
#include "Animation/AnimMontage.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Components/SphereComponent.h"
#include "Components/TextBlock.h"
#include "AIController.h"
#include "Components/SkeletalMeshComponent.h"


ADamageDummy::ADamageDummy()
{
	// Set this actor to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;


	bReplicates = true;
	SetReplicateMovement(true);
	bAlwaysRelevant = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	RootComponent = GetCapsuleComponent();

	SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComp"));
	SkeletalMeshComp->SetupAttachment(RootComponent);
	SkeletalMeshComp->SetRelativeLocation(FVector(0, 0, -96));
	SkeletalMeshComp->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));

	SwordMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(SkeletalMeshComp, TEXT("SwordSocket"));
	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthComponent = CreateDefaultSubobject<AUHealthComponent>(TEXT("HealthComponent"));

	MyWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MyWidgetComponent"));
	MyWidgetComponent->SetupAttachment(RootComponent); 
	MyWidgetComponent->SetRelativeLocation(FVector(0, 0, 100));
	MyWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); 

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
    DetectionSphere->SetupAttachment(RootComponent);
    DetectionSphere->SetSphereRadius(500.0f);
    DetectionSphere->SetCollisionProfileName("Trigger");
    DetectionSphere->SetGenerateOverlapEvents(true);

	DamageNumberWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("DamageNumberWidget"));
	DamageNumberWidget->SetupAttachment(SkeletalMeshComp); // Attach it to the skeletal mesh or capsule
	DamageNumberWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 200.0f)); // Adjust location to float above the dummy
	DamageNumberWidget->SetWidgetSpace(EWidgetSpace::Screen);
	DamageNumberWidget->SetVisibility(false); // Start hidden

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);


	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 200.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> DeathMontageObject(TEXT("AnimMontage'/Game/Weapon/Sword/Death_Montage.Death_Montage'"));
	if (DeathMontageObject.Succeeded())
	{
		DeathMontage = DeathMontageObject.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> HitReactMontageObject(TEXT("AnimMontage'/Game/Weapon/Sword/HitReact_Montage.HitReact_Montage'"));
	if (HitReactMontageObject.Succeeded())
	{
		HitReactMontage = HitReactMontageObject.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> AttackMontageObject(TEXT("AnimMontage'/Game/Weapon/Sword/SwordSlashMontage.SwordSlashMontage'"));
	if (AttackMontageObject.Succeeded())
	{
		AttackMontage = AttackMontageObject.Object;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/UI/BP_DamageNumber"));
	if (WidgetClassFinder.Succeeded())
	{
		DamageNumberWidgetClass = WidgetClassFinder.Class;
	}

	RespawnDelay = 5.0f;

	bTargetCleared = false;

	LastAttacker = nullptr;  // Default attacker

	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;

	AttackCooldownDuration = 3.0f;
}


void ADamageDummy::BeginPlay()
{
	Super::BeginPlay();

	TotalDummy = 4.f;

	if (!GetOwner()) // Ensure it has an owner
	{
		// Set owner to nullptr (indicating no specific owner)
		SetOwner(nullptr); // No specific player owning it, owned by the server
	}

	if (HealthComponent)
	{
		HealthComponent->InitializeHealth(MaxHealth, CurrentHealth); 
		UE_LOG(LogTemp, Warning, TEXT("Current Health Updated, %f"), CurrentHealth);
	}

	SetCurrentHealth(CurrentHealth, MaxHealth);

	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ADamageDummy::OnOverlapBegin);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &ADamageDummy::OnOverlapEnd);


	InitialSpawnLocation = GetActorLocation();
	InitialSpawnRotation = GetActorRotation();

	ADamageDummyAIController* AIController = Cast<ADamageDummyAIController>(GetController());

	if (AIController)
	{
	
		Cast<ADamageDummyAIController>(GetController())->RandomMovement();
	}
}

void ADamageDummy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADamageDummy, CurrentHealth);
}

void ADamageDummy::OnRep_CurrentHealth()
{
	UE_LOG(LogTemp, Warning, TEXT("Health replicated. Current Health: %f"), CurrentHealth);
	SetCurrentHealth(CurrentHealth, MaxHealth); // Update the UI whenever health changes
}

void ADamageDummy::PlayAttackAnimation_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("Animating Montage"));

	if (AttackMontage)
	{
		// Play the attack montage on the character's AnimInstance
		UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(AttackMontage);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Montage Not Found"));
	}
}

void ADamageDummy::PerformSwordAttack()
{
	if (!SwordMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("SwordMesh not found!"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("line tracing"));

	// Get the socket locations
	const FVector StartLocation = SwordMesh->GetSocketLocation(TEXT("Start"));
	const FVector EndLocation = SwordMesh->GetSocketLocation(TEXT("End"));

	// Line trace hit result
	FHitResult HitResult;

	// Set up collision parameters
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this); // Ignore the character itself

	// Perform the line trace
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,            // Hit result
		StartLocation,        // Trace start
		EndLocation,          // Trace end
		ECC_Pawn,      // Collision channel
		CollisionParams       // Collision parameters
	);

	// Debug: Draw the trace in the world
	DrawDebugLine(
		GetWorld(),
		StartLocation,
		EndLocation,
		bHit ? FColor::Red : FColor::Green,
		false,
		2.f,
		0,
		1.f
	);

	// If the trace hits something, process the hit
	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();

		if (HitActor)
		{

			if (!HitActors.Contains(HitActor))
			{
				// Add the actor to the list of hit actors
				HitActors.Add(HitActor);


				UE_LOG(LogTemp, Warning, TEXT("Sword hit: %s"), *HitActor->GetName());

				AMyProject3Character* Player = Cast<AMyProject3Character>(HitActor);

				if (Player)
				{
					// Apply damage to the dummy

					Player->TakeDamage(50.f);

					// Log for debugging
					UE_LOG(LogTemp, Warning, TEXT("Dealt %f damage to: %s"), 50.f, *Player->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Hit actor is not a Player: %s"), *HitActor->GetName());
				}
			}
		}

	}
}

void ADamageDummy::ResetHitActors()
{
	HitActors.Empty(); // Clear the list of hit actors
}

void ADamageDummy::ShowDamageNumber(float Damage, const FVector& HitLocation)
{
	UE_LOG(LogTemp, Warning, TEXT("Showing Showing"));
	if (DamageNumberWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("Showing inside"));
		// Create the widget

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (PlayerController)
			{
				UUserWidget* DamageWidget = CreateWidget<UUserWidget>(PlayerController, DamageNumberWidgetClass);
				if (DamageWidget)
				{
					
					DamageWidget->AddToPlayerScreen();

					// Set the widget's damage text if needed (you need to expose this in your widget BP)
					UTextBlock* DamageText = Cast<UTextBlock>(DamageWidget->GetWidgetFromName("DamageText"));
					if (DamageText)
					{
						DamageText->SetText(FText::AsNumber(Damage));

						AMyProject3Character* GetBool = Cast<AMyProject3Character>(LastAttacker);

						if (GetBool->bIsCritical)
						{
							DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
							// You can also add animations or effects for critical hits
						}
					}

					FVector OffsetLocation = HitLocation + FVector(0, 0, 100.0f); // Adjust Z-value as needed

					// Determine world-to-screen position
					FVector2D ScreenPosition;
					if (UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), OffsetLocation, ScreenPosition))
					{
						// Set the widget's position on screen
						FVector2D WidgetPosition = ScreenPosition;
						DamageWidget->SetPositionInViewport(WidgetPosition);
					}

					// Optionally destroy the widget after a delay
					FTimerHandle TimerHandle;
					GetWorldTimerManager().SetTimer(TimerHandle, [DamageWidget]() {
						if (DamageWidget)
						{
							DamageWidget->RemoveFromViewport();
						}
						}, 1.5f, false); // 1.5 seconds lifetime
				}
			}
		}
	}
}

void ADamageDummy::ClearDamageWidget()
{
    DamageWidgetComponent->SetWidget(nullptr); // Clear the widget
}

void ADamageDummy::OnHitboxHit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if the actor hit is a valid target (e.g., a character)
	Aprojectile* Projectile = Cast<Aprojectile>(OtherActor);

	if (Projectile)
	{
		AActor* OwnerActor = Projectile->GetOwner();

		if (OwnerActor)
		{
			// Try to cast the owner to your character class (e.g., AMyProject3Character)
			AMyProject3Character* CharacterOwner = Cast<AMyProject3Character>(OwnerActor);

			if (CharacterOwner)
			{
				// Now you can access properties of the character that fired the projectile
				UE_LOG(LogTemp, Warning, TEXT("Projectile hit by: %s"), *CharacterOwner->GetName());

				// Optionally, you can apply damage, effects, or logic based on the character
				// Example: Apply damage to the damage dummy or some other logic
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Projectile fired by an unknown actor!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Projectile has no owner!"));
		}
	}

	Destroy();
}


void ADamageDummy::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) 
{
	if (OtherActor && OtherActor->IsA(AMyProject3Character::StaticClass()))
	{
		// Begin attacking the player
		UE_LOG(LogTemp, Warning, TEXT("Attacking Player"));

	}
}

void ADamageDummy::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex) {
	
		UE_LOG(LogTemp, Warning, TEXT("Stop Attacking Player"));
	
}

void ADamageDummy::PerformLightningStrike()
{
	
}

void ADamageDummy::ResetAttack() {
	if (OverlappingCharacter)
	{
		// Example: Access a function or property of the character
		OverlappingCharacter->TakeDamage(10);
	}
}


// Check if the actor is dead
bool ADamageDummy::IsDead() const
{
    return CurrentHealth <= 0.f;
}

void ADamageDummy::ServerTakeDamage_Implementation(float DamageAmount, AActor* InstigatorActor)
{
	TakeDamage(DamageAmount, InstigatorActor); // Call the damage logic on the server
}

bool ADamageDummy::ServerTakeDamage_Validate(float DamageAmount, AActor* InstigatorActor)
{
	return true;
}

// Function to apply damage
void ADamageDummy::TakeDamage(float DamageAmount, AActor* InstigatorActor)
{

	if (HealthComponent)
	{
		TArray<FHitResult> HitResults;

		LastAttacker = InstigatorActor;

		HealthComponent->TakeDamage(DamageAmount);

		CurrentHealth = HealthComponent->Health;

		SetCurrentHealth(CurrentHealth, MaxHealth);

		ShowDamageNumber(DamageAmount, GetActorLocation());

		UE_LOG(LogTemp, Warning, TEXT("Current Health Updated, %f"), CurrentHealth);
		if (HealthComponent)
		{
			HealthComponent->InitializeHealth(MaxHealth, CurrentHealth);
		}

		if (CurrentHealth <= 0.0f)
		{
			if (SkeletalMeshComp && DeathMontage)
			{

				UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance();

				if (AnimInstance)
				{

					// Play the Death Montage
					AnimInstance->Montage_Play(DeathMontage);
				}
			}

			// Delay destruction until the animation finishes
			GetWorld()->GetTimerManager().SetTimer(DestructionTimerHandle, this, &ADamageDummy::DestroyDummy, 2.0f, false);

			IsDead();
		}
		else
		{
			if (SkeletalMeshComp && HitReactMontage)
			{
				UAnimInstance* AnimInstance = SkeletalMeshComp->GetAnimInstance();
				if (AnimInstance)
				{
					AnimInstance->Montage_Play(HitReactMontage);

				}
			}
		}


	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("HealthComponent is nullptr!"));
	}

  
} 

void ADamageDummy::DestroyDummy()
{

	AMyProject3Character* Player = Cast<AMyProject3Character>(LastAttacker);

	UE_LOG(LogTemp, Warning, TEXT("Dummy destroyed! Respawning..."));

	AAIController* DummyController = Cast<AAiNPC>(GetController());
	if (DummyController)
	{
		AAiNPC* AiController = Cast<AAiNPC>(DummyController);
		if (AiController)
		{
			AiController->ClearAttackTimer();
			UE_LOG(LogTemp, Warning, TEXT("Clearing Timer..."));
		}
	}

	// Start respawn timer
	//GetWorld()->GetTimerManager().SetTimer(RespawnTimerHandle, this, &ADamageDummy::Respawn, 4.0f, false );

	if (Player)
	{
		Player->GainExperience(25.f);
	}


	// Destroy the dummy actor after the animation
	if ( TotalDummy > 1.f)
	{
		TotalDummy -= 1.f;
	}
	else
	{
		bTargetCleared = true;
		Player->SetTargetCleared();
	}

	Destroy();


	
}

void ADamageDummy::Respawn()
{
	UE_LOG(LogTemp, Warning, TEXT("Respawn function called!"));

	// Reset health
	CurrentHealth = MaxHealth;

	if (HealthComponent)
	{
		HealthComponent->InitializeHealth(MaxHealth, CurrentHealth); 
		UE_LOG(LogTemp, Warning, TEXT("Current Health Updated, %f"), CurrentHealth);
	}

	SetCurrentHealth(CurrentHealth, MaxHealth);

	// Make the actor visible and active again
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	SetActorLocation(InitialSpawnLocation);
	SetActorRotation(InitialSpawnRotation);

}

void ADamageDummy::SetCurrentHealth(float NewCurrentHealth, float NewMaxHealth)
{
	float HealthPercentage = NewCurrentHealth / NewMaxHealth;

	if (MyWidgetComponent)
	{
		UUserWidget* Widget = MyWidgetComponent->GetUserWidgetObject();
		if (Widget)
		{
			HealthBarWidget = Cast<UUHealthBarWidget>(Widget);
			if (HealthBarWidget)
			{
				HealthBarWidget->UpdateHealthBar(HealthPercentage);
			}
		}
	}


}

