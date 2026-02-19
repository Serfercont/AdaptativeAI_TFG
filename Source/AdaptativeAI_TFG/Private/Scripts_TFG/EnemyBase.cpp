// Rights reserved by Sergio Fernandez


#include "Scripts_TFG/EnemyBase.h"
#include "AI/BaseAIController.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AEnemyBase::AEnemyBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ABaseAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	WalkSpeed = 300.f;
	RunMultiplier = 1.5f;
	TurnRate = 360.f;

	//Walking speed Initialization
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	//Rotation Initialization
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, TurnRate, 0.f);
}

// Called when the game starts or when spawned
void AEnemyBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AEnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnemyBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

// Function to set the movement state of the enemy (walking or running)
void AEnemyBase::SetMovementState(bool bIsChasing)
{
	if (bIsChasing)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * RunMultiplier;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

