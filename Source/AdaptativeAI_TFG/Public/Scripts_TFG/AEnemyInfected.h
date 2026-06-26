// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "Scripts_TFG/EnemyBase.h"
#include "AEnemyInfected.generated.h"

class UUtilityAIComponent;

/**
 * 
 */
UCLASS()
class ADAPTATIVEAI_TFG_API AAEnemyInfected : public AEnemyBase
{
	GENERATED_BODY()
	
public:
	AAEnemyInfected();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI | Utility")
	UUtilityAIComponent* UtilityAI;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AI | Utility")
	float StressLevel = 0.0f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:

	struct FDamageRecord {
		float Damage;
		float Timestamp;
	};

	static TArray<FDamageRecord> GlobalDamageRecords;
	static void RecordGlobalDamage(float DamageAmount, float CurrentTime);
	static float GetRecentGlobalDamage(float CurrentTime, float TimeWindow);

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float AlertCooldown;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float AlertRadius;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float AlertDuration;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float FuryDuration;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float FuryMultiplier;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float FuryTurnRateMultiplier;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	bool bIsFuryMode;

	UPROPERTY(EditAnywhere, Category = "AI | Abilities")
	float LungeDistance;

	UPROPERTY(EditAnywhere, Category = "AI | Throw")
	float ProjectileVelocity;

	UPROPERTY(EditAnywhere, Category = "AI | Throw")
	float MinThrowDistance;

	UPROPERTY(EditAnywhere, Category = "AI | Throw")
	float MaxThrowDistance;

	UPROPERTY(EditAnywhere, Category = "AI | Throw")
	float ThrowCooldown;

	UPROPERTY(EditAnywhere, Category = "AI | Throw")
	float ThrowPrecision;

	UPROPERTY(EditAnywhere, Category = "AI | Throw")
	TSubclassOf<AActor> StoneProjectileClass;

	UPROPERTY(EditAnywhere, Category = "AI | Utility")
	float SoundInvestigatePriority;

	UPROPERTY(EditAnywhere, Category = "AI | Dodge")
	bool bIsDodging;

	float DodgeEndTime;
	float LastDodgeTime;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	void UpdateBlackboardValues();

	void UpdateUtilityInputs();
	void UpdateStressLevel(float DeltaTime);

private:
	void UpdateFleeInput(class UBlackboardComponent* BlackboardComp);
	void UpdateFinalJumpInput(class UBlackboardComponent* BlackboardComp);
	void UpdateThrowInput(class UBlackboardComponent* BlackboardComp);

public:

	FTimerHandle UtilityTimerHandle;

	// Active when one infected detects the player, making nearby infected aware of the player's presence for a short time
	UFUNCTION(BlueprintCallable, Category = "AI | Abilities")
	void PerformLaCrida(AActor* TargetPlayer);

	// Active when the player abuse of height advantage and close-distance weapons, making the infected throw objects at the player
	UFUNCTION(BlueprintCallable, Category = "AI | Throw")
	void ThrowObject(AActor* TargetPlayer);

	// Active while chasing the player, doing zig-zag movements to make it harder to hit them with ranged weapons, and to avoid melee attacks  
	UFUNCTION(BlueprintCallable, Category = "AI | Abilities")
	FVector CalculateDodgeLocation(AActor* TargetPlayer);

	// Active when the infected is low on health, increasing its speed and damage for a short time, but making it more vulnerable to attacks
	void EnterFuryMode();

	// Active while looking for the player, making the infected investigate noises and other stimuli in the environment, increasing its perception range and priority for a short time
	void ChainAlert();

	// Active when the player is close enough, doing a quick attack.
	UFUNCTION(BlueprintCallable, Category = "AI | Abilities")
	void NearbyAttack(AActor* TargetPlayer);

	// Active when the health of the infected is low, doing a powerful jump attack that can hit the player from a distance.
	UFUNCTION(BlueprintCallable, Category = "AI | Abilities")
	void FinalAttackJump(AActor* TargetPlayer);
};
