// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Scripts_TFG/EnemyMercenary.h"
#include "EnemySquad.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EPlayerStrategy : uint8
{
	None UMETA(DisplayName = "None"),
	Agressive UMETA(DisplayName = "Agressive"),
	Camping UMETA(DisplayName = "Camping"),
	Silent UMETA(DisplayName = "Silent"),
};

UCLASS()
class ADAPTATIVEAI_TFG_API AEnemySquad : public AInfo
{
	GENERATED_BODY()

public:

	AEnemySquad();

protected:

	virtual void BeginPlay() override;
	FVector LastRecordedPlayerPosition = FVector::ZeroVector;

	float PlayerStationaryStartTime = -1.0f;

	int32 RecentKills = 0;
	float LastKillTime = 0.0f;
	bool bDefensiveActive = false;
	FVector DefenseInitPosition = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Squad")
	float CampingDetectionTime = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Squad")
	float CampingMovementThreshold = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Squad")
	float KillWindowDuration = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Squad")
	int32 agressiveKillThreshold = 2;


	bool ResolvePlayerTarget(FVector& OutPlayerPosition, AActor*& OutTargetPlayer);
	EPlayerStrategy ClassifyPlayer(const FVector& CurrentPlayerPosition);
	void EnterStrategy(EPlayerStrategy NewStrategy);
	void TickStrategy(EPlayerStrategy Strategy, const FVector& CurrentPlayerPosition, AActor* TargetPlayer);
	void ExitStrategy(EPlayerStrategy OldStrategy);

	void CoordinateDefensiveRetreat();
	void CancelDefensiveRetreat();

public: 

	bool bFlankActivated = false;
	float TimePlayerLastSeen = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TArray<AEnemyMercenary*> SquadMembers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	EPlayerStrategy CurrentPlayerStrategy;

	FTimerHandle SquadBrainTimerHandle;

	FVector FlankInitiatedAtPosition;

	void InitializeSquad(const TArray<AEnemyMercenary*>& Members);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void EvaluateSquadStrategy();

	void IssueOrderToMember(AEnemyMercenary* Member, ESquadOrder NewOrder);

	void SharePlayerLocation(AActor* TargetPlayer);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void RemoveMemeber(AEnemyMercenary* MemberToRemove);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void AlertAllMembers(AActor* TargetPlayer);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void CoordinateFlankAndSupress();

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void CancelFlanking();

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void UpdateSuppressionTargets(FVector TargetLoc);
	
};