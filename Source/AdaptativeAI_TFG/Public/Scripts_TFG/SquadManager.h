// Rights reserved by Sergio Fernandez

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SquadManager.generated.h"

class AEnemyMercenary;

UCLASS()
class ADAPTATIVEAI_TFG_API ASquadManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ASquadManager();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Squad | Organization")
	void OrganizeSquads();

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad | Organization")
	TArray<AEnemyMercenary*> UnassignedMercenaries;
	FTimerHandle OrganizeTimerHandle;

	void RegisterSquadMember(AEnemyMercenary* NewMember);
};
