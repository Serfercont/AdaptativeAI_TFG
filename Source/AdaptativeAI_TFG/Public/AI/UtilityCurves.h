// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "UtilityCurves.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EUtilityCurveType : uint8
{
	Linear				UMETA(DisplayName = "Linear"),
	Exponential			UMETA(DisplayName = "Exponential"),
	Logistic			UMETA(DisplayName = "Logistic"),
	Inverse				UMETA(DisplayName = "Inverse"),
	InverseExponential	UMETA(DisplayName = "Inverse Exponential")
};

class ADAPTATIVEAI_TFG_API FUtilityCurves
{
public:
	
	static float Linear(float X, float M = 1.0f, float B = 0.0f)
	{
		return FMath::Clamp(M * X + B, 0.0f, 1.0f);
	}

	static float Exponential(float X, float K = 2.0)
	{
		const float ClampedX = FMath::Clamp(X, 0.0f, 1.0f);
		return FMath::Clamp(FMath::Pow(ClampedX, K), 0.0f, 1.0f);
	}

	static float Logistic(float X, float K = 10.0f, float X0=0.5f)
	{
		const float Exponent = -K * (X - X0);
		const float Value = 1.0f / (1.0f + FMath::Exp(Exponent));
		return FMath::Clamp(Value, 0.0f, 1.0f);
	}

	static float Inverse(float X)
	{
		return FMath::Clamp(1.0f - FMath::Clamp(X, 0.0f, 1.0f), 0.0f, 1.0f);
	}

	static float InverseExponential(float X, float K = 2.0f)
	{
		const float ClampedX = FMath::Clamp(X, 0.0f, 1.0f);
		const float Emptiness = 1.0 - ClampedX;
		return FMath::Clamp(FMath::Pow(Emptiness, K), 0.0f, 1.0f);
	}

	static float Evaluate(EUtilityCurveType CurveType, float X, float ParamA = 1.0f, float ParamB = 0.0f)
	{
		switch (CurveType)
		{
		case EUtilityCurveType::Linear:
			return Linear(X, ParamA, ParamB);
		case EUtilityCurveType::Exponential:
			return Exponential(X, ParamA);
		case EUtilityCurveType::Logistic:
			return Logistic(X, ParamA, ParamB);
		case EUtilityCurveType::Inverse:
			return Inverse(X);
		case EUtilityCurveType::InverseExponential:
			return InverseExponential(X, ParamA);
		default:
			return 0.0f;
		}
	}
};
