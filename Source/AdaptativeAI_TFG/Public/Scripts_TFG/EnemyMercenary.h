// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "Scripts_TFG/EnemyBase.h"
#include "EnemyMercenary.generated.h"

/**
 * 
 */
UCLASS()
class ADAPTATIVEAI_TFG_API AEnemyMercenary : public AEnemyBase
{
	GENERATED_BODY()

public:

	AEnemyMercenary();

	UPROPERTY(EditAnywhere, Category = "AI | Organization")
	EEnemyRole RoleType;

	UPROPERTY(EditAnywhere, Category = "AI | Organization")
	int32 SquadID;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float ReloadTime;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float FireRate;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float Accuracy;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float EffectiveRange;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float AmoCount;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float ReactionTime;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float ComunicationRange;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float ComunicationCooldown;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float CoverSearchRadius;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float FlankingSearchRadius;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float coverPreference;

	UPROPERTY(EditAnywhere, Category = "AI | Tactics")
	float FlankingPreference;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float Precision;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float stressLevel;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float SuppressionLevel;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float SuppressionEffectiveness;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float SuppressionRecoveryRate;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float SuppressionThreshold;

	UPROPERTY(EditAnywhere, Category = "AI | Combat")
	float SuppressionDamageMultiplier;

	UPROPERTY(EditAnywhere, Category = "AI | Utility")
	int PlayerStrategyPreference;


	//Functions

	// Reload the weapon when the amo count is low or after a certain number of shots
	void ReloadWeapon();

	// Active when the mercenary is under heavy fire, making it take cover to reduce incoming damage
	void TakeCover();

	// Active on Agressive Mode, syncronizing flank movement with other states.
	void FlankEnemy();

	// Active when the mercenary is in a squad, allowing it to share information about the player's position and coordinate attacks with other squad members.
	void CommunicateWithSquad();

	// Active when the mercenary is under heavy fire, making it suppress the player's position to reduce their accuracy and make it harder for them to attack the mercenary
	void SuppressEnemy();

	// Update a number that controls the mercenary's stress level, which can affect the behavior.
	void UpdateStressLevel(float DeltaTime);

	// Active at the beginning of the game, organizing the mercenaries into squads based on their roles and preferences, and assigning them to different areas of the map to control.
	void SquadOrganization();

	// Active when the mercenary is low on health or out of amo, making it retreat to a safe position to recover
	void TactialRetreat();

	// Active when the player is using a hit-and-run strategy, making the mercenary focus on defense and evasion to avoid taking damage
	void DefensiveMode();

	// Active when the player is using a close-range strategy, making the mercenary focus on clearing rooms and tight spaces to prevent the player from getting close
	void CleanRoom();

	// Active when the player is silent, making the mercenary focus on gathering information and researching the player
	void IntensiveResearch();

	// logic to the sniper type.
	void SniperMode();

	// Active when the player is using a defensive strategy. coordinating flank and supression function.
	void AggressiveMode();

};
