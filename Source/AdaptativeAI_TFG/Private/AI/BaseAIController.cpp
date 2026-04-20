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

void ABaseAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	HandlePerceptionUpdate(Actor, Stimulus);
}

void ABaseAIController::HandlePerceptionUpdate(AActor* Actor, FAIStimulus Stimulus)
{
    UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Error, TEXT("[BaseAIC] HandlePerceptionUpdate: BlackboardComp es NULL en %s"), *GetNameSafe(GetPawn()));
        return;
    }

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
        UE_LOG(LogTemp, Warning, TEXT("[BaseAIC] %s VE al jugador"), *GetNameSafe(GetPawn()));

        BlackboardComp->SetValueAsBool(TEXT("IsPlayerVisible"), true);
        BlackboardComp->SetValueAsObject(TEXT("TargetActor"), Actor);
        BlackboardComp->SetValueAsVector(TEXT("LastKnownLocation"), Actor->GetActorLocation());
        if (Enemy)
        {
            Enemy->SetMovementState(true);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[BaseAIC] %s PIERDE al jugador"), *GetNameSafe(GetPawn()));

        BlackboardComp->SetValueAsBool(TEXT("IsPlayerVisible"), false);

        if (Enemy && !Enemy->IsA(AAEnemyInfected::StaticClass()))
        {
            BlackboardComp->ClearValue(TEXT("TargetActor"));
            Enemy->SetMovementState(false);
        }
    }
}
