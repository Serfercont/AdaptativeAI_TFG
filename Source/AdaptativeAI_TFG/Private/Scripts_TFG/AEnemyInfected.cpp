// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/AEnemyInfected.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"


AAEnemyInfected::AAEnemyInfected()
{
	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	AlertRadius = 5000.f;
}

void AAEnemyInfected::BeginPlay()
{
	Super::BeginPlay();

	UpdateBlackboardValues();
	GetWorldTimerManager().SetTimer(UtilityTimerHandle, this, &AAEnemyInfected::EvaluateUtilityScores, 0.5f, true);
}

float AAEnemyInfected::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);

	UpdateBlackboardValues();

	if (CurrentHealth <= 0.f)
	{
		Destroy();
	}

	return ActualDamage;
}

void AAEnemyInfected::EvaluateUtilityScores()
{
	//First utility ai implementation , calculating the flee score based on health and nearby allies
	AAIController* AIController = Cast<AAIController>(GetController());

	if (!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();

	float HealthPct = CurrentHealth / MaxHealth;
	int32 NearbyAllies = BlackboardComp->GetValueAsInt(TEXT("NearbyAllies"));

	//flee curve, inverse exponential
	float FleeCurveValue = 0.0f;

	if (HealthPct < 0.3f)
	{
		FleeCurveValue = FMath::Pow(1.0f - HealthPct, 2.0f);
	}

	float FleeBaseWeight = 0.8f;
	float AdapatativeMultiplier = 1.0f;

	float FleeUtilityFinal = FleeBaseWeight * FleeCurveValue * AdapatativeMultiplier;
	FleeUtilityFinal = FMath::Clamp(FleeUtilityFinal, 0.0f, 1.0f);

	UpdateBlackboardValues();
}

void AAEnemyInfected::UpdateBlackboardValues()
{
	// Updates values in the blackboard to make decisions
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBlackboardComponent())
	{
		UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
		//HealthPct
		float HealthPct = CurrentHealth / MaxHealth;
		AIController->GetBlackboardComponent()->SetValueAsFloat("SelfHealthPct", HealthPct);

		//NearbyAllies
		int32 AlliesCount = 0;
		TArray<AActor*> FoundActors;

		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAEnemyInfected::StaticClass(), FoundActors);

		for (AActor* Actor : FoundActors)
		{
			if (Actor != this && FVector::Dist(Actor->GetActorLocation(), GetActorLocation()) <= 1000.0f)
			{
				AlliesCount++;
			}
		}

		BlackboardComp->SetValueAsInt("NearbyAllies", AlliesCount);

		//TargetActor
		AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject("TargetActor"));

		if (TargetActor)
		{
			float HeightDifference = TargetActor->GetActorLocation().Z - GetActorLocation().Z;
			float Distance2D = FVector::Dist2D(TargetActor->GetActorLocation(), GetActorLocation());
			bool bIsUnreachable = HeightDifference > 200.0f && ((Distance2D > MinThrowDistance) && (Distance2D < MaxThrowDistance));

			BlackboardComp->SetValueAsBool("IsPlayerUnreachable", bIsUnreachable);
		}
	}
}

void AAEnemyInfected::PerformLaCrida(AActor* TargetPlayer)
{
	if (!TargetPlayer)
	{
		return;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AAEnemyInfected::StaticClass(), FoundActors);

	for (AActor* Actor : FoundActors)
	{
		if (Actor != this && FVector::Dist(Actor->GetActorLocation(), GetActorLocation()) <= AlertRadius)
		{
			AAIController* AllyAI = Cast<AAIController>(Cast<APawn>(Actor)->GetController());
			if (AllyAI && AllyAI->GetBlackboardComponent())
			{
				AllyAI->GetBlackboardComponent()->SetValueAsObject("TargetActor", TargetPlayer);
				AllyAI->GetBlackboardComponent()->SetValueAsBool("IsAlerted", true);
				AllyAI->GetBlackboardComponent()->SetValueAsVector("LastKnownLocation", TargetPlayer->GetActorLocation());
				AllyAI->GetBlackboardComponent()->SetValueAsBool("HasPerformedCrida", true);
			}
		}
	}
}

void AAEnemyInfected::ThrowObject(AActor* TargetPlayer)
{
	if (!TargetPlayer || !StoneProjectileClass)
	{
		return;
	}

	FVector SpawnLocation = GetActorLocation() + FVector(0, 0, 0);
	FVector EndLocation = TargetPlayer->GetActorLocation();
	FVector LaunchVelocity;

	bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, LaunchVelocity, SpawnLocation, EndLocation, 0.f, 0.5f);

	if (bSuccess)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		
		AActor* Projectile = GetWorld()->SpawnActor<AActor>(StoneProjectileClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (Projectile)
		{
			UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Projectile->GetComponentByClass(UStaticMeshComponent::StaticClass()));
			if (MeshComp)
			{
				MeshComp->SetSimulatePhysics(true);
				MeshComp->SetPhysicsLinearVelocity(LaunchVelocity);
			}
		}
	}
}

void AAEnemyInfected::Dodge()
{
}

void AAEnemyInfected::EnterFuryMode()
{
}

void AAEnemyInfected::ExitFuryMode()
{
}

void AAEnemyInfected::ChainAlert()
{
}

void AAEnemyInfected::NearbyAttack()
{
}

void AAEnemyInfected::FinalAttackJump()
{
}

void AAEnemyInfected::RunAway()
{
}
