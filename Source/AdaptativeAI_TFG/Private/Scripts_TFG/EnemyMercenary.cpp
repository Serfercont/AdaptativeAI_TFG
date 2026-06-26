// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/EnemyMercenary.h"
#include "Scripts_TFG/SquadManager.h"
#include "Scripts_TFG/EnemySquad.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "ShooterWeapon.h"
#include "ShooterNPC.h"
#include "Scripts_TFG/EnemySquad.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/MercenaryAIController.h"
#include "AI/UtilityAIComponent.h"
#include <Kismet/KismetMathLibrary.h>
#include <NavigationSystem.h>

AEnemyMercenary::AEnemyMercenary()
{
	AIControllerClass = AMercenaryAIController::StaticClass();
	PrimaryActorTick.bCanEverTick = true;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 120.f, 0.f);
	bUseControllerRotationYaw = false;

	UtilityAI = CreateDefaultSubobject<UUtilityAIComponent>(TEXT("UtilityAIComponent"));
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

	EffectiveRange = 2000.f;
	AmmoCount = 30.f;

	GetWorldTimerManager().SetTimer(InitHandle, this, &AEnemyMercenary::InitializeByRole, 1.2f, false);

	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, [this]()
		{
			AAIController* AIC = Cast<AAIController>(GetController());
			if (AIC && AIC->GetBlackboardComponent())
			{
				AIC->GetBlackboardComponent()->SetValueAsFloat("ApproachRadius", 1600.f);
			}
		}, 0.1f, false);

	GetWorldTimerManager().SetTimer(UtilityTimerHandle, this, &AEnemyMercenary::EvaluateUtilityScores, 0.5f, true);
}

void AEnemyMercenary::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!bIsInCombat)
	{
		return;
	}

	BlackboardUpdateAccumulator += DeltaTime;
	if(BlackboardUpdateAccumulator >= BlackboardUpdateInterval)
	{
		UpdateBlackboardValues();
		BlackboardUpdateAccumulator = 0.f;
	}
}

void AEnemyMercenary:: EnterCombat()
{
	bIsInCombat = true;

	AAIController* AIController = Cast<AAIController>(GetController());
	bool bIsSuppressor = false;
	bool bDefender = false;
	if(AIController && AIController->GetBlackboardComponent())
	{
		bIsSuppressor = AIController->GetBlackboardComponent()->GetValueAsBool("IsSuppressor");
		bDefender = AIController->GetBlackboardComponent()->GetValueAsBool("IsDefender");
	}

	if (bIsSuppressor || bDefender)
	{
		return;
	}

	SetFacingMode(true);
	SetMovementState(true);
	UpdateChargeSpeed();
}

void AEnemyMercenary::ExitCombat()
{
	bIsInCombat = false;
	SetFacingMode(false);
	StopShooting();
	SetMovementState(false);
	
	if (!MySquad || !MySquad->bFlankActivated)
	{
		ClearCombatRole();
	}
}


bool AEnemyMercenary::PerformShoot(AActor* Target)
{
	if(!Target)
	{
		return false;
	}

	if(bIsSupressing)
	{
		return true;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if(AIController && AIController->GetBlackboardComponent())
	{
		if(AIController->GetBlackboardComponent()->GetValueAsBool("IsDefender") && !AIController->GetBlackboardComponent()->GetValueAsBool("HasClearLineOfSight"))
		{
			return false;
		}
	}

	float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if(DistanceToTarget > MaxEngagementRange)
	{
		return false;
	}

	if (EquippedWeapon)
	{
		if(EquippedWeapon->GetBulletCount() <= 0)
		{
			return false;
		}
	}
	else if(AmmoCount <= 0.f)
	{
		return false;
	}

	CurrentAimTarget = Target;

	if (EquippedWeapon)
	{
		EquippedWeapon->StartFiring();
	}
	else
	{
		UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, UDamageType::StaticClass());
		AmmoCount--;
		UpdateBlackboardValues();
	}
	return true;
}

