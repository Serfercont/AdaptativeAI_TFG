// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/EnemySquad.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/Engine.h"
#include <AI/MercenaryAIController.h>

AEnemySquad::AEnemySquad()
{
	PrimaryActorTick.bCanEverTick = false;
	CurrentPlayerStrategy = EPlayerStrategy::None;

	CampingMovementThreshold = 200;
	CampingDetectionTime = 5.0f;
}

void AEnemySquad::BeginPlay()
{
	Super::BeginPlay();
}

bool AEnemySquad::ResolvePlayerTarget(FVector& OutPlayerPosition, AActor*& OutTargetPlayer)
{
	OutPlayerPosition = FVector::ZeroVector;
	OutTargetPlayer = nullptr;
	
	FVector FallbackLocation = FVector::ZeroVector;
	bool bHasFallbackLocation = false;

	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (!Member) continue;
		AAIController* AIC = Cast<AAIController>(Member->GetController());
		if (!AIC || !AIC->GetBlackboardComponent()) continue;

		UBlackboardComponent* BB = AIC->GetBlackboardComponent();
		
		AActor* Candidate = Cast<AActor>(BB->GetValueAsObject(FName("TargetActor")));

		if (Candidate)
		{
			OutTargetPlayer = Candidate;
			OutPlayerPosition = Candidate->GetActorLocation();
			return true;
		}
		if (!bHasFallbackLocation)
		{
			FVector LastKnown = BB->GetValueAsVector(FName("LastKnownLocation"));
			if(!LastKnown.IsZero())
			{
				FallbackLocation = LastKnown;
				bHasFallbackLocation = true;
			}
		}
	}

	if(bHasFallbackLocation)
	{
		OutPlayerPosition = FallbackLocation;
		return true;
	}

	return false;
}

EPlayerStrategy AEnemySquad::ClassifyPlayer(const FVector& CurrentPlayerPosition)
{
	float Now = GetWorld()->GetTimeSeconds();

	if(LastKillTime > 0.f && (Now - LastKillTime) > KillWindowDuration)
	{
		RecentKills = 0;
	}

	if(RecentKills >= agressiveKillThreshold)
	{
		return EPlayerStrategy::Agressive;
	}

	if(bDefensiveActive)
	{
		return EPlayerStrategy::Agressive;
	}

	float DistanceMoved = FVector::Dist(CurrentPlayerPosition, LastRecordedPlayerPosition);

	if (DistanceMoved > CampingMovementThreshold)
	{
		LastRecordedPlayerPosition = CurrentPlayerPosition;

		if (!bFlankActivated)
		{
			PlayerStationaryStartTime = GetWorld()->GetTimeSeconds();
			return EPlayerStrategy::None;
		}
		return EPlayerStrategy::Camping;
	}
	if (bFlankActivated)
	{
		return EPlayerStrategy::Camping;
	}
	float TimeStationary = GetWorld()->GetTimeSeconds() - PlayerStationaryStartTime;

	if (TimeStationary >= CampingDetectionTime)
	{
		return EPlayerStrategy::Camping;
	}

	return EPlayerStrategy::None;
}

void AEnemySquad::EnterStrategy(EPlayerStrategy NewStrategy)
{
	switch (NewStrategy)
	{
	case EPlayerStrategy::Camping:
		UE_LOG(LogTemp, Warning, TEXT("[Squad] JUGADOR CAMPEANDO. INICIANDO FLANQUEO."));
		if (!bFlankActivated)
		{
			CoordinateFlankAndSupress();
		}
		break;
	case EPlayerStrategy::Agressive:
		UE_LOG(LogTemp, Warning, TEXT("[Squad] JUGADOR AGRESIVO. COORDINANDO RETIRADA DEFENSIVA."));
		if (!bDefensiveActive)
		{
			CoordinateDefensiveRetreat();
		}
		break;

	default:
		break;
	}
}

void AEnemySquad::TickStrategy(EPlayerStrategy Strategy, const FVector& CurrentPlayerPosition, AActor* TargetPlayer)
{
	switch (Strategy)
	{
		case EPlayerStrategy::Camping:
		{
			if (!bFlankActivated)
			{
				break;
			}

			float DistFromFlankOrigin = FVector::Dist(CurrentPlayerPosition, FlankInitiatedAtPosition);
			if (DistFromFlankOrigin > 1500.f)
			{
				bool bAnyMemberReloading = false;
				for(AEnemyMercenary* Member : SquadMembers)
				{
					if (!Member) continue;
					AAIController* AIC = Cast<AAIController>(Member->GetController());
					if (AIC && AIC->GetBlackboardComponent() &&	AIC->GetBlackboardComponent()->GetValueAsBool(FName("IsReloading")))
					{
						bAnyMemberReloading = true;
						break;
					}
				}
				if(!bAnyMemberReloading)
				{
					ExitStrategy(EPlayerStrategy::Camping);
					CurrentPlayerStrategy = EPlayerStrategy::None;
					PlayerStationaryStartTime = GetWorld()->GetTimeSeconds();
					LastRecordedPlayerPosition = CurrentPlayerPosition;
				}
			}
			else
			{
				UpdateSuppressionTargets(CurrentPlayerPosition);
			}
			break;
		}

		default:
			break;

		case EPlayerStrategy::Agressive:
		{
			if (!bDefensiveActive)
			{
				break;
			}

			float Now = GetWorld()->GetTimeSeconds();
			bool bKillsExpired = (LastKillTime < 0.f) || ((Now - LastKillTime) > KillWindowDuration);
			float DistFromOrigin = FVector::Dist(CurrentPlayerPosition, DefenseInitPosition);

			if (bKillsExpired && DistFromOrigin > 2000.f)
			{
				ExitStrategy(EPlayerStrategy::Agressive);
				CurrentPlayerStrategy = EPlayerStrategy::None;
				PlayerStationaryStartTime = GetWorld()->GetTimeSeconds();
				LastRecordedPlayerPosition = CurrentPlayerPosition;
				RecentKills = 0;
			}
			break;
		}
	}
}

