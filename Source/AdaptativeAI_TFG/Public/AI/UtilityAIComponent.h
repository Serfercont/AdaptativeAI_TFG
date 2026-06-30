// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/UtilityCurves.h"
#include "UtilityAIComponent.generated.h"

class UBlackboardComponent;

/**
 * 
 */

UENUM(BlueprintType)
enum class EAdaptativeProfile : uint8
{
	None					UMETA(DisplayName = "None"),
	CounterAggresive		UMETA(DisplayName = "CounterAggressive"),
	CounterStealthy			UMETA(DisplayName = "CounterStealthy"),
	CounterCamper			UMETA(DisplayName = "CounterCamper")
};

USTRUCT(BlueprintType)
struct FUtilityAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	FName ActionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	FName InputKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	EUtilityCurveType CurveType = EUtilityCurveType::Linear;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float CurveParamA = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float CurveParamB = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float BaseWeight = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float MultCounterAggresive = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float MultCounterStealthy = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float MultCounterCamper = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	FName OutputScoreKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	FName OutputBoolKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	bool bInputIsNormalized = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility Action")
	float inputMax = 1.0f;
};


UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class ADAPTATIVEAI_TFG_API UUtilityAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUtilityAIComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility AI")
	TArray<FUtilityAction> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility AI")
	float EvaluationInterval = .5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility AI")
	float ActivationThreshold = .05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility AI")
	float StressSaturationLevel = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Utility AI")
	EAdaptativeProfile CurrentProfile = EAdaptativeProfile::None;

	UPROPERTY(BlueprintReadWrite, Category = "Utility AI")
	float StressLevel = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility AI")
	bool bDebugScores = false;

	UFUNCTION(BlueprintCallable, Category = "Utility AI")
	void SetEvaluationEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Utility AI")
	void SetAdaptativeProfile(EAdaptativeProfile NewProfile) {CurrentProfile = NewProfile;}

	UFUNCTION(BlueprintCallable, Category = "Utility AI")
	FName GetLastWinningAction() const { return LastWinningAction; }

private:

	void EvaluateActions();

	UBlackboardComponent* GetBlackboardComponent() const;

	float GetAdaptativeMultiplier(const FUtilityAction& Action) const;

	FTimerHandle EvaluationTimerHandle;

	FName LastWinningAction = NAME_None;

	bool bEvaluationEnabled = true;
};