float AEnemyMercenary::GetAimSpreadForDistance(float Distance) const
{
	const float Far = 90.f;

	switch (RoleType)
	{
	case EEnemyRole::Sniper:
		if (Distance <= CloseRangeThreshold)
		{
			return 8.f;
		}
		if (Distance <= MediumRangeThreshold)
		{
			return 1.5f;
		}
		return 2.f;

	case EEnemyRole::Rifle:
		if (Distance <= CloseRangeThreshold)
		{
			return 2.f;
		}
		if (Distance <= MediumRangeThreshold)
		{
			return 5.f;
		}
		return 9.f;

	case EEnemyRole::Shotgun:
		if (Distance <= CloseRangeThreshold)
		{
			return 1.5f;
		}
		if (Distance <= MediumRangeThreshold)
		{
			return 12.f;
		}
		return Far;

	default:
		if(Distance <= CloseRangeThreshold)
		{
			return 2.f;
		}
		if(Distance <= MediumRangeThreshold)
		{
			return 5.f;
		}
		return 12.f;
	}
}

void AEnemyMercenary::StopShooting()
{
	if(EquippedWeapon)
	{
		EquippedWeapon->StopFiring();
	}

	CurrentAimTarget = nullptr;
}

void AEnemyMercenary::InitializeByRole()
{
	switch(RoleType)
	{
	case EEnemyRole::Sniper:
		WalkSpeed = 250.f;
		RunMultiplier = 1.3f;
		TurnRate = 90.f;
		EffectiveRange = 5000.f;
		Damage = 0.80f;
		Precision = 0.8f;
		ReloadTime = 3.5f;
		AmmoCount = 5.f;
		break;
	case EEnemyRole::Rifle:
		WalkSpeed = 300.f;
		RunMultiplier = 1.7f;
		TurnRate = 120.f;
		EffectiveRange = 2000.f;
		Damage = 0.25f;
		Precision = 0.6f;
		ReloadTime = 2.f;
		AmmoCount = 30.f;
		break;
	case EEnemyRole::Shotgun:
		WalkSpeed = 450.f;
		RunMultiplier = 1.5f;
		TurnRate = 200.f;
		EffectiveRange = 300.f;
		Damage = 0.70f;
		Precision = 0.4f;
		ReloadTime = 2.5f;
		AmmoCount = 8.f;
		break;
	default:
		WalkSpeed = 300.f;
		RunMultiplier = 1.7f;
		TurnRate = 120.f;
		EffectiveRange = 2000.f;
		Damage = 0.25f;
		Precision = 0.6f;
		ReloadTime = 2.f;
		AmmoCount = 30.f;
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	switch (RoleType)
	{
		case EEnemyRole::Sniper:
			GetCharacterMovement()->RotationRate = FRotator(0.f, TurnRate, 0.f);
			break;
		case EEnemyRole::Rifle:
			GetCharacterMovement()->RotationRate = FRotator(0.f, TurnRate, 0.f);
			break;
		case EEnemyRole::Shotgun:
			GetCharacterMovement()->RotationRate = FRotator(0.f, TurnRate, 0.f);
			break;
		default:
			GetCharacterMovement()->RotationRate = FRotator(0.f, 120.f, 0.f);
			break;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if(AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsInt("RoleID", (int32)RoleType);
		AIController->GetBlackboardComponent()->SetValueAsFloat("ApproachRadius", EffectiveRange * 0.8f);
	}

	TSubclassOf<AShooterWeapon> SelectedWeaponClass = nullptr;
	switch (RoleType)
	{
		case EEnemyRole::Sniper:   SelectedWeaponClass = SniperWeaponClass;  break;
		case EEnemyRole::Rifle:    SelectedWeaponClass = RifleWeaponClass;   break;
		case EEnemyRole::Shotgun:  SelectedWeaponClass = ShotgunWeaponClass; break;
		default:                   SelectedWeaponClass = RifleWeaponClass;   break;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	if(SelectedWeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		EquippedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(SelectedWeaponClass, GetActorTransform(), SpawnParams);
		if (EquippedWeapon)
		{
			AttachWeaponMeshes(EquippedWeapon);

			switch (RoleType)
			{
			case EEnemyRole::Sniper:
				EquippedWeapon->SetRefireRate(2.5f);
				EquippedWeapon->SetFullAuto(false);
				EquippedWeapon->SetMagazineSize(5);
				break;
			case EEnemyRole::Rifle:
				EquippedWeapon->SetRefireRate(0.3f);
				EquippedWeapon->SetFullAuto(true);
				EquippedWeapon->SetMagazineSize(30);
				break;
			case EEnemyRole::Shotgun:
				EquippedWeapon->SetRefireRate(1.0f);
				EquippedWeapon->SetFullAuto(false);
				EquippedWeapon->SetMagazineSize(8);
				break;
			default:
				break;
			}
			UpdateBlackboardValues();
		}
	}
	UpdateBlackboardValues();
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
	Blackboard->SetValueAsFloat("AmmoCount", AmmoCount);


	AActor* TargetPlayer = Cast<AActor>(Blackboard->GetValueAsObject(FName("TargetActor")));

	bool bWasInRange = Blackboard->GetValueAsBool("IsInRange");

	bool bNewInRange = false;
	bool bNewHasSight = false;

	if (TargetPlayer)
	{
		float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		Blackboard->SetValueAsFloat("DistanceToPlayer", DistanceToPlayer);

		float RangeForState = (RoleType == EEnemyRole::Shotgun) ? EffectiveRange : MaxEngagementRange;
		float RangeThreshold = bWasInRange ? (RangeForState * 1.1f) : RangeForState;
		bNewInRange = DistanceToPlayer <= RangeThreshold;

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		bool bTraceHit = GetWorld()->LineTraceSingleByChannel(
			Hit,
			GetActorLocation() + FVector(0, 0, 60),
			TargetPlayer->GetActorLocation() + FVector(0, 0, 60),
			ECC_Visibility, Params);
		bNewHasSight = !bTraceHit || (Hit.GetActor() == TargetPlayer);

		if (bIsInCombat && !bIsSupressing)
		{
			AMercenaryAIController* MercAIC = Cast<AMercenaryAIController>(GetController());
			if (MercAIC && MercAIC->GetFocusActor() != TargetPlayer)
			{
				MercAIC->SetFocus(TargetPlayer, EAIFocusPriority::Gameplay);
			}
		}
	}
	else
	{
		Blackboard->SetValueAsFloat("DistanceToPlayer", 999999.f);
	}

	if (Blackboard->GetValueAsBool("IsInRange") != bNewInRange)
	{
		Blackboard->SetValueAsBool("IsInRange", bNewInRange);
	}
	if (Blackboard->GetValueAsBool("HasClearLineOfSight") != bNewHasSight)
	{
		Blackboard->SetValueAsBool("HasClearLineOfSight", bNewHasSight);
	}

	if (bIsInCombat)
	{
		AMercenaryAIController* MercAIController = Cast<AMercenaryAIController>(AIController);
		if (MercAIController && TargetPlayer)
		{
			if(MercAIController->GetFocusActor() != TargetPlayer)
			{
				MercAIController->SetFocus(TargetPlayer, EAIFocusPriority::Gameplay);
			}
		}
		else if (MercAIController && !TargetPlayer)
		{
			MercAIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
	UpdateChargeSpeed();
	UpdateUtilityInputs();
}

void AEnemyMercenary::UpdateUtilityInputs()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();

	UpdateReloadInput(Blackboard);
	UpdateCoverInput(Blackboard);
	UpdateAdaptativeProfile();
}

void AEnemyMercenary::UpdateReloadInput(UBlackboardComponent* Blackboard)
{
	const float MagSize = EquippedWeapon ? EquippedWeapon->GetMagazineSize() : 30.f;

	float CurrentAmmo = EquippedWeapon ? (float)EquippedWeapon->GetBulletCount() : AmmoCount;
	CurrentAmmo = FMath::Max(0.f, CurrentAmmo);

	const float AmmoPct = (MagSize > 0.f) ? FMath::Clamp(CurrentAmmo / MagSize, 0.f, 1.f) : 0.f;
	Blackboard->SetValueAsFloat("AmmoPct", AmmoPct);
}

void AEnemyMercenary::UpdateCoverInput(UBlackboardComponent* Blackboard)
{
	const float HealthPct = (MaxHealth > 0.f) ? FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f) : 0.f;
	Blackboard->SetValueAsFloat("HealthPctInput", HealthPct);

	const FVector MyLocation = GetActorLocation();

	AActor* TargetPlayer = Cast<AActor>(Blackboard->GetValueAsObject(FName("TargetActor")));
	const float DistanceToPlayer = TargetPlayer ? FVector::Dist(MyLocation, TargetPlayer->GetActorLocation()) : 999999.f;

	const FVector CoverLocation = Blackboard->GetValueAsVector("CoverLocation");
	const bool bHasCover = !CoverLocation.IsNearlyZero();
	const float DistanceToCover = bHasCover ? FVector::Dist(MyLocation, CoverLocation) : 999999.f;

	const bool bCoverWorth = bHasCover && (DistanceToCover < DistanceToPlayer * CoverDistanceAdvantage);
	Blackboard->SetValueAsBool("IsCoverWorth", bCoverWorth);
}

void AEnemyMercenary::UpdateAdaptativeProfile()
{
	if (!UtilityAI)
	{
		return;
	}

	UtilityAI->StressLevel = stressLevel;

	if(!MySquad)
	{
		return;
	}

	switch (MySquad->CurrentPlayerStrategy)
	{
	case EPlayerStrategy::Agressive:
		UtilityAI->SetAdaptativeProfile(EAdaptativeProfile::CounterAggresive);
		break;
	case EPlayerStrategy::Camping:
		UtilityAI->SetAdaptativeProfile(EAdaptativeProfile::CounterCamper);
		break;
	case EPlayerStrategy::Silent:
		UtilityAI->SetAdaptativeProfile(EAdaptativeProfile::CounterStealthy);
		break;
	default:
		UtilityAI->SetAdaptativeProfile(EAdaptativeProfile::None);
		break;
	}
}

void AEnemyMercenary::EvaluateUtilityScores()
{
	UpdateBlackboardValues();
}

void AEnemyMercenary::NotifySquadOfDeath()
{
	if(!MySquad)
	{
		return;
	}

	MySquad->RemoveMemeber(this);
}

float AEnemyMercenary::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	CurrentHealth -= ActualDamage;
	CurrentHealth = FMath::Clamp(CurrentHealth, 0.f, MaxHealth);
	UpdateBlackboardValues();

	if(CurrentHealth <= 0.f)
	{
		GetWorldTimerManager().ClearTimer(UtilityTimerHandle);
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
		Destroy();
	}

	return ActualDamage;
}

void AEnemyMercenary::SetFacingMode(bool bFacePlayer)
{
	AMercenaryAIController* AIController = Cast<AMercenaryAIController>(GetController());
	if(!AIController)
	{
		return;
	}

	if(bFacePlayer)
	{
		AActor* TargetPlayer = Cast<AActor>(AIController->GetBlackboardComponent()? AIController->GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")): nullptr);

		if(TargetPlayer)
		{
			AIController->SetFocus(TargetPlayer, EAIFocusPriority::Gameplay);
		}
	}
	else
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AEnemyMercenary::StartReloadWeapon()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	if(Blackboard->GetValueAsBool("IsReloading"))
	{
		return;
	}

	Blackboard->SetValueAsBool("IsReloading", true);

	float Duration = ReloadTime > 0.f ? ReloadTime : 2.0f;

	GetWorldTimerManager().SetTimer(
		ReloadTimerHandle,
		[this]()
		{
			if (EquippedWeapon)
			{
				EquippedWeapon->Reload();
			}

			switch (RoleType)
			{
			case EEnemyRole::Sniper:   AmmoCount = 5.f;  break;
			case EEnemyRole::Rifle:    AmmoCount = 30.f; break;
			case EEnemyRole::Shotgun:  AmmoCount = 8.f;  break;
			default:                   AmmoCount = 30.f; break;
			}
			UpdateBlackboardValues();

			AAIController* AIC = Cast<AAIController>(GetController());
			if (AIC && AIC->GetBlackboardComponent())
			{
				AIC->GetBlackboardComponent()->SetValueAsBool(FName("IsReloading"), false);
			}
		},
		Duration,
		false
	);
}

FVector AEnemyMercenary::CalculateSniperPosition(AActor* Target)
{
	if(!Target)
	{
		return GetActorLocation();
	}

	FVector MyLocation = GetActorLocation();
	FVector ToTarget = (Target->GetActorLocation() - MyLocation).GetSafeNormal();

	FVector IdealPosition = MyLocation - (ToTarget * 3000.f);
	IdealPosition.Z += 200.f;

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation NavResult;

	if(NavSys && NavSys->ProjectPointToNavigation(IdealPosition, NavResult, FVector(500.f, 500.f, 500.f)))
	{
		return NavResult.Location;
	}
	return MyLocation;
}

void AEnemyMercenary::PerformSuppressionFire(FVector TargetLocation)
{
	if(!EquippedWeapon)
	{
		return;
	}

	bIsSupressing = true;

	SuppressionTargetLocation = TargetLocation + FVector(
		FMath::RandRange(-250.f, 250.f),
		FMath::RandRange(-250.f, 250.f),
		FMath::RandRange(0.f, 80.f)
	);

	CurrentAimTarget = nullptr;

	AMercenaryAIController* AIController = Cast<AMercenaryAIController>(GetController());
	if (AIController)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		AIController->SetFocalPoint(SuppressionTargetLocation, EAIFocusPriority::Gameplay);
	}
	SetMovementState(false);
	EquippedWeapon->StartFiring();
}

void AEnemyMercenary::StopSuppressionFire()
{
	bIsSupressing = false;
	SuppressionTargetLocation = FVector::ZeroVector;
	CurrentAimTarget = nullptr;
	if (EquippedWeapon)
	{
		EquippedWeapon->StopFiring();
	}
	
	AMercenaryAIController* AIController = Cast<AMercenaryAIController>(GetController());
	if (AIController)
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	SetMovementState(true);
}

void AEnemyMercenary::AssignSupressorRole()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	AIController->GetBlackboardComponent()->SetValueAsBool("IsSuppressor", true);
	AIController->GetBlackboardComponent()->SetValueAsBool("IsFlanker", false);
}

void AEnemyMercenary::AssignFlankerRole()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}
	AIController->GetBlackboardComponent()->SetValueAsBool("IsSuppressor", false);
	AIController->GetBlackboardComponent()->SetValueAsBool("IsFlanker", true);

	SetMovementState(true);
}

