// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "BaseAIController.h"
#include "MercenaryAIController.generated.h"

class AEnemyMercenary;
class UBehaviorTree;

UCLASS()
class ADAPTATIVEAI_TFG_API AMercenaryAIController : public ABaseAIController
{
	GENERATED_BODY()

public:
	AMercenaryAIController(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void HandlePerceptionUpdate(AActor* Actor, FAIStimulus Stimulus) override;
	void OnLostPlayerConfirmed();

public:
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* MercenaryBehaviorTree;

private:

	UPROPERTY()
	AEnemyMercenary* MercenaryPawn;

	FTimerHandle LoseTargetTimerHandle;
	float LoseTargetGraceTime = 5.0f;
};
