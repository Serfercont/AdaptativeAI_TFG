// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnemyBase.generated.h"

UENUM(BlueprintType)
enum class EEnemyRole : uint8
{
	None UMETA(DisplayName = "None"),
	Leader UMETA(DisplayName = "Leader"),
	Support UMETA(DisplayName = "Support"),
	Assault UMETA(DisplayName = "Assault"),
	Sniper UMETA(DisplayName = "Sniper")
};

UCLASS()
class ADAPTATIVEAI_TFG_API AEnemyBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Movement")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Movement")
	float RunMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Movement")
	float TurnRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Combat")
	float AttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Combat")
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Combat")
	float AttackCooldown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI | Vital")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI | Vital")
	float CurrentHealth;

	UFUNCTION(BlueprintCallable, Category = "AI | Movement")
	void SetMovementState(bool bIsChasing);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
