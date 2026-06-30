// Rights reserved by Sergio Fernandez


#include "AI/UtilityAIComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"


UUtilityAIComponent::UUtilityAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUtilityAIComponent::BeginPlay()
{
	Super::BeginPlay();

	if(EvaluationInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(EvaluationTimerHandle, this, &UUtilityAIComponent::EvaluateActions, EvaluationInterval, true);
	}
}

void UUtilityAIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUtilityAIComponent::SetEvaluationEnabled(bool bEnabled)
{
	bEvaluationEnabled = bEnabled;

	if (!bEnabled && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(EvaluationTimerHandle);
	}
	else if (bEnabled && GetWorld() && EvaluationInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(EvaluationTimerHandle, this, &UUtilityAIComponent::EvaluateActions, EvaluationInterval, true);
	}
}

UBlackboardComponent* UUtilityAIComponent::GetBlackboardComponent() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		AAIController* AIController = Cast<AAIController>(OwnerPawn->GetController());
		if (AIController)
		{
			return AIController->GetBlackboardComponent();
		}
	}
	return nullptr;
}

void UUtilityAIComponent::EvaluateActions()
{
	if(!bEvaluationEnabled)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if(!BlackboardComp)
	{
		return;
	}

	float BestScore = ActivationThreshold;
	int32 BestIndex = INDEX_NONE;

	for(int32 i= 0; i < Actions.Num(); ++i)
	{
		const FUtilityAction& Action = Actions[i];

		float RawInput = BlackboardComp->GetValueAsFloat(Action.InputKey);

		float NormalizedInput = RawInput;
		if(!Action.bInputIsNormalized && Action.inputMax > KINDA_SMALL_NUMBER)
		{
			NormalizedInput = FMath::Clamp(RawInput / Action.inputMax, 0.0f, 1.0f);
		}

		float UCurve = FUtilityCurves::Evaluate(Action.CurveType, NormalizedInput, Action.CurveParamA, Action.CurveParamB);

		float MAdapt = GetAdaptativeMultiplier(Action);
		float UFinal = (UCurve * Action.BaseWeight) * MAdapt;

		float UResult = FMath::Min(1.0f, UFinal);

		if(Action.OutputScoreKey != NAME_None)
		{
			BlackboardComp->SetValueAsFloat(Action.OutputScoreKey, UResult);
		}

		if(UResult > BestScore)
		{
			BestScore = UResult;
			BestIndex = i;
		}

		if (bDebugScores && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, EvaluationInterval, FColor::Green, FString::Printf(TEXT("Action: %s, Input: %.2f, Normalized: %.2f, Curve: %.2f, MAdapt: %.2f, Final: %.2f"), *Action.ActionName.ToString(), RawInput, NormalizedInput, UCurve, MAdapt, UResult));
		}
	}

	for(int32 i = 0; i < Actions.Num(); ++i)
	{
		const FUtilityAction& Action = Actions[i];
		if(Action.OutputBoolKey != NAME_None)
		{
			BlackboardComp->SetValueAsBool(Action.OutputBoolKey, i == BestIndex);
		}
	}
}

float UUtilityAIComponent::GetAdaptativeMultiplier(const FUtilityAction& Action) const
{
	if(StressLevel >= StressSaturationLevel)
	{
		return 1.0f;
	}

	switch (CurrentProfile)
	{
	case EAdaptativeProfile::CounterAggresive:
		return Action.MultCounterAggresive;

	case EAdaptativeProfile::CounterStealthy:
		return Action.MultCounterStealthy;

	case EAdaptativeProfile::CounterCamper:
		return Action.MultCounterCamper;

	case EAdaptativeProfile::None:

	default:
		return 1.0f;
	}
}
