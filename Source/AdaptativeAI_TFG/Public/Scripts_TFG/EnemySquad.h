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

public: 

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	TArray<AEnemyMercenary*> SquadMembers;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad")
	EPlayerStrategy CurrentPlayerStrategy;

	FTimerHandle SquadBrainTimerHandle;

	void InitializeSquad(const TArray<AEnemyMercenary*>& Members);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void EvaluateSquadStrategy();

	void IssueOrderToMember(AEnemyMercenary* Member, ESquadOrder NewOrder);

	void SharePlayerLocation(AActor* TargetPlayer);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void RemoveMemeber(AEnemyMercenary* MemberToRemove);

	UFUNCTION(BlueprintCallable, Category = "Squad")
	void AlertAllMembers(AActor* TargetPlayer);
	
};