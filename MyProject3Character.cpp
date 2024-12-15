// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProject3Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Kismet/GameplayStatics.h"
#include "InputActionValue.h"
#include "Components/WidgetComponent.h"
#include "projectile.h"
#include "UHealthComponent.h"
#include "UHealthBarWidget.h"
#include "UPlayerHealthBar.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "DamageDummy.h" 
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimMontage.h"
#include "Components/PrimitiveComponent.h"
#include "CollisionQueryParams.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"


DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMyProject3Character

AMyProject3Character::AMyProject3Character()
{

	TargetDummyClass = ADamageDummy::StaticClass();

	bReplicates = true;
	SetReplicateMovement(true);

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	PrimaryActorTick.bCanEverTick = true;

	// Initialize the health component
	HealthComponent = CreateDefaultSubobject<AUHealthComponent>(TEXT("HealthComponent"));

	SkeletalMeshComp = FindComponentByClass<USkeletalMeshComponent>();

	SwordMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SwordMesh"));

	SwordMesh->SetupAttachment(GetMesh(), TEXT("BackSwordSocket"));

	SwordMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarComponent"));
	HealthBarComponent->SetupAttachment(SkeletalMeshComp); // Attach to root or a desired component
	HealthBarComponent->SetWidgetSpace(EWidgetSpace::Screen); // Or World for 3D widgets
	HealthBarComponent->SetDrawSize(FVector2D(150, 30)); // Adjust the size

	static ConstructorHelpers::FObjectFinder<UAnimMontage> MontageObject(TEXT("AnimMontage'/Game/Weapon/Sword/SwordSlashMontage.SwordSlashMontage'"));
	if (MontageObject.Succeeded())
	{
		SwordSlashMontage = MontageObject.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimMontage> HitReactMontageObject(TEXT("AnimMontage'/Game/Weapon/Sword/HitReact_Montage.HitReact_Montage'"));
	if (HitReactMontageObject.Succeeded())
	{
		HitReactMontage = HitReactMontageObject.Object;
	}
	
	static ConstructorHelpers::FClassFinder<UUserWidget> WidgetClassFinder(TEXT("/Game/UI/HealthBar"));
	if (WidgetClassFinder.Succeeded())
	{
		HealthBarWidgetClass = WidgetClassFinder.Class;
	}
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);


	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 300.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	Experience = 20.f;
	Level = 1;
	ExpToLevelUp = 100.f;  // Modify this based on your preferred leveling system

	if (HasAuthority())
	{
		SetPlayerRole(EPlayerRole::Warrior);
	}
	else
	{
		SetPlayerRole(EPlayerRole::Mage);
	}

	Health = MaxHealth;

	AttackCooldownTime = 1.0f;
	HealCooldownTime = 2.0f;
	bCanAttack = true;
	bCanHeal = true;

	bTargetCleared = false;


	static ConstructorHelpers::FObjectFinder<UAnimSequence> anim(TEXT("/Script/Engine.AnimSequence'/Game/RamsterZ_FreeAnims_Volume1/AnimationSequence/H2H/H2H_PunchCombo01.H2H_PunchCombo01'"));
	PunchCombo01 = anim.Object;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// Initialize the health with unique values for this character
	if (HealthComponent)
	{
		HealthComponent->InitializeHealth(MaxHealth, Health); // MaxHealth = 200, InitialHealth = 150
		SetCurrentHealth(MaxHealth, Health);
	}

	HealthBarWidget1 = CreateWidget<UUserWidget>(GetWorld(), HealthBarWidgetClass);
	if (HealthBarWidget1)
	{
		HealthBarWidget1->AddToViewport();
	}

}

void AMyProject3Character::BeginPlay()
{
	Super::BeginPlay();

	SpawnDamageDummy();

	if (HealthBarComponent)
	{
		// Check if this character is controlled by the local player
		if (IsLocallyControlled())
		{
			// Hide the health bar for the owner
			HealthBarComponent->SetVisibility(false);
		}
		else
		{
			// Show the health bar for others
			HealthBarComponent->SetVisibility(true);
		}
	}

	if (HasAuthority())
	{
		SetPlayerRole(EPlayerRole::Warrior);
	}
	else
	{
		SetPlayerRole(EPlayerRole::Mage);
	}

	SetCurrentHealth(MaxHealth, Health);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProject3Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AutoAimAtTarget(DeltaTime);
}

void AMyProject3Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyProject3Character, Damage);
}

void AMyProject3Character::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMyProject3Character::PerformSwordAttack()
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

				ADamageDummy* Dummy = Cast<ADamageDummy>(HitActor);

				if (Dummy)
				{

					if (HasAuthority())
					{
						Dummy->TakeDamage(Damage, this);
						UE_LOG(LogTemp, Warning, TEXT("Dealt %f damage to: %s"), Damage, *Dummy->GetName());
					}
					else
					{
						PerformDamage(Damage, this, Dummy);
						
					}
					// Log for debugging
					
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Hit actor is not a Damage Dummy: %s"), *HitActor->GetName());
				}
			}
		}
	}
}

void AMyProject3Character::PerformDamage_Implementation(float DamageAmount, AActor* InstigatorActor, ADamageDummy* Dummy)
{
	UE_LOG(LogTemp, Warning, TEXT("From Server Dealt %f damage to: %s"), DamageAmount, *Dummy->GetName());

	Dummy->TakeDamage(DamageAmount, InstigatorActor);
}

bool AMyProject3Character::PerformDamage_Validate(float DamageAmount, AActor* InstigatorActor, ADamageDummy* Dummy)
{
	return true;
}

void AMyProject3Character::SpawnDamageDummy()
{
	FVector SpawnLocation = GetActorLocation() + FVector(200.f, 0.f, 0.f); // Just an offset for example
	FRotator SpawnRotation = FRotator::ZeroRotator;  // Or some rotation


}

void AMyProject3Character::AutoAimAtTarget(float DeltaTime)
{
	// Find the closest target
	CurrentTarget = FindClosestTarget();

	if (CurrentTarget)
	{
		// Calculate the direction to the target
		FVector TargetLocation = CurrentTarget->GetActorLocation();
		FVector DirectionToTarget = TargetLocation - GetActorLocation();

		// Convert direction to rotation
		FRotator TargetRotation = DirectionToTarget.Rotation();

		FRotator NewControlRotation = FMath::RInterpTo(GetControlRotation(), TargetRotation, DeltaTime, 10.0f); // Adjust the speed here

	}
}

void AMyProject3Character::SetPlayerRole(EPlayerRole ARole)
{

	PlayerRole = ARole;
	switch (ARole)
	{
	case EPlayerRole::Warrior:
		MaxHealth = 200.0f;
		BaseDamage = 25.0f;

		break;

	case EPlayerRole::Mage:
		MaxHealth = 100.0f;
		BaseDamage = 50.0f;

		break;

	default:
		MaxHealth = 200.0f;
		BaseDamage = 25.0f;
		break;
	}
}

void AMyProject3Character::GainExperience(float Amount)
{
	Experience += Amount;

	// Check if we have enough experience to level up
	if (Experience >= ExpToLevelUp)
	{
		LevelUp();
	}
}

void AMyProject3Character::LevelUp()
{
	Level++;  // Increase the level
	Experience -= ExpToLevelUp;  // Subtract the amount required for leveling up
	ExpToLevelUp *= 1.2f;  // Increase the required EXP for the next level
	BaseDamage *= 1.2f;

	UE_LOG(LogTemp, Warning, TEXT("Character leveled up to level %d!"), Level);

	// Update the level
	UpdateLevel();
}

void AMyProject3Character::ResetHitActors()
{
	HitActors.Empty(); // Clear the list of hit actors
}

void AMyProject3Character::UpdateLevel()
{

	float NewHealth = MaxHealth + (Level * 10);  // Increase health by 10 per level
	MaxHealth = NewHealth;

	UE_LOG(LogTemp, Warning, TEXT("Character's new level: %d"), Level);
}

AActor* AMyProject3Character::FindClosestTarget()
{

	FVector PlayerLocation = GetActorLocation();
	AActor* ClosestTarget = nullptr;
	float MinDistance = AutoAimRange;

	// Search for Damage Dummies instead of generic actors
	TArray<AActor*> PotentialTargets;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), TargetDummyClass, PotentialTargets);

	if (PotentialTargets.Num() == 0)
	{
		if (GEngine)
		{
			/*GEngine->AddOnScreenDebugMessage(
				-1, 5.0f, FColor::Red,
				TEXT("No targets of TargetDummyClass found in the world!")
			);*/
		}
		return nullptr;
	}

	for (AActor* Target : PotentialTargets)
	{
		
		float Distance = FVector::Dist(PlayerLocation, Target->GetActorLocation());

		/* Debugging: Draw a line to each target and log distance
		DrawDebugLine(
			GetWorld(),
			PlayerLocation,
			Target->GetActorLocation(),
			FColor::Blue,   // Color of the line
			false,          // Do not persist
			2.0f,           // Duration (2 seconds)
			0,
			2.0f            // Thickness 
		);*/

		// Debugging: Print the target's name and distance
		if (GEngine)
		{
			/*GEngine->AddOnScreenDebugMessage(
				-1, 2.0f, FColor::Yellow,
				FString::Printf(TEXT("Target: %s, Distance: %.2f"),
					*Target->GetName(), Distance)
			);*/
		}

		if (Distance < MinDistance)
		{
			ClosestTarget = Target;
			MinDistance = Distance;
		}
	}

	return ClosestTarget;
}