void AEnemySquad::ExitStrategy(EPlayerStrategy OldStrategy)
{
	switch (OldStrategy)
	{
	case EPlayerStrategy::Camping:
		if (bFlankActivated)
		{
			CancelFlanking();
		}
		break;
	case EPlayerStrategy::Agressive:
		if (bDefensiveActive)
		{
			CancelDefensiveRetreat();
		}
		break;

	default:
		break;
	}
}

void AEnemySquad::CoordinateDefensiveRetreat()
{
	TArray<AEnemyMercenary*> ActiveMembers;
	for(AEnemyMercenary* Member : SquadMembers)
	{
		if(Member && Member->CurrentHealth > 0 && Member->RoleType != EEnemyRole::Sniper)
		{
			AAIController* AIC = Cast<AAIController>(Member->GetController());
			if(AIC && AIC->GetBlackboardComponent() && AIC->GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")))
			{
				ActiveMembers.Add(Member);
			}
		}
	}

	if(ActiveMembers.Num() == 0)
	{
		return;
	}

	bDefensiveActive = true;

	for(AEnemyMercenary* Member : ActiveMembers)
	{
		Member->ClearCombatRole();
		Member->AssignDefenderRole();

		AAIController* AIController = Cast<AAIController>(Member->GetController());
		if(AIController && AIController->GetBlackboardComponent())
		{
			bool bCheck = AIController->GetBlackboardComponent()->GetValueAsBool(FName("IsDefender"));

			UE_LOG(LogTemp, Warning, TEXT("[Squad] %s es defensor: %s"), *Member->GetName(), bCheck ? TEXT("true") : TEXT("false"));
		}
	}

	DefenseInitPosition = LastRecordedPlayerPosition;

	if(GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
			FString::Printf(TEXT("Escuadron: RETIRADA DEFENSIVA COORDINADA! %d mercenarios se reagrupan"), ActiveMembers.Num()));
	}
}

void AEnemySquad::CancelDefensiveRetreat()
{
	bDefensiveActive = false;
	for(AEnemyMercenary* Member : SquadMembers)
	{
		if(Member)
		{
			Member->ClearCombatRole();
		}
	}
}

void AEnemySquad::InitializeSquad(const TArray<AEnemyMercenary*>& Members)
{
	SquadMembers = Members;

	for(AEnemyMercenary* Member : SquadMembers)
	{
		Member->MySquad = this;
	}

	GetWorldTimerManager().SetTimer(SquadBrainTimerHandle, this, &AEnemySquad::EvaluateSquadStrategy, 0.5f, true);
}


void AEnemySquad::EvaluateSquadStrategy()
{
	FVector CurrentPlayerPosition = FVector::ZeroVector;
	AActor* TargetPlayer = nullptr;
	bool bAnyMemberHasTarget = ResolvePlayerTarget(CurrentPlayerPosition, TargetPlayer);

	if (!bAnyMemberHasTarget)
	{
		if (CurrentPlayerStrategy != EPlayerStrategy::None)
		{
			ExitStrategy(CurrentPlayerStrategy);
			CurrentPlayerStrategy = EPlayerStrategy::None;
		}
		PlayerStationaryStartTime = -1.f;
		LastRecordedPlayerPosition = FVector::ZeroVector;
		return;
	}

	if (PlayerStationaryStartTime < 0.f)
	{
		PlayerStationaryStartTime = GetWorld()->GetTimeSeconds();
		LastRecordedPlayerPosition = CurrentPlayerPosition;
		return;
	}

	EPlayerStrategy DetectedStrategy = ClassifyPlayer(CurrentPlayerPosition);

	if (DetectedStrategy != CurrentPlayerStrategy)
	{
		ExitStrategy(CurrentPlayerStrategy);
		EnterStrategy(DetectedStrategy);
		CurrentPlayerStrategy = DetectedStrategy;
	}

	TickStrategy(CurrentPlayerStrategy, CurrentPlayerPosition, TargetPlayer);
	
}

void AEnemySquad::IssueOrderToMember(AEnemyMercenary* Member, ESquadOrder NewOrder)
{
	AAIController* AIController = Cast<AAIController>(Member->GetController());
	if (AIController && AIController->GetBlackboardComponent())
	{
		AIController->GetBlackboardComponent()->SetValueAsEnum(FName("SquadOrder"), static_cast<uint8>(NewOrder));
	}
}

