// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Scripts_TFG/EnemyMercenary.h"
#include "EnemySquad.generated.h"

/**
 * 
 */
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

	UPROPERTY(EditAnywhere, Category = "Squad")
	float CampingDetectionTime = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Squad")
	float CampingMovementThreshold = 300.0f;

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