// Derechos reservados por Sergio Fernandez

#include "AI/BaseAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

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
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

    if (!BlackboardComp)
    {
        return;
    }

    if (Actor && Stimulus.WasSuccessfullySensed())
    {
		BlackboardComp->SetValueAsObject(TEXT("TargetActor"), Actor);
    }

    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor %s was lost!"), *Actor->GetName());
        BlackboardComp->ClearValue(TEXT("TargetActor"));
    }
}