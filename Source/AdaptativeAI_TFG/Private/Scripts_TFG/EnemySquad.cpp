// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/EnemySquad.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/Engine.h"

AEnemySquad::AEnemySquad()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentPlayerStrategy = EPlayerStrategy::None;
}

void AEnemySquad::BeginPlay()
{
	Super::BeginPlay();
}

void AEnemySquad::InitializeSquad(const TArray<AEnemyMercenary*>& Members)
{
	SquadMembers = Members;

	for(AEnemyMercenary* Member : SquadMembers)
	{
		Member->MySquad = this;
	}

	GetWorldTimerManager().SetTimer(SquadBrainTimerHandle, this, &AEnemySquad::EvaluateSquadStrategy, 2.0f, true);
}


void AEnemySquad::EvaluateSquadStrategy()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("Cerebro de Escuadron evaluando tacticas..."));
	}

	if(CurrentPlayerStrategy == EPlayerStrategy::Defensive)
	{
		//Strategy To counter Defensive Player Strategy	
	}
	else if(CurrentPlayerStrategy == EPlayerStrategy::Agressive)
	{
		//Strategy To counter Agressive Player Strategy
	}
	else if (CurrentPlayerStrategy == EPlayerStrategy::Silent)
	{
		//Strategy To counter Silent Player Strategy
	}
	
}

void AEnemySquad::IssueOrderToMember(AEnemyMercenary* Member, ESquadOrder NewOrder)
{
	AAIController* AIController = Cast<AAIController>(Member->GetController());
	if (AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsEnum(FName("SquadOrder"), static_cast<uint8>(NewOrder));
	}
}

void AEnemySquad::SharePlayerLocation(AActor* TargetPlayer)
{
	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (Member)
		{
			AAIController* AIController = Cast<AAIController>(Member->GetController());
			if (AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->SetValueAsVector(FName("LastKnownLocation"), TargetPlayer->GetActorLocation());
				AIController->GetBlackboardComponent()->SetValueAsBool(FName("IsPlayerVisible"), true);
				AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), TargetPlayer);
			}
		}
	}
}

bool AEnemySquad::RequestAttackSlot(AEnemyMercenary* RequestingMember)
{
	if(CurrentAttackers.Contains(RequestingMember))
	{
		return true;
	}
	if(CurrentAttackers.Num() >= MaxAttackSlots)
	{
		return false;
	}
	CurrentAttackers.Add(RequestingMember);
	return true;
}

void AEnemySquad::ReleaseAttackSlot(AEnemyMercenary* ReleasingMember)
{
	if(CurrentAttackers.Contains(ReleasingMember))
	{
		CurrentAttackers.Remove(ReleasingMember);
	}
}

void AEnemySquad::RemoveMemeber(AEnemyMercenary* MemberToRemove)
{
	if(!MemberToRemove)
	{
		return;
	}

	SquadMembers.Remove(MemberToRemove);

	ReleaseAttackSlot(MemberToRemove);

	if(SquadMembers.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(SquadBrainTimerHandle);
		Destroy();
	}
}
