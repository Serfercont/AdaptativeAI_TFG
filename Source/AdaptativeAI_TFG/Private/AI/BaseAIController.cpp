// Derechos reservados por Sergio Fernandez

#include "AI/BaseAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Scripts_TFG/EnemyBase.h"
#include "Scripts_TFG/AEnemyInfected.h"

ABaseAIController::ABaseAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponent"));
}

void ABaseAIController::BeginPlay()
{
    Super::BeginPlay();
    if (PerceptionComp)
    {
        PerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &ABaseAIController::OnPerceptionUpdated);
    }
}

// Function to handle perception updates, setting the target actor and last known location in the blackboard, and updating the enemy's movement state
void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

    if (!BlackboardComp || !Actor)
    {
        return;
    }

	AEnemyBase* Enemy = Cast<AEnemyBase>(GetPawn());
	APawn* PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn();

    if (Actor != PlayerPawn)
    {
		return;
    }

    if (Stimulus.WasSuccessfullySensed())
    {
		BlackboardComp->SetValueAsBool(TEXT("IsPlayerVisible"), true);
		BlackboardComp->SetValueAsObject(TEXT("TargetActor"), Actor);
		BlackboardComp->SetValueAsVector(TEXT("LastKnownLocation"), Actor->GetActorLocation());
        if(Enemy)
        {
            Enemy->SetMovementState(true);
		}
    }
    else
    {
		BlackboardComp->SetValueAsBool(TEXT("IsPlayerVisible"), false);
        
        if (Enemy && !Enemy->IsA(AAEnemyInfected::StaticClass()))
        { 
            BlackboardComp->ClearValue(TEXT("TargetActor"));
            Enemy->SetMovementState(false);
        }
    }
}