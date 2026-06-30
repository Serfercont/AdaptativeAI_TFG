// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/AEnemyInfected.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "NavigationSystem.h"
#include "DrawDebugHelpers.h"
#include "AI/UtilityAIComponent.h"
#include "ShooterProjectile.h"
#include "Components/CapsuleComponent.h"

TArray<AAEnemyInfected::FDamageRecord> AAEnemyInfected::GlobalDamageRecords;
AAEnemyInfected::AAEnemyInfected()
{
	MaxHealth = 100.f;
	CurrentHealth = MaxHealth;
	AlertRadius = 5000.f;
	bIsDodging = false;
	LastDodgeTime = -15.f;
	Damage = 70.f;
	AttackRange = 150.f;
	AttackCooldown = 1.5f;

	UtilityAI = CreateDefaultSubobject<UUtilityAIComponent>(TEXT("UtilityAIComponent"));
}

void AAEnemyInfected::BeginPlay()
{
	Super::BeginPlay();
	GlobalDamageRecords.Empty();

	ConfigureUtilityActions();

	UpdateBlackboardValues();
	UpdateUtilityInputs();

	GetWorldTimerManager().SetTimer(UtilityTimerHandle, this,&AAEnemyInfected::UpdateUtilityInputs, 0.2f, true);
}

void AAEnemyInfected::ConfigureUtilityActions()
{
	if(!UtilityAI)
	{
		return;
	}

	UtilityAI->EvaluationInterval = 0.5f;
	UtilityAI->ActivationThreshold = 0.35f;
	UtilityAI->StressSaturationLevel = 1.0f;

	UtilityAI->Actions.Empty();

	// Action: Flee
	{
		FUtilityAction Flee;
		Flee.ActionName				= "Flee";
		Flee.InputKey				= "FleeInput";
		Flee.CurveType				= EUtilityCurveType::Linear;
		Flee.CurveParamA			= 1.0f;
		Flee.CurveParamB			= 0.0f;
		Flee.BaseWeight				= 0.9f;
		Flee.OutputScoreKey			= "FleeScore";
		Flee.OutputBoolKey			= "ShouldFlee";
		Flee.bInputIsNormalized		= true;
		Flee.inputMax				= 1.0f;

		UtilityAI->Actions.Add(Flee);
	}

	//Action: FinalJump
	{
		FUtilityAction FinalJump;
		FinalJump.ActionName			= "FinalJump";
		FinalJump.InputKey				= "FinalJumpInput";
		FinalJump.CurveType				= EUtilityCurveType::Inverse;
		FinalJump.CurveParamA			= 1.0f;
		FinalJump.CurveParamB			= 0.0f;
		FinalJump.BaseWeight			= 0.7f;
		FinalJump.OutputScoreKey		= "FinalJumpScore";
		FinalJump.OutputBoolKey			= "ShouldFinalJump";
		FinalJump.bInputIsNormalized	= true;
		FinalJump.inputMax				= 1.0f;

		UtilityAI->Actions.Add(FinalJump);
	}

	//Action: Throw
	{
		FUtilityAction Throw;
		Throw.ActionName			= "ThrowStone";
		Throw.InputKey				= "ThrowInput";
		Throw.CurveType				= EUtilityCurveType::Logistic;
		Throw.CurveParamA			= 8.0f;
		Throw.CurveParamB			= 0.4f;
		Throw.BaseWeight			= 0.7f;
		Throw.OutputScoreKey		= "ThrowScore";
		Throw.OutputBoolKey			= "ShouldThrow";
		Throw.bInputIsNormalized	= true;
		Throw.inputMax				= 1.0f;

		UtilityAI->Actions.Add(Throw);
	}

}

void AAEnemyInfected::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	UpdateStressLevel(DeltaTime);

	if (UtilityAI)
	{
		UtilityAI->StressLevel=StressLevel;
	}
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

void AAEnemyInfected::UpdateUtilityInputs()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}
	UpdateBlackboardValues();

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();

	UpdateFleeInput(BlackboardComp);
	UpdateFinalJumpInput(BlackboardComp);
	UpdateThrowInput(BlackboardComp);

	if (UtilityAI)
	{
		UtilityAI->StressLevel=StressLevel;
	}
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
			if (Actor != this && FVector::Dist(Actor->GetActorLocation(), GetActorLocation()) <= 2000.0f)
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

		if (!bIsDodging && RecentDamage >= 50.f)
		{
			if (CurrentTime > LastDodgeTime + 2.f)
			{
				bIsDodging = true;
				DodgeEndTime = CurrentTime + 10.f;
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

void AAEnemyInfected::UpdateStressLevel(float DeltaTime)
{
	const float Now = GetWorld()->GetTimeSeconds();
	const float RecentDamage = GetRecentGlobalDamage(Now, 3.f);

	const float StressTarget = FMath::Clamp(RecentDamage / 150.f, 0.f, 1.f);

	if(StressTarget > StressLevel)
	{
		StressLevel = FMath::FInterpTo(StressLevel, StressTarget, DeltaTime, 4.f);
	}
	else
	{
		StressLevel = FMath::FInterpTo(StressLevel, StressTarget, DeltaTime, 0.5f);
	}

	StressLevel = FMath::Clamp(StressLevel, 0.f, 1.f);
}

void AAEnemyInfected::UpdateFleeInput(UBlackboardComponent* BlackboardComp)
{
	const float HealthPct = (MaxHealth > 0.f) ? CurrentHealth / MaxHealth : 0.f;

	const int32 NearbyAllies = BlackboardComp->GetValueAsInt("NearbyAllies");
	const float LowHealthFactor = 1.f - HealthPct;

	const float AloneFactor = (NearbyAllies <= 0) ? 1.f : 0.6f;
	const float FleeInput = FMath::Clamp(LowHealthFactor * AloneFactor, 0.f, 1.f);

	BlackboardComp->SetValueAsFloat("FleeInput", FleeInput);
}

void AAEnemyInfected::UpdateFinalJumpInput(UBlackboardComponent* BlackboardComp)
{
	const float HealthPct = (MaxHealth > 0.f) ? CurrentHealth / MaxHealth : 0.f;
	BlackboardComp->SetValueAsFloat("FinalJumpInput", HealthPct);
}

void AAEnemyInfected::UpdateThrowInput(UBlackboardComponent* BlackboardComp)
{
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject("TargetActor"));
	float ThrowInput = 0.f;
	if (TargetActor)
	{
		const float HeighDiff = TargetActor->GetActorLocation().Z - GetActorLocation().Z;
		const float Distance2D = FVector::Dist2D(TargetActor->GetActorLocation(), GetActorLocation());
		const float HeightFactor = FMath::Clamp(HeighDiff / 200.f, 0.f, 1.f);

		const bool bPlayerAbove = HeighDiff > 200.f;
		const bool bInThrowRange = (Distance2D > MinThrowDistance) && (bPlayerAbove || Distance2D < MaxThrowDistance);
		ThrowInput = (bInThrowRange) ? HeightFactor : 0.f;
	}
	BlackboardComp->SetValueAsFloat("ThrowInput", ThrowInput);
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

	FVector SpawnLocation = GetActorLocation() + FVector(0.f, 0.f, 80.0f);
	FVector EndLocation = TargetPlayer->GetActorLocation();
	FVector LaunchVelocity;

	bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, LaunchVelocity, SpawnLocation, EndLocation, 0.f, 0.8f);

	if (bSuccess)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		
		AShooterProjectile* Projectile = GetWorld()->SpawnActor<AShooterProjectile>(StoneProjectileClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams);

		if (Projectile)
		{
			if (UPrimitiveComponent* ProjRoot = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
			{
				ProjRoot->IgnoreActorWhenMoving(this, true);
				ProjRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
			Projectile->LaunchWithVelocity(LaunchVelocity);

			FTimerHandle ReenableTimerHandle;
			TWeakObjectPtr<AShooterProjectile> WeakProjectile = Projectile;
			GetWorld()->GetTimerManager().SetTimer(ReenableTimerHandle, [WeakProjectile]()
			{
				if (WeakProjectile.IsValid())
				{
					if (UPrimitiveComponent* ProjRoot = Cast<UPrimitiveComponent>(WeakProjectile->GetRootComponent()))
					{
						ProjRoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
					}
				}
			}, 1.0f, false);
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
	float DodgeAmplitude = FMath::RandRange(180.f, 250.f);
	float ForwardOffset = FMath::RandRange(350.f, 650.f);

	if (DistanceToPlayer < ForwardOffset)
	{
		ForwardOffset = DistanceToPlayer * 0.5f;
	}

	FVector DodgeLocation = MyLocation + (ToPlayer * ForwardOffset) + (RightVector * RandomDirection * DodgeAmplitude);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation NavLocation;

	return DodgeLocation;
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

FString AAEnemyInfected::GetDebugStateText() const
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return TEXT("No Controller or Blackboard");
	}

	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();

	if(BlackboardComp->GetValueAsBool("IsFuryMode"))
	{
		return TEXT("Fury Mode");
	}
	if(BlackboardComp->GetValueAsBool("ShouldFlee"))
	{
		return TEXT("Huyendo");
	}
	if(BlackboardComp->GetValueAsBool("ShouldFinalJump"))
	{
		return TEXT("Salto");
	}
	if(BlackboardComp->GetValueAsBool("ShouldThrow"))
	{
		return TEXT("Lanzando");
	}
	if(BlackboardComp->GetValueAsBool("IsDodging"))
	{
		return TEXT("Esquivando");
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject("TargetActor"));
	if(TargetActor)
	{
		float DistanceToPlayer = BlackboardComp->GetValueAsFloat("DistanceToPlayer");
		if(DistanceToPlayer <= 150.f)
		{
			return TEXT("Atacando");
		}
		return TEXT("Persiguiendo");
	}

	return TEXT("Patrullando");
}