void AEnemySquad::SharePlayerLocation(AActor* TargetPlayer)
{
	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (Member)
		{
			AAIController* AIController = Cast<AAIController>(Member->GetController());
			if (AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->SetValueAsVector(FName("LastKnownLocation"), TargetPlayer->GetActorLocation());
				AIController->GetBlackboardComponent()->SetValueAsBool(FName("IsPlayerVisible"), true);
				AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), TargetPlayer);
			}
		}
	}
}

void AEnemySquad::RemoveMemeber(AEnemyMercenary* MemberToRemove)
{
	if(!MemberToRemove)
	{
		return;
	}

	float Now = GetWorld()->GetTimeSeconds();
	if(LastKillTime > 0.f && (Now - LastKillTime) > KillWindowDuration)
	{
		RecentKills = 0;
	}

	RecentKills++;
	LastKillTime = Now;

	SquadMembers.Remove(MemberToRemove);

	if(SquadMembers.Num() == 0)
	{
		GetWorldTimerManager().ClearTimer(SquadBrainTimerHandle);
		Destroy();
	}
}

void AEnemySquad::AlertAllMembers(AActor* TargetPlayer)
{
	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (Member)
		{
			AAIController* AIController = Cast<AAIController>(Member->GetController());
			if (AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->SetValueAsObject(FName("TargetActor"), TargetPlayer);
				AIController->GetBlackboardComponent()->SetValueAsBool(FName("IsPlayerVisible"), true);
				AIController->GetBlackboardComponent()->SetValueAsVector(FName("LastKnownLocation"), TargetPlayer->GetActorLocation());
			}
			Member->EnterCombat();
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red,
			FString::Printf(TEXT("Escuadron: ALERTA TOTAL! %d mercenarios atacan"), SquadMembers.Num()));
	}
}

void AEnemySquad::CoordinateFlankAndSupress()
{

	TArray<AEnemyMercenary*> ActiveMembers;
	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (Member && Member->CurrentHealth > 0 && Member->RoleType != EEnemyRole::Sniper)
		{
			AAIController* AIC = Cast<AAIController>(Member->GetController());
			if (AIC && AIC->GetBlackboardComponent() && AIC->GetBlackboardComponent()->GetValueAsObject(FName("TargetActor")))
			{
				ActiveMembers.Add(Member);
			}
		}
	}

	int32 TotalActive = ActiveMembers.Num();
	if (TotalActive == 0)
	{
		return;
	}

	bFlankActivated = true;

	int32 NumFlankers = FMath::Max(1, FMath::FloorToInt(TotalActive * 0.35f));
	int32 NumSuppressors = TotalActive - NumFlankers;

	ActiveMembers.Sort([](const AEnemyMercenary& A, const AEnemyMercenary& B)
	{
		return (int32)A.RoleType < (int32)B.RoleType;
	});

	FVector AimTarget = LastRecordedPlayerPosition + FVector(0, 0, 50);

	for (int32 i = 0; i < ActiveMembers.Num(); i++)
	{
		if (i < NumFlankers && ActiveMembers[i]->RoleType != EEnemyRole::Sniper)
		{
			ActiveMembers[i]->AssignFlankerRole();
		}
		else
		{
			ActiveMembers[i]->AssignSupressorRole();

			AAIController* AIController = Cast<AAIController>(ActiveMembers[i]->GetController());
			if (AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->SetValueAsVector(FName("SuppressionTarget"), AimTarget);
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange,
			FString::Printf(TEXT("FLANQUEO: %d supresores, %d flanqueadores"), NumSuppressors, NumFlankers));
	}

	FlankInitiatedAtPosition = LastRecordedPlayerPosition;
}

void AEnemySquad::CancelFlanking()
{
	bFlankActivated = false;
	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (Member)
		{
			Member->ClearCombatRole();
		}
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green,
			TEXT("FLANQUEO CANCELADO: Todos los miembros vuelven a ser supresores"));
	}
}

void AEnemySquad::UpdateSuppressionTargets(FVector TargetLoc)
{
	for(AEnemyMercenary* Member : SquadMembers)
	{
		if (Member && Member->bIsSupressing)
		{
			Member->SuppressionTargetLocation = TargetLoc + FVector(FMath::RandRange(-150.f, 150.f), FMath::RandRange(-150.f, 150.f), FMath::RandRange(0.f, 60.f));
			AAIController* AIController = Cast<AAIController>(Member->GetController());
			if(AIController && AIController->GetBlackboardComponent())
			{
				AIController->GetBlackboardComponent()->SetValueAsVector(FName("SuppressionTarget"), Member->SuppressionTargetLocation);

				AMercenaryAIController* ControllerAI = Cast<AMercenaryAIController>(AIController);
				if(ControllerAI)
				{
					ControllerAI->SetFocalPoint(Member->SuppressionTargetLocation, EAIFocusPriority::Gameplay);
				}
			}
		}
	}
}