void AMyProject3Character::EquipWeapon(bool bAttachToHand)
{
	if (SwordMesh)
	{
		// If we are attaching the weapon to the hand
		if (bAttachToHand)
		{
			SwordMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("SwordSocket"));
			bIsWeaponAttachedToHand = true;  // Mark the weapon as attached to the hand

			// Reset and start the timer to detach after a period (e.g., 3 seconds)
			GetWorld()->GetTimerManager().ClearTimer(WeaponHoldTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(WeaponHoldTimerHandle, this, &AMyProject3Character::DetachWeapon, 4.0f, false);
		}
		else
		{
			// Attach the weapon to the back when idle or walking
			SwordMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackSwordSocket"));
			bIsWeaponAttachedToHand = false;  // Mark the weapon as detached
		}
	}
}

void AMyProject3Character::DetachWeapon()
{
	if (SwordMesh)
	{
		// Move weapon to the back socket when time is up
		SwordMesh->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, TEXT("BackSwordSocket"));
		bIsWeaponAttachedToHand = false;
	}
}

void AMyProject3Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)


{

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProject3Character::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProject3Character::Look);

		// BAsic Attack
		EnhancedInputComponent->BindAction(BasicAttackAction, ETriggerEvent::Triggered, this, &AMyProject3Character::StartBasicAttack);

		EnhancedInputComponent->BindAction(HealingSkillAction, ETriggerEvent::Triggered, this, &AMyProject3Character::StartHealingSkill);

		
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMyProject3Character::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AMyProject3Character::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AMyProject3Character::StartBasicAttack(const FInputActionValue& Value)
{
	if (bCanAttack)
	{
	UE_LOG(LogTemp, Warning, TEXT("we are using basic attack"));

	BasicAttackUsed = true;
	
	bCanAttack = false;

	Damage = CalculateDamage();

	// Check for overlapping actors in the player's range
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ADamageDummy::StaticClass());  // Only check for DamageDummy act

	switch (PlayerRole)
	{
	case EPlayerRole::Warrior:


		if (HasAuthority()) // If running on the server (host)
		{
			Multicast_PlayAttackAnimation();
		}
		else // If running on a client
		{
			Server_PlayAttackAnimation();
		}

		break;

	case EPlayerRole::Mage:
		
		if (CurrentTarget)
		{

			UE_LOG(LogTemp, Warning, TEXT("current current"));

			// Aim directly at the target
			FVector StartLocation = GetActorLocation();  // Start point of the projectile
			FVector TargetLocation;

			UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(CurrentTarget->GetRootComponent());
			if (RootComp)
			{
				TargetLocation = RootComp->Bounds.Origin;  // Center of the actor's collision bounds
			}
			else
			{
				TargetLocation = CurrentTarget->GetActorLocation() + FVector(0, 0, 50);  // Default offset if no collision bounds exist
			}

			// Calculate the direction to the target
			FRotator FireDirection = (TargetLocation - StartLocation).Rotation();


			if (HasAuthority())
			{
				// Spawn the projectile (replace SpawnProjectile with your own implementation)
				Aprojectile* Projectile = GetWorld()->SpawnActor<Aprojectile>(ProjectileAttack, GetActorLocation(), FireDirection);

				Projectile->SetOwner(this);
			}
			else
			{

			}
		}
		else
		{
			GetWorld()->SpawnActor<Aprojectile>(ProjectileAttack, GetActorLocation() + GetActorForwardVector() * 100.0f, GetActorRotation());

		}

		break;

	default:
		
		for (AActor* Actor : OverlappingActors)
		{
			ADamageDummy* Dummy = Cast<ADamageDummy>(Actor);

			if (Dummy)
			{
				if (HasAuthority())
				{
					// If we're on the server, directly apply damage
					Dummy->TakeDamage(Damage, this);
				}
				else
				{
					// If we're a client, request the server to apply damage
					Dummy->ServerTakeDamage(Damage, this);
				}

			}
		};

		break;
	}

	GetWorld()->GetTimerManager().SetTimer(AttackCooldownTimerHandle, this, &AMyProject3Character::ResetAttackCooldown, AttackCooldownTime, false);
	

	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Attack on cooldown"));
	}
	
}

