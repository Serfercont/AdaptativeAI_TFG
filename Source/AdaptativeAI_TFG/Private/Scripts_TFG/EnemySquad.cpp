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
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.5f, FColor::Cyan, TEXT("Cerebro de Escuadron evaluando tacticas..."));
	}

	FVector CurrentPlayerPosition = FVector::ZeroVector;
	bool bAnyMemberHasTarget = false;
	AActor* TargetPlayer = nullptr;

	for (AEnemyMercenary* Member : SquadMembers)
	{
		if (!Member) continue;
		AAIController* AIC = Cast<AAIController>(Member->GetController());
		if (!AIC || !AIC->GetBlackboardComponent()) continue;

		UBlackboardComponent* BB = AIC->GetBlackboardComponent();
		TargetPlayer = Cast<AActor>(BB->GetValueAsObject(FName("TargetActor")));

		if (TargetPlayer)
		{
			bAnyMemberHasTarget = true;
			CurrentPlayerPosition = TargetPlayer->GetActorLocation();
			break;
		}

		FVector LastKnown = BB->GetValueAsVector(FName("LastKnownLocation"));
		if (!LastKnown.IsZero())
		{
			bAnyMemberHasTarget = true;
			CurrentPlayerPosition = LastKnown;
		}
	}

	if (!bAnyMemberHasTarget)
	{
		PlayerStationaryStartTime = -1.f;
		LastRecordedPlayerPosition = FVector::ZeroVector;
		if (bFlankActivated) CancelFlanking();
		return;
	}

	if (PlayerStationaryStartTime < 0.f)
	{
		PlayerStationaryStartTime = GetWorld()->GetTimeSeconds();
		LastRecordedPlayerPosition = CurrentPlayerPosition;
		return;
	}

	float DistanceMoved = FVector::Dist(CurrentPlayerPosition, LastRecordedPlayerPosition);

	if (DistanceMoved > CampingMovementThreshold)
	{
		LastRecordedPlayerPosition = CurrentPlayerPosition;

		if (!bFlankActivated)
		{
			PlayerStationaryStartTime = GetWorld()->GetTimeSeconds();
		}
		return;
	}

	if (bFlankActivated)
	{
		float DistFromFlankOrigin = FVector::Dist(CurrentPlayerPosition, FlankInitiatedAtPosition);
		if (DistFromFlankOrigin > 1500.f)
		{
			bool bAnyMemberReloading = false;
			for(AEnemyMercenary* Member : SquadMembers)
			{
				if (!Member) continue;
				AAIController* AIC = Cast<AAIController>(Member->GetController());
				if (AIC && AIC->GetBlackboardComponent())
				{
					if (AIC->GetBlackboardComponent()->GetValueAsBool(FName("IsReloading")))
					{
						bAnyMemberReloading = true;
						break;
					}
				}
			}
			if(!bAnyMemberReloading)
			{
				CancelFlanking();
			}
		}
		else
		{
			UpdateSuppressionTargets(CurrentPlayerPosition);
		}
		return;
	}
	

	float TimeStationary = GetWorld()->GetTimeSeconds() - PlayerStationaryStartTime;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow,
			FString::Printf(TEXT("[Squad] Jugador quieto %.1fs / %.1fs para flanqueo"),
				TimeStationary, CampingDetectionTime));
	}

	if (!bFlankActivated && TimeStationary >= CampingDetectionTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Squad] JUGADOR CAMPEANDO %.1fs. INICIANDO FLANQUEO."),TimeStationary);
		CoordinateFlankAndSupress();
	}
	else if (bFlankActivated && TargetPlayer)
	{
		UpdateSuppressionTargets(CurrentPlayerPosition);
	}
	
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

/*bool AEnemySquad::RequestAttackSlot(AEnemyMercenary* RequestingMember)
{
	if(CurrentAttackers.Contains(RequestingMember))
	{
		return true;
	}
	if(CurrentAttackers.Num() >= MaxAttackSlots)
	{
		return false;
	}
	CurrentAttackers.Add(RequestingMember);
	return true;
}

void AEnemySquad::ReleaseAttackSlot(AEnemyMercenary* ReleasingMember)
{
	if(CurrentAttackers.Contains(ReleasingMember))
	{
		CurrentAttackers.Remove(ReleasingMember);
	}
}*/

void AEnemySquad::RemoveMemeber(AEnemyMercenary* MemberToRemove)
{
	if(!MemberToRemove)
	{
		return;
	}

	SquadMembers.Remove(MemberToRemove);

	//ReleaseAttackSlot(MemberToRemove);

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
		if (Member && Member->CurrentHealth > 0)
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
		if (i < NumFlankers)
		{
			ActiveMembers[i]->AssignFlankerRole();
		}
		else
		{
			ActiveMembers[i]->AssignSupressorRole();
			//ActiveMembers[i]->PerformSuppressionFire(LastRecordedPlayerPosition);

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
