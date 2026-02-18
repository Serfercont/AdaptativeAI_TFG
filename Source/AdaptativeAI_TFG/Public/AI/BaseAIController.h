// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BaseAIController.generated.h"


/**
 * 
 */
UCLASS()
class ADAPTATIVEAI_TFG_API ABaseAIController : public AAIController
{
	GENERATED_BODY()


public:
	ABaseAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* PerceptionComp;

protected:
	virtual void BeginPlay() override;	

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
};
