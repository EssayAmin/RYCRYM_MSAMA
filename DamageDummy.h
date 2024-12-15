// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "DamageDummy.generated.h"

class AUHealthComponent;
class AMyProject3Character;
class UUHealthBarWidget;
class UWidgetComponent;
class UAnimMontage;
class ADamageDummyAIController;


UCLASS()
class MYPROJECT3_API ADamageDummy : public ACharacter
{
	GENERATED_BODY()

    
	
public:	
	ADamageDummy();

    UFUNCTION(BlueprintCallable, Category = "Event")
    void TakeDamage(float DamageAmount, AActor* InstigatorActor);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerTakeDamage(float DamageAmount, AActor* InstigatorActor);


    FVector InitialSpawnLocation;
    FRotator InitialSpawnRotation;



    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
    float RespawnDelay;

    

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

    FTimerHandle DestructionTimerHandle;

    FTimerHandle RespawnTimerHandle;

    void DestroyDummy();

    void Respawn();

    AActor* LastAttacker;  // Keeps track of who attacked this dummy

    AMyProject3Character* OverlappingCharacter;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_CurrentHealth(); // Called when health updates on clients


public:	

    // Health component
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
    AUHealthComponent* HealthComponent;

    UFUNCTION(BlueprintCallable, Category = "Health")
    void SetCurrentHealth(float NewCurrentHealth, float NewMaxHealth);

    UPROPERTY(VisibleAnywhere, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
    USkeletalMeshComponent* SwordMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
    USkeletalMeshComponent* SkeletalMeshComp;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* DetectionSphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
    UAnimSequence* DeathAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
    UAnimSequence* HitReactAnim;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animations")
    UAnimSequence* Idle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* DeathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
    UAnimMontage* HitReactMontage;

    UPROPERTY()
    UUHealthBarWidget* HealthBarWidget;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, EditAnywhere, BlueprintReadWrite, Category = "Health")
    float CurrentHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    TSubclassOf<UUserWidget> HealthBarWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
    class UProgressBar* EnemyHealthBar;

    UFUNCTION(BlueprintCallable, Category = "Health")
    bool IsDead() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
    UWidgetComponent* MyWidgetComponent;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    UFUNCTION()
    void OnHitboxHit(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void PerformLightningStrike();

    void ResetAttack();

    UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Animations")
    void PlayAttackAnimation();

    void PerformSwordAttack();

    void ResetHitActors();

    TSet<AActor*> HitActors;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UWidgetComponent* DamageNumberWidget;

    float AttackCooldownDuration;

    FTimerHandle AttackCooldownTimer;

    FTimerHandle NumberDamageTimer;

    FTimerHandle IdleTransitionTimerHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<UUserWidget> DamageNumberWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage UI")
    UWidgetComponent* DamageWidgetComponent;

    void ShowDamageNumber(float Damage, const FVector& HitLocation);

    void ClearDamageWidget();

    UPROPERTY(EditDefaultsOnly, Category = "Animations")
    UAnimMontage* AttackMontage;

    float TotalDummy;

    bool bTargetCleared;

};
