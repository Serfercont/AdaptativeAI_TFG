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
	SetFacingMode(true);

	AAIController* AIController = Cast<AAIController>(GetController());
	bool bIsSuppressor = false;
	if(AIController && AIController->GetBlackboardComponent())
	{
		bIsSuppressor = AIController->GetBlackboardComponent()->GetValueAsBool("IsSuppressor");
	}

	if (!bIsSuppressor)
	{
		SetMovementState(true);
	}
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

/*bool AEnemyMercenary::RequestAttackSlot()
{
	if(!MySquad)
	{
		return false;
	}

	bool bHasSlot = MySquad->RequestAttackSlot(this);

	AAIController* AIController = Cast<AAIController>(GetController());
	if(AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsBool("HasAttackSlot", bHasSlot);
	}

	return bHasSlot;
}

void AEnemyMercenary::ReleaseAttackSlot()
{
	if (!MySquad)
	{
		return;
	}

	MySquad->ReleaseAttackSlot(this);
	AAIController* AIController = Cast<AAIController>(GetController());
	if (AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsBool("HasAttackSlot", false);
	}
}*/

bool AEnemyMercenary::PerformShoot(AActor* Target)
{
	if(!Target)
	{
		return false;
	}

	float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if(DistanceToTarget > EffectiveRange)
	{
		return false;
	}

	if (EquippedWeapon)
	{
		CurrentAimTarget = Target;
		EquippedWeapon->StartFiring();
		return true;
	}

	if(AmmoCount <= 0.f)
	{
		return false;
	}

	UGameplayStatics::ApplyDamage(Target, Damage, GetController(), this, UDamageType::StaticClass());
	
	AmmoCount--;
	UpdateBlackboardValues();
	return true;
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
		WalkSpeed = 350.f;
		RunMultiplier = 1.5f;
		TurnRate = 160.f;
		EffectiveRange = 800.f;
		Damage = 0.50f;
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
			UpdateBlackboardValues();
		}
	}
	UpdateBlackboardValues();
}

void AEnemyMercenary::FindCoverPosition(AActor* ThreatActor)
{
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

	Blackboard->SetValueAsBool("IsInRange", false);
	Blackboard->SetValueAsBool("HasClearLineOfSight", false);
	if(TargetPlayer)
	{
		float DistanceToPlayer = FVector::Dist(GetActorLocation(), TargetPlayer->GetActorLocation());
		Blackboard->SetValueAsFloat("DistanceToPlayer", DistanceToPlayer);

		bool bCurrentlyInRange = Blackboard->GetValueAsBool("IsInRange");
		bool bNewInRange;
		if (bCurrentlyInRange)
		{
			bNewInRange = DistanceToPlayer <= (EffectiveRange * 1.1f);
		}
		else
		{
			bNewInRange = DistanceToPlayer <= EffectiveRange;
		}
		Blackboard->SetValueAsBool("IsInRange", bNewInRange);

		FHitResult Hit;
		FVector StartLoc = GetActorLocation() + FVector(0.f, 0.f, 60.f);
		FVector EndLoc = TargetPlayer->GetActorLocation() + FVector(0.f, 0.f, 60.f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		QueryParams.AddIgnoredActor(TargetPlayer);

		bool bHitCover = GetWorld()->LineTraceSingleByChannel(Hit, StartLoc, EndLoc, ECC_Visibility, QueryParams);

		Blackboard->SetValueAsBool("HasClearLineOfSight", !bHitCover);
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

	Blackboard->SetValueAsFloat("ApproachRadius", EffectiveRange * 0.8f);

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

	SetMovementState(false);
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

void AEnemyMercenary::ClearCombatRole()
{
	StopSuppressionFire();
	StopShooting();

	bIsSupressing = false;
	AAIController* AIController = Cast<AAIController>(GetController());
	if (!AIController || !AIController->GetBlackboardComponent())
	{
		return;
	}

	AIController->GetBlackboardComponent()->SetValueAsBool("IsSuppressor", false);
	AIController->GetBlackboardComponent()->SetValueAsBool("IsFlanker", false);
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
		AActor* TargetPlayer = Cast<AActor>(AIController->GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")));
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
	FVector AimTarget = CurrentAimTarget->GetActorLocation()+ FVector(0.f,0.f,0.f);
	AimTarget.Z += FMath::RandRange(MinAimOffsetZ, MaxAimOffsetZ);

	FVector AimDirection = (AimTarget - AimSource).GetSafeNormal();
	AimDirection = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(AimDirection, AimVarianceHalfAngle);
	AimTarget = AimSource + (AimDirection * AimRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByChannel(HitResult, AimSource, AimTarget, ECC_Visibility, QueryParams);

	return HitResult.bBlockingHit ? HitResult.ImpactPoint : HitResult.TraceEnd;
}

void AEnemyMercenary::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	AmmoCount = static_cast<float>(CurrentAmmo);
	UpdateBlackboardValues();
	UE_LOG(LogTemp, Warning, TEXT("[Ammo] %s: Ammo=%d / %d"), *GetName(), CurrentAmmo, MagazineSize);
}

void AEnemyMercenary::OnSemiWeaponRefire()
{
	if (CurrentAimTarget && EquippedWeapon)
	{
		EquippedWeapon->StartFiring();
	}
}