void AEnemyMercenary::AssignDefenderRole()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}
	
	StopSuppressionFire();
	StopShooting();

	bIsDefending = true;
	bIsSupressing = false;

	UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
	Blackboard->SetValueAsBool("IsSuppressor", false);
	Blackboard->SetValueAsBool("IsFlanker", false);

	Blackboard->ClearValue("CoverLocation");

	Blackboard->SetValueAsBool("IsDefender", true);

	UE_LOG(LogTemp, Warning, TEXT("[Merc %s] DEFENDER ROLE ASSIGNED"), *GetName());

	AMercenaryAIController* MercAIController = Cast<AMercenaryAIController>(AIController);
	if (MercAIController)
	{
		MercAIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	SetMovementState(true);
}

void AEnemyMercenary::ClearCombatRole()
{
	StopSuppressionFire();
	StopShooting();

	bIsSupressing = false;
	bIsDefending = false;

	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	AIController->GetBlackboardComponent()->SetValueAsBool("IsSuppressor", false);
	AIController->GetBlackboardComponent()->SetValueAsBool("IsFlanker", false);
	AIController->GetBlackboardComponent()->SetValueAsBool("IsDefender", false);
}

FVector AEnemyMercenary::CalculateFlankPosition(AActor* ThreatActor)
{
	if (!ThreatActor)
	{
		return GetActorLocation();
	}

	FVector PlayerLocation = ThreatActor->GetActorLocation();
	FVector PlayerForward = ThreatActor->GetActorForwardVector();
	FVector PlayerRight = ThreatActor->GetActorRightVector();

	float Side = FMath::RandBool() ? 1.f : -1.f;

	FVector FlankDirection = (-PlayerForward + PlayerRight *Side).GetSafeNormal();
	FVector IdealFlankPosition = PlayerLocation + FlankDirection * FMath::RandRange(600.f, 1200.f);

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
	FNavLocation NavResult;

	if(NavSys && NavSys->ProjectPointToNavigation(IdealFlankPosition, NavResult, FVector(500.f, 500.f, 300.f)))
	{
		return NavResult.Location;
	}

	return PlayerLocation + FlankDirection * 800.f;
}

void AEnemyMercenary::UpdateChargeSpeed()
{
	if(!bIsInCombat || RoleType != EEnemyRole::Shotgun)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
		return;
	}

	AAIController* AIController = Cast<AAIController>(GetController());
	if(!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	bool bInRange = AIController->GetBlackboardComponent()->GetValueAsBool("IsInRange");
	bool bIsCharging = !bInRange && !bIsDefending && !bIsSupressing;

	if (bIsCharging)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * RunMultiplier;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void AEnemyMercenary::RefreshSuppressionFocalPoint(FVector NewTarget)
{
	if (!bIsSupressing)
	{
		return;
	}

	SuppressionTargetLocation = NewTarget + FVector(
		FMath::RandRange(-80.f, 80.f),
		FMath::RandRange(-80.f, 80.f),
		FMath::RandRange(0.f, 40.f)
	);

	AMercenaryAIController* AIController = Cast<AMercenaryAIController>(GetController());
	if (AIController)
	{
		AIController->SetFocalPoint(SuppressionTargetLocation, EAIFocusPriority::Gameplay);
	}
}


void AEnemyMercenary::CommunicateWithSquad()
{
	AAIController* AIController = Cast<AAIController>(GetController());
	if(AIController && AIController->GetBlackboardComponent() && MySquad)
	{
		AActor* TargetPlayer = Cast<AActor>(AIController->GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")));
		if (TargetPlayer)
		{
			MySquad->SharePlayerLocation(TargetPlayer);
		}
	}
}

void AEnemyMercenary::AttachWeaponMeshes(AShooterWeapon* WeaponToAttach)
{
	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, false);
	WeaponToAttach->AttachToActor(this, AttachmentRules);

	WeaponToAttach->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRules, ThirdPersonWeaponSocket);
}

FVector AEnemyMercenary::GetWeaponTargetLocation()
{
	if(bIsSupressing)
	{
		return SuppressionTargetLocation + FVector(
			FMath::RandRange(-100.f, 100.f),
			FMath::RandRange(-100.f, 100.f),
			FMath::RandRange(0.f, 50.f)
		);
	}

	if(!CurrentAimTarget)
	{
		return GetActorLocation() + GetActorForwardVector() * 1000.f;
	}

	FVector AimSource = GetMesh()->GetSocketLocation(FName("head"));
	FVector AimTarget = CurrentAimTarget->GetActorLocation();
	AimTarget.Z += FMath::RandRange(MinAimOffsetZ, MaxAimOffsetZ);

	float DistanceToTarget = FVector::Dist(GetActorLocation(), CurrentAimTarget->GetActorLocation());
	float SpreadHalfAngle = GetAimSpreadForDistance(DistanceToTarget);

	FVector AimDirection = (AimTarget - AimSource).GetSafeNormal();
	AimDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(AimDirection, SpreadHalfAngle);
	AimTarget = AimSource + (AimDirection * AimRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByChannel(HitResult, AimSource, AimTarget, ECC_Visibility, QueryParams);

	return HitResult.bBlockingHit ? HitResult.ImpactPoint : HitResult.TraceEnd;
}

void AEnemyMercenary::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	float PrevAmmo = AmmoCount;
	AmmoCount = static_cast<float>(CurrentAmmo);

	if (AmmoCount <= 0.f && PrevAmmo > 0.f)
	{
		AAIController* AIC = Cast<AAIController>(GetController());
		if (AIC && AIC->GetBlackboardComponent())
		{
			AIC->GetBlackboardComponent()->SetValueAsFloat("AmmoCount", AmmoCount);
		}
	}
	else if (AmmoCount > 0.f)
	{
		AAIController* AIC = Cast<AAIController>(GetController());
		if (AIC && AIC->GetBlackboardComponent())
		{
			AIC->GetBlackboardComponent()->SetValueAsFloat("AmmoCount", AmmoCount);
		}
	}
}

void AEnemyMercenary::OnSemiWeaponRefire()
{
	if (CurrentAimTarget && EquippedWeapon)
	{
		EquippedWeapon->StartFiring();
	}
}