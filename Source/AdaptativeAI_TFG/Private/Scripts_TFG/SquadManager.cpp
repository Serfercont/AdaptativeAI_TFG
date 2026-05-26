// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/SquadManager.h"
#include "Scripts_TFG/EnemySquad.h"
#include "Scripts_TFG/EnemyMercenary.h"
#include "Engine/Engine.h"
#include "BehaviorTree/BlackboardComponent.h"

ASquadManager::ASquadManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASquadManager::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(OrganizeTimerHandle, this, &ASquadManager::OrganizeSquads, 1.0f, false);
}

void ASquadManager::OrganizeSquads()
{
	int32 SquadCounter = 1;

	while(UnassignedMercenaries.Num() > 0)
	{
		TArray<AEnemyMercenary*> GroupOf5;
		for(int32 i = 0; i < 5; i++)
		{
			if (UnassignedMercenaries.Num() > 0)
			{
				GroupOf5.Add(UnassignedMercenaries[0]);
				UnassignedMercenaries.RemoveAt(0);
			}
		}

		AEnemySquad* NewSquad = GetWorld()->SpawnActor<AEnemySquad>(AEnemySquad::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);

		if (NewSquad)
		{
			for (int j = 0; j < GroupOf5.Num(); ++j)
			{
				GroupOf5[j]->SquadID = SquadCounter;
				if (j == 0)
				{
					GroupOf5[j]->RoleType = EEnemyRole::Sniper;
				}
				else if( j == 1 || j == 2)
				{
					GroupOf5[j]->RoleType = EEnemyRole::Rifle;
				}
				else
				{
					GroupOf5[j]->RoleType = EEnemyRole::Shotgun;
				}
			}

			NewSquad->InitializeSquad(GroupOf5);

			if (GEngine)
			{
				FString DebugMsg = FString::Printf(TEXT("Escuadron %d Creado! Miembros: %d."), SquadCounter, GroupOf5.Num());
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, DebugMsg);
			}
		}

		SquadCounter++;
	}
}

void ASquadManager::RegisterSquadMember(AEnemyMercenary* NewMember)
{
	if(NewMember && !UnassignedMercenaries.Contains(NewMember))
	{
		UnassignedMercenaries.Add(NewMember);
	}
}


