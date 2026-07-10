// Fill out your copyright notice in the Description page of Project Settings.


#include "Action.h"

#include "CoreConfig.h"
#include "CoreManagerSubSystem.h"
#include "MyMaruCharacter.h"
#include "MyMaruInventoryComponent.h"
#include "MyMaruSaveGame.h"
#include "Kismet/GameplayStatics.h"

static TObjectPtr<APawn> s_PlayerPawn;
static TObjectPtr<UMyMaruSaveGame> s_SaveGame;

void ActionInitialize(TObjectPtr<UMyMaruSaveGame> SaveGame, TObjectPtr<APawn> PlayerPawn)
{
	s_SaveGame = SaveGame;
	s_PlayerPawn = PlayerPawn;
}

static int64 s_NextID = 0;

UFUNCTION(BlueprintCallable, Category = "Action")
void UAction::DebugDuplicateEvent( UAction *pAction, const int prevDays /* = 1*/ )
{
	//pAction->Duplicate();
	//TObjectPtr<UMyMaruSaveGame> SaveGame = Cast<UMyMaruSaveGame>(UGameplayStatics::GetGameInstance(pAction->GetWorld()));

}

UAction::UAction()
{
	// MH Do this in the separate function
	//// if (s_SaveGame)
	////	s_SaveGame->RegisterAction(this);

	ID = ++s_NextID;

	// MH Bad date, but better to have something here than nothing for now
	Timestamp = FDateTime::Now();
}

void UAction::RegisterDate( FDateTime InDateTime )
{

	TObjectPtr<UCoreManagerSubSystem> Core = s_PlayerPawn->GetWorld()->GetSubsystem<UCoreManagerSubSystem>();	

	if( Core->DEBUG_CreateAtDate )
	{
		UE_LOG(LogMyMaruSaveGame, Display, TEXT("*** FORCING DATETIME *** %s"), *InDateTime.ToString());
		InDateTime = Core->CurrentDateTime;
	}

	Timestamp = InDateTime;

	if (s_SaveGame)
	{
		s_SaveGame->RegisterAction( this, InDateTime);
		s_SaveGame->OnRegisterAction.Broadcast( this );
	}
}

FString UAction::GetDetails() const
{
	return TEXT( "Not implemented" );
}

void UAction::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);

	Ar << Timestamp;
	Ar << Type;
}

void UAction::Print()
{
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("Action: %s"), *GetName())

	FString TimestampString = FString::Printf(TEXT("Timestamp: %02d:%02d:%02d"), Timestamp.GetHour(), Timestamp.GetMinute(), Timestamp.GetSecond());
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  Timestamp: %s"), *TimestampString);
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  Type: %s"), *UEnum::GetValueAsString(Type));
}

void UAction_SpawnGrowable::Print()
{
	Super::Print();

	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  GrowableKey: %s"), *GrowableKey.ToString());
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  Location: %s"), *Location.ToString());
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  RandRotation: %f"), RandRotation);
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  FinalScale: %f"), FinalScale);
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  GrowthTime: %f"), GrowthTime);
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  DelayScale: %f"), DelayScale);
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  Layer: %d"), Layer);
}

void UAction_SpawnGrowable::Execute(bool bRestoreMode)
{
	Super::Execute();

	UE_LOG( Growable, Display, TEXT("SpawnGrowableAction Execute %s"), *GetName());


	// Fetch growable config.
	TObjectPtr<UCoreManagerSubSystem> CoreManagerSubSystem = s_PlayerPawn->GetWorld()->GetSubsystem<UCoreManagerSubSystem>();
	TObjectPtr<const FGrowableConfig> GrowableConfig = CoreManagerSubSystem->GetConfig()->FindGrowableConfig(GrowableKey);

	FTransform 	Transform(Location);
	TObjectPtr<AGrowable> NewGrowable = s_PlayerPawn->GetWorld()->SpawnActorDeferred<AGrowable>(GrowableConfig->GrowableClass, Transform);
	check(NewGrowable);

	bSkipGrowth = bRestoreMode;
	NewGrowable->Initialize(this, GrowableConfig->StaticMesh);
	
	// Initialize.
	Transform.SetRotation(FQuat(FVector::UpVector, RandRotation));
	NewGrowable->FinishSpawning(Transform);
	
	CoreManagerSubSystem->RegisterGrowable(NewGrowable, Layer);
}

void UAction_SpawnGrowable::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << GrowableKey;
	Ar << Location;
	Ar << RandRotation;
	Ar << FinalScale;
	Ar << GrowthTime;
	Ar << DelayScale;
	Ar << Layer;
	Ar << HealthDataType;
	Ar << GrowableRadius;
}

/*
 * Transact action. NOTE: this should never be a concrete class; it's just used to provide common behavior for Spend and
 * Grant.
 */

void UAction_Transact::Print()
{
	Super::Print();

	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  Type: %s"), *UEnum::GetValueAsString(HealthDataType));
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("  Amount: %d"), Amount)
}

TObjectPtr<UMyMaruInventoryComponent> UAction_Transact::GetInventory()
{
	TObjectPtr<AMyMaruCharacter> MyMaruCharacter = Cast<AMyMaruCharacter>(s_PlayerPawn);
	check(MyMaruCharacter);
	TObjectPtr<UMyMaruInventoryComponent> Inventory = MyMaruCharacter->Inventory;
	check(Inventory);
	return Inventory;
}

void UAction_Transact::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << HealthDataType;
	Ar << Amount;
}

/*
 * Spend action.
 */

void UAction_Spend::Execute(bool bRestoreMode)
{
	Super::Execute();
	// need to be saved for the undo process
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("Spend %d %s"), Amount, *UEnum::GetValueAsString(HealthDataType));
	GetInventory()->Spend(HealthDataType, Amount);
	
}

/*
 * Grant action
 */

void UAction_Grant::Execute(bool bRestoreMode)
{
	Super::Execute();
	// no need to restore since we are saving the earned 
	if (!bRestoreMode)
	{
		UE_LOG(LogMyMaruSaveGame, Display, TEXT("Grant %d %s"), Amount, *UEnum::GetValueAsString(HealthDataType));
		GetInventory()->Grant(HealthDataType, Amount);
	}
}
