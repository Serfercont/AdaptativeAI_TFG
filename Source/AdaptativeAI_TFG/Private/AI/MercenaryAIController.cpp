// Rights reserved by Sergio Fernandez


#include "AI/MercenaryAIController.h"
#include "Scripts_TFG/EnemyMercenary.h"
#include "Scripts_TFG/EnemySquad.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"


AMercenaryAIController::AMercenaryAIController(const FObjectInitializer& ObjectInitializer):Super(ObjectInitializer)
{
}

void AMercenaryAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	MercenaryPawn = Cast<AEnemyMercenary>(InPawn);

	if (MercenaryPawn && MercenaryBehaviorTree)
	{
		bool bSuccess =RunBehaviorTree(MercenaryBehaviorTree);
	}
}

void AMercenaryAIController::OnUnPossess()
{
	if (MercenaryPawn)
	{
		//MercenaryPawn->ReleaseAttackSlot();
		MercenaryPawn->NotifySquadOfDeath();
	}

	Super::OnUnPossess();
}

void AMercenaryAIController::HandlePerceptionUpdate(AActor* Actor, FAIStimulus Stimulus)
{

	Super::HandlePerceptionUpdate(Actor, Stimulus);

	if (!MercenaryPawn || !MercenaryPawn->MySquad)
	{
		return;
	}

	APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();
	if (Actor != PlayerPawn)
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		GetWorldTimerManager().ClearTimer(LoseTargetTimerHandle);

		if(MercenaryPawn->bIsInCombat)
		{
			MercenaryPawn->MySquad->SharePlayerLocation(Actor);
		}
		else
		{
			MercenaryPawn->MySquad->AlertAllMembers(Actor);
		}	
	}
	else
	{
		if (GetBlackboardComponent())
		{
			GetBlackboardComponent()->SetValueAsBool("IsPlayerVisible", false);
		}
			GetWorldTimerManager().SetTimer(LoseTargetTimerHandle, this, &AMercenaryAIController::OnLostPlayerConfirmed, LoseTargetGraceTime, false);
	}
}

void AMercenaryAIController::OnLostPlayerConfirmed()
{
	if (MercenaryPawn)
	{
		MercenaryPawn->ExitCombat();
	}
}