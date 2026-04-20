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

AEnemyMercenary::AEnemyMercenary()
{
	AIControllerClass = AMercenaryAIController::StaticClass();
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

	if(WeaponClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		EquippedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);
		
	}
	FTimerHandle InitHandle;
	GetWorldTimerManager().SetTimer(InitHandle, this, &AEnemyMercenary::InitializeByRole, 0.2f, false);

	GetWorldTimerManager().SetTimer(UtilityTimerHandle, this, &AEnemyMercenary::EvaluateUtilityScores, 0.5f, true);
}

bool AEnemyMercenary::RequestAttackSlot()
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
}

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
		Damage = 80.f;
		Precision = 0.8f;
		ReloadTime = 3.5f;
		AmmoCount = 5.f;
		break;
	case EEnemyRole::Rifle:
		WalkSpeed = 300.f;
		RunMultiplier = 1.7f;
		TurnRate = 120.f;
		EffectiveRange = 2000.f;
		Damage = 25.f;
		Precision = 0.6f;
		ReloadTime = 2.f;
		AmmoCount = 30.f;
		break;
	case EEnemyRole::Shotgun:
		WalkSpeed = 350.f;
		RunMultiplier = 1.5f;
		TurnRate = 160.f;
		EffectiveRange = 600.f;
		Damage = 50.f;
		Precision = 0.4f;
		ReloadTime = 2.5f;
		AmmoCount = 8.f;
		break;
	default:
		WalkSpeed = 300.f;
		RunMultiplier = 1.7f;
		TurnRate = 120.f;
		EffectiveRange = 2000.f;
		Damage = 25.f;
		Precision = 0.6f;
		ReloadTime = 2.f;
		AmmoCount = 30.f;
		break;
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
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
	if(!CurrentAimTarget)
	{
		return GetActorLocation() + GetActorForwardVector() * 1000.f;
	}

	FVector AimSource = GetMesh()->GetSocketLocation(FName("head"));
	FVector AimTarget = CurrentAimTarget->GetActorLocation();
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
}

void AEnemyMercenary::OnSemiWeaponRefire()
{
	if (CurrentAimTarget && EquippedWeapon)
	{
		EquippedWeapon->StartFiring();
	}
}
