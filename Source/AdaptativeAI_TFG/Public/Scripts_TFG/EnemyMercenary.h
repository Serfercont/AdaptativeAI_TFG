// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "Scripts_TFG/EnemyBase.h"
#include "ShooterWeaponHolder.h"
#include "EnemyMercenary.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EEnemyRole : uint8
{
	None UMETA(DisplayName = "None"),
	Shotgun UMETA(DisplayName = "Shotgun"),
	Rifle UMETA(DisplayName = "Rifle"),
	Sniper UMETA(DisplayName = "Sniper")
};

UENUM(BlueprintType)
enum class ESquadOrder : uint8
{
	None UMETA(DisplayName = "None"),
	DefendArea UMETA(DisplayName = "Defend Area"),
	AttackPlayer UMETA(DisplayName = "Attack Player"),
	FreeFire UMETA(DisplayName = "Free Fire"),
	Supression UMETA(DisplayName = "Supression"),
	FlankLeft UMETA(DisplayName = "Flank Left"),
	FlankRight UMETA(DisplayName = "Flank Right"),
	CleanRoom UMETA(DisplayName = "Clean Room"),
	TacticalRetreat UMETA(DisplayName = "Tactical Retreat"),
	HoldPosition UMETA(DisplayName = "Hold Position"),
};

UCLASS()
class ADAPTATIVEAI_TFG_API AEnemyMercenary : public AEnemyBase, public IShooterWeaponHolder
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:

	AEnemyMercenary();

	UPROPERTY(EditAnywhere,BlueprintReadWrite, Category = "AI | Organization")
	EEnemyRole RoleType;

	UPROPERTY(EditAnywhere, Category = "AI | Organization")
	int32 SquadID;

	UPROPERTY(BlueprintReadWrite, Category = "AI | Organization")
	class AEnemySquad* MySquad;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float ReloadTime;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float FireRate;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float Accuracy;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float EffectiveRange;

	UPROPERTY(EditAnywhere, Category = "AI | WeaponStats")
	float AmmoCount;

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
	int32 PlayerStrategy;

	UPROPERTY(BlueprintReadWrite, Category = "AI | Organization")
	class ASquadManager* MySquadManager;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI | Weapon")
	class AShooterWeapon* EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	TSubclassOf<class AShooterWeapon> WeaponClass;

	UPROPERTY(EditDefaultsOnly,Category = "AI | Weapon")
	TSubclassOf<class AShooterWeapon> RifleWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	TSubclassOf<class AShooterWeapon> SniperWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	TSubclassOf<class AShooterWeapon> ShotgunWeaponClass;

	UPROPERTY(BlueprintReadOnly, Category = "AI | Combat")
	bool bIsInCombat;

	UPROPERTY(BlueprintReadOnly, Category = "AI | Combat")
	bool bIsSupressing = false;

	UPROPERTY(BlueprintReadOnly, Category = "AI | Combat")
	FVector SuppressionTargetLocation = FVector::ZeroVector;

	FTimerHandle UtilityTimerHandle;

	FTimerHandle ReloadTimerHandle;
	//Functions

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void EnterCombat();

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void ExitCombat();

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	bool PerformShoot(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void InitializeByRole();

	UFUNCTION(BlueprintCallable, Category = "AI | Tactics")
	void FindCoverPosition(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable, Category = "AI | Blackboard")
	void UpdateBlackboardValues();

	void EvaluateUtilityScores();

	UFUNCTION(BlueprintCallable, Category = "AI | Organization")
	void NotifySquadOfDeath();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void SetFacingMode(bool bFacePlayer);


	// Reload the weapon when the amo count is low or after a certain number of shots
	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void StartReloadWeapon();

	UFUNCTION(BlueprintCallable, Category = "AI | Tactics")
	FVector CalculateSniperPosition(AActor* Target);

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void PerformSuppressionFire(FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void StopSuppressionFire();

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void AssignSupressorRole();

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void AssignFlankerRole();

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void ClearCombatRole();

	UFUNCTION(BlueprintCallable, Category = "AI | Tactics")
	FVector CalculateFlankPosition(AActor* ThreatActor);

	UFUNCTION(BlueprintCallable)
	void RefreshSuppressionFocalPoint(FVector NewTarget);

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

	//ISHOOTERWEAPONHOLDER

	virtual void AttachWeaponMeshes(AShooterWeapon* WeaponToAttach) override;
	virtual FVector GetWeaponTargetLocation() override;
	virtual void PlayFiringMontage(UAnimMontage* Montage) override {} 
	virtual void AddWeaponRecoil(float Recoil) override {}             
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& InWeaponClass) override {}
	virtual void OnWeaponActivated(AShooterWeapon* InWeapon) override {}
	virtual void OnWeaponDeactivated(AShooterWeapon* InWeapon) override {}
	virtual void OnSemiWeaponRefire() override;

	UPROPERTY()
	AActor* CurrentAimTarget;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	float MinAimOffsetZ = -20.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	float MaxAimOffsetZ = 40.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	float AimVarianceHalfAngle = 3.f;  

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	float AimRange = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI | Weapon")
	FName ThirdPersonWeaponSocket = FName("weapon_r");

	UFUNCTION(BlueprintCallable, Category = "AI | Combat")
	void StopShooting();

	FTimerHandle InitHandle;

	virtual void Tick(float DeltaTime) override;
	float BlackboardUpdateAccumulator = 0.f;
	static constexpr float BlackboardUpdateInterval = 0.1f;

};
