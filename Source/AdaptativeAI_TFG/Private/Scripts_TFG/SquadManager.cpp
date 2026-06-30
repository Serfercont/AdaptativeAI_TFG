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

		AEnemyMercenary* FirstMember = UnassignedMercenaries[0];
		if(!FirstMember)
		{
			UnassignedMercenaries.RemoveAt(0);
			continue;
		}
		GroupOf5.Add(FirstMember);
		UnassignedMercenaries.RemoveAt(0);

		const FVector FirstMemberLocation = FirstMember->GetActorLocation();
		UnassignedMercenaries.Sort([FirstMemberLocation](const AEnemyMercenary& A, const AEnemyMercenary& B)
		{
				const float DistA = FVector::DistSquared(A.GetActorLocation(), FirstMemberLocation);
				const float DistB = FVector::DistSquared(B.GetActorLocation(), FirstMemberLocation);
				return DistA < DistB;
		});

		while(GroupOf5.Num() < 5 && UnassignedMercenaries.Num() > 0)
		{
			GroupOf5.Add(UnassignedMercenaries[0]);
			UnassignedMercenaries.RemoveAt(0);
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


