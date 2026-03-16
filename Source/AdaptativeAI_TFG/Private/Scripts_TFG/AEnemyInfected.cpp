// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/AEnemyInfected.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"

TArray<AAEnemyInfected::FDamageRecord> AAEnemyInfected::GlobalDamageRecords;
AAEnemyInfected::AAEnemyInfected()
{
	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	AlertRadius = 5000.f;
	bIsDodging = false;
	LastDodgeTime = -15.f;
	Damage = 20.f;
	AttackRange = 150.f;
	AttackCooldown = 1.5f;
}

void AAEnemyInfected::BeginPlay()
{
	Super::BeginPlay();

	UpdateBlackboardValues();
	GetWorldTimerManager().SetTimer(UtilityTimerHandle, this, &AAEnemyInfected::EvaluateUtilityScores, 0.5f, true);
}

void AAEnemyInfected::RecordGlobalDamage(float DamageAmount, float CurrentTime)
{
	GlobalDamageRecords.Add({DamageAmount, CurrentTime });
}

float AAEnemyInfected::GetRecentGlobalDamage(float CurrentTime, float TimeWindow)
{
	float TotalDamage = 0.f;

	for (int i = GlobalDamageRecords.Num() - 1; i >= 0; i--)
	{
		if (CurrentTime - GlobalDamageRecords[i].Timestamp <= TimeWindow)
		{
			TotalDamage += GlobalDamageRecords[i].Damage;
		}
		else
		{
			GlobalDamageRecords.RemoveAt(i);
		}
	}
	return TotalDamage;
}

float AAEnemyInfected::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);

	RecordGlobalDamage(ActualDamage, GetWorld()->GetTimeSeconds());
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
			if (Actor != this && FVector::Dist(Actor->GetActorLocation(), GetActorLocation()) <= 2500.0f)
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

			BlackboardComp->SetValueAsFloat("DistanceToPlayer", Distance2D);
			BlackboardComp->SetValueAsBool("IsPlayerUnreachable", bIsUnreachable);
		}

		//Dodge Logic
		float CurrentTime = GetWorld()->GetTimeSeconds();
		float RecentDamage = GetRecentGlobalDamage(CurrentTime, 2.f);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.5f, FColor::Yellow, FString::Printf(TEXT("Daño global (2s): %f"), RecentDamage));
		}

		if (!bIsDodging && RecentDamage >= 50.f)
		{
			if (CurrentTime > LastDodgeTime + 2.f)
			{
				bIsDodging = true;
				DodgeEndTime = CurrentTime + 15.f;
			}
		}

		if (bIsDodging && CurrentTime > DodgeEndTime)
		{
			bIsDodging = false;
			LastDodgeTime = CurrentTime;
		}

		BlackboardComp->SetValueAsBool("IsDodging", bIsDodging);

		//Fury Mode Logic
		if (!bIsFuryMode && HealthPct <= 0.2f)
		 {
			 EnterFuryMode();
			 BlackboardComp->SetValueAsBool("IsFuryMode", true);
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

FVector AAEnemyInfected::CalculateDodgeLocation(AActor* TargetPlayer)
{
	if (!TargetPlayer)
	{
		return GetActorLocation();
	}

	FVector MyLocation = GetActorLocation();
	FVector PlayerLocation = TargetPlayer->GetActorLocation();

	FVector ToPlayer = PlayerLocation - MyLocation;
	ToPlayer.Z = 0.f;

	float DistanceToPlayer = ToPlayer.Size2D();

	if (DistanceToPlayer < LungeDistance + 150.f)
	{
		return PlayerLocation;
	}
	ToPlayer.Normalize();

	FVector RightVector = FVector::CrossProduct(ToPlayer, FVector::UpVector);

	float RandomDirection = FMath::RandBool() ? 1.f : -1.f;
	float DodgeAmplitude = FMath::RandRange(150.f, 220.f);
	float ForwardOffset = FMath::RandRange(350.f, 650.f);

	if (DistanceToPlayer < ForwardOffset)
	{
		ForwardOffset = DistanceToPlayer * 0.5f;
	}

	FVector DodgeLocation = MyLocation + (ToPlayer * ForwardOffset) + (RightVector * RandomDirection * DodgeAmplitude);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation NavLocation;

	if (NavSys && NavSys->ProjectPointToNavigation(DodgeLocation, NavLocation, FVector(500.f, 500.f, 500.f)))
	{
		DrawDebugSphere(GetWorld(), NavLocation.Location, 50.f, 12, FColor::Red, false, 2.f);
		return NavLocation.Location;
	}

	return PlayerLocation;
}

void AAEnemyInfected::EnterFuryMode()
{
	WalkSpeed *= FuryMultiplier;
	Damage *= FuryMultiplier;

	SetMovementState(true);

}

void AAEnemyInfected::ChainAlert()
{
}

void AAEnemyInfected::NearbyAttack(AActor* TargetPlayer)
{
	if (!TargetPlayer)
	{
		return;
	}

	float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());

	if (DistanceToPlayer <= AttackRange + 50.f)
	{
		UGameplayStatics::ApplyDamage(TargetPlayer, Damage, GetController(), this, UDamageType::StaticClass());
	}
}

void AAEnemyInfected::FinalAttackJump(AActor* TargetPlayer)
{
	if (!TargetPlayer)
	{
		return;
	}

	FVector StartLocation = GetActorLocation();
	FVector EndLocation = TargetPlayer->GetActorLocation();
	FVector PlayerLocation = TargetPlayer->GetActorLocation();

	//Prediction

	FVector PlayerVelocity = TargetPlayer->GetVelocity();
	PlayerVelocity.Z = 0.f;

	if (PlayerVelocity.SizeSquared() > 100.f)
	{
		float DistanceToPlayer = FVector::Dist2D(StartLocation, PlayerLocation);
		float EstimatedFlightTime = DistanceToPlayer / 800.f;

		EstimatedFlightTime = FMath::Clamp(EstimatedFlightTime, 0.5f, 1.5f);

		FVector PredictedOffset = PlayerVelocity * EstimatedFlightTime;
		FVector PredictedLocation = PlayerLocation + PredictedOffset;

		PredictedOffset = PredictedOffset.GetClampedToMaxSize(400.f);
		EndLocation = PlayerLocation + PredictedOffset;

		if (GEngine)
		{
			DrawDebugSphere(GetWorld(), EndLocation, 30.f, 12, FColor::Orange, false, 2.f);
			DrawDebugLine(GetWorld(), PlayerLocation, EndLocation, FColor::Orange, false, 2.f, 0, 2.f);
		}

	}

	//Jump Calculation
	FVector LaunchVelocity;

	bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, LaunchVelocity, StartLocation, EndLocation, 0.f, 0.5f);

	if (bSuccess)
	{
		LaunchVelocity *= 1.1f;

		LaunchCharacter(LaunchVelocity, true, true);
	}
}