void AMyProject3Character::Server_PlayAttackAnimation_Implementation()
{
	// Call the multicast function to replicate the animation
	Multicast_PlayAttackAnimation();
}

// Function that runs on all clients, including the host, to play the attack animation
void AMyProject3Character::Multicast_PlayAttackAnimation_Implementation()
{
	if (SwordSlashMontage && GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{

			ResetHitActors();

			if (!bIsWeaponAttachedToHand)
			{
				EquipWeapon(true);  // Attach weapon to hand on attack
			}
			else
			{
				// If already attached, reset the timer
				GetWorld()->GetTimerManager().ClearTimer(WeaponHoldTimerHandle);
				GetWorld()->GetTimerManager().SetTimer(WeaponHoldTimerHandle, this, &AMyProject3Character::DetachWeapon, 4.0f, false);
			}

			AnimInstance->Montage_Play(SwordSlashMontage);

		}
	};
}

void AMyProject3Character::StartHealingSkill(const FInputActionValue& Value)
{
	if (bCanHeal)
	{
		UE_LOG(LogTemp, Warning, TEXT("HEaling"));
		ApplyAOEHealing(30.f, 60.f, GetActorLocation());

		bCanHeal = false;
	}

	GetWorld()->GetTimerManager().SetTimer(HealCooldownTimerHandle, this, &AMyProject3Character::ResetHealCooldown, HealCooldownTime, false);
	
}

void AMyProject3Character::TakeDamage(float DamageAmount)
{
	UE_LOG(LogTemp, Warning, TEXT("we are taking damage "), DamageAmount);

	if (HitReactMontage && GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{

			AnimInstance->Montage_Play(HitReactMontage);

		}
	}

	HealthComponent->TakeDamage(DamageAmount);

	Health = HealthComponent ? HealthComponent->Health : 0.f;

	if (playerHealth < 0.00f)
	{
		playerHealth = 0.00f;
	}
}

void AMyProject3Character::ResetAttackCooldown()
{
	bCanAttack = true;  // Cooldown is over, allow another attack

	UE_LOG(LogTemp, Warning, TEXT("Attack cooldown over"));
}

void AMyProject3Character::ResetHealCooldown()
{
	bCanHeal = true;  // Cooldown is over, allow another attack
	UE_LOG(LogTemp, Warning, TEXT("Heal cooldown over"));
}


void AMyProject3Character::ApplyAOEHealing(float HealAmount, float Radius, FVector Origin)
{
	// Create a list to hold hit results
	TArray<FHitResult> HitResults;

	HealthComponent->Heal(HealAmount);

	UE_LOG(LogTemp, Warning, TEXT("healing "));

	Health = HealthComponent ? HealthComponent->Health : 0.f;
}

float AMyProject3Character::CalculateDamage()
{
	// Determine if the hit is critical
	bIsCritical = FMath::FRand() <= CritChance; // Random float between 0.0 and 1.0

	Damage = BaseDamage;

	if (bIsCritical)
	{
		Damage *= CritMultiplier;
		UE_LOG(LogTemp, Warning, TEXT("Critical Hit! Damage: %f"), Damage);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Normal Hit! Damage: %f"), Damage);
	}

	return Damage;
}

void AMyProject3Character::SetCurrentHealth(float NewCurrentHealth, float NewMaxHealth)
{
	float HealthPercentage = NewCurrentHealth / NewMaxHealth;

	if (HealthBarComponent)
	{
		UUserWidget* Widget = HealthBarComponent->GetUserWidgetObject();
		if (Widget)
		{
			HealthBarWidget2 = Cast<UUHealthBarWidget>(Widget);
			if (HealthBarWidget2)
			{
				HealthBarWidget2->UpdateHealthBar(HealthPercentage);
			}
		}
	}


}

void AMyProject3Character::SetTargetCleared()
{
	bTargetCleared = true;
}
