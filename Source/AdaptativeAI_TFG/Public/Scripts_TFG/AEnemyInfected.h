// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "Scripts_TFG/EnemyBase.h"
#include "AEnemyInfected.generated.h"

/**
 * 
 */
UCLASS()
class ADAPTATIVEAI_TFG_API AAEnemyInfected : public AEnemyBase
{
	GENERATED_BODY()
	
public:
	AAEnemyInfected();

protected:
	virtual void BeginPlay() override;

public:

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

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

	//Utility AI
	UFUNCTION()
	void EvaluateUtilityScores();

	void UpdateBlackboardValues();

	FTimerHandle UtilityTimerHandle;

	// Active when one infected detects the player, making nearby infected aware of the player's presence for a short time
	UFUNCTION(BlueprintCallable, Category = "AI | Abilities")
	void PerformLaCrida(AActor* TargetPlayer);

	// Active when the player abuse of height advantage and close-distance weapons, making the infected throw objects at the player
	UFUNCTION(BlueprintCallable, Category = "AI | Throw")
	void ThrowObject(AActor* TargetPlayer);

	// Active while chasing the player, doing zig-zag movements to make it harder to hit them with ranged weapons, and to avoid melee attacks  
	void Dodge();

	// Active when the infected is low on health, increasing its speed and damage for a short time, but making it more vulnerable to attacks
	void EnterFuryMode();

	// Active when the fury mode ends, resetting the infected's speed and damage to normal values
	void ExitFuryMode();

	// Active while looking for the player, making the infected investigate noises and other stimuli in the environment, increasing its perception range and priority for a short time
	void ChainAlert();

	// Active when the player is close enough, doing a quick attack.
	void NearbyAttack();

	// Active when the health of the infected is low, doing a powerful jump attack that can hit the player from a distance.
	UFUNCTION(BlueprintCallable, Category = "AI | Abilities")
	void FinalAttackJump(AActor* TargetPlayer);

	// Active when an infected is low on health or alone, it will escape to other infected group.
	void RunAway();
};
