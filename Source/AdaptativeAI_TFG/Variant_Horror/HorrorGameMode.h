// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HorrorGameMode.generated.h"

/**
 *  Simple GameMode for a first person horror game
 */
UCLASS(abstract)
class ADAPTATIVEAI_TFG_API AHorrorGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:

	/** Constructor */
	AHorrorGameMode();
};
