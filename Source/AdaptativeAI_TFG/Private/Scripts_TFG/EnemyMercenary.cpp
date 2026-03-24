// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/EnemyMercenary.h"
#include "Scripts_TFG/SquadManager.h"
#include "Scripts_TFG/EnemySquad.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

AEnemyMercenary::AEnemyMercenary()
{
}

void AEnemyMercenary::BeginPlay()
{
	Super::BeginPlay();

	AActor* FoundManager = UGameplayStatics::GetActorOfClass(GetWorld(), ASquadManager::StaticClass());

	if(FoundManager)
	{
		MySquadManager = Cast<ASquadManager>(FoundManager);
		if(MySquadManager)
		{
			MySquadManager->RegisterSquadMember(this);
		}
	}
	GetWorldTimerManager().SetTimer(UtilityTimerHandle, this, &AEnemyMercenary::EvaluateUtilityScores, 0.5f, true);
}

void AEnemyMercenary::UpdateBlackboardValues()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();

	float HealthPct = CurrentHealth / MaxHealth;
	Blackboard->SetValueAsFloat("SelfHealthPct", HealthPct);
}

void AEnemyMercenary::EvaluateUtilityScores()
{
	UpdateBlackboardValues();
}

void AEnemyMercenary::ReloadWeapon()
{
}

void AEnemyMercenary::TakeCover()
{
}

void AEnemyMercenary::FlankEnemy()
{
}

void AEnemyMercenary::CommunicateWithSquad()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(AIController && AIController->GetBlackboardComponent() && MySquad)
	{
		AActor* TargetPlayer = Cast<AActor>(AIController->GetBlackboardComponent()->GetValueAsObject(FName("TargetPlayer")));
		if (TargetPlayer)
		{
			MySquad->SharePlayerLocation(TargetPlayer);
		}
	}
}

void AEnemyMercenary::SuppressEnemy()
{
}

void AEnemyMercenary::UpdateStressLevel(float DeltaTime)
{
}

void AEnemyMercenary::SquadOrganization()
{
}

void AEnemyMercenary::TactialRetreat()
{
}

void AEnemyMercenary::DefensiveMode()
{
}

void AEnemyMercenary::CleanRoom()
{
}

void AEnemyMercenary::IntensiveResearch()
{
}

void AEnemyMercenary::SniperMode()
{
}

void AEnemyMercenary::AggressiveMode()
{
}
