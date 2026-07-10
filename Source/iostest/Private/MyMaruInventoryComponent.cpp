// Fill out your copyright notice in the Description page of Project Settings.


#include "MyMaruInventoryComponent.h"
#include "HealthKitSubsystem.h"
#include "MyMaruSaveGame.h"
#include "MyMaruCharacter.h"

// Sets default values for this component's properties
UMyMaruInventoryComponent::UMyMaruInventoryComponent(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	Bank = CreateDefaultSubobject<UHealthData>(TEXT("HealthData"));
}


// Called when the game starts
void UMyMaruInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache reference to health kit subsubsystem.
	HealthKitSubsystem = GetWorld()->GetSubsystem<UHealthKitSubsystem>();
}


// Called every frame
void UMyMaruInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                              FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (SyncResultFuture.IsReady())
	{
		TSharedPtr<SyncResult> MySyncResult = SyncResultFuture.Consume();

		// Add the synced results. NOTE: this bypasses the Action system entirely.
		Bank->Add(MySyncResult->HealthDataDelta);
		RegisterDate();
		/*
		UE_LOG(LogTemp, Display, TEXT("Health Data Sync Result:"));
		
		for (const auto& HealthData : MySyncResult->HealthDataDelta)
		{
			FString EnumName = UEnum::GetValueAsString(HealthData.Key);
			int32 Value = HealthData.Value;

			UE_LOG(LogTemp, Display, TEXT("  %s = %d"), *EnumName, Value);
		}
		*/

		OnHealthDataUpdated.Broadcast(Bank);

		OnSyncComplete.ExecuteIfBound(MySyncResult->bSucceeded);
		OnSyncComplete.Unbind();
	}
}

bool UMyMaruInventoryComponent::Sync(const FOnSyncComplete& InOnSyncComplete)
{
	if (SyncResultFuture.IsValid() && !SyncResultFuture.IsReady())
		return false;

	OnSyncComplete = InOnSyncComplete;

	SyncResultFuture = Async(EAsyncExecution::Thread, [this](void) -> TSharedPtr<UMyMaruInventoryComponent::SyncResult>
	{
		SyncResult* MySyncResult = new SyncResult();

		// Get the latest health data from the device. This call can be as slow as it needs to be.  
		HealthKitSubsystem->FetchHealthData();

		MySyncResult->HealthDataDelta.Add(
			EHealthDataType::MoveCalories,
			StaticCast<int32>(HealthKitSubsystem->GetCaloriesBurnedDelta()));

		MySyncResult->HealthDataDelta.Add(
			EHealthDataType::ExerciseMinutes,
			StaticCast<int32>(HealthKitSubsystem->GetExerciseMinutesDelta()));

		MySyncResult->HealthDataDelta.Add(
			EHealthDataType::StandHoursCount,
			StaticCast<int32>(HealthKitSubsystem->GetStandTimeDelta()));

		MySyncResult->bSucceeded = true;

		return MakeShareable(MySyncResult);
	});

	return true;
}

bool UMyMaruInventoryComponent::Spend(EHealthDataType Type, int32 Amount)
{
	bool bSpent = Bank->TrySpend(Type, Amount);
	if (bSpent)
	{
		RegisterDate();
		OnHealthDataUpdated.Broadcast(Bank);
	}

	return bSpent;
}

void UMyMaruInventoryComponent::Grant(EHealthDataType Type, int32 Amount)
{
	Bank->Add(Type, Amount);
	RegisterDate();
	OnHealthDataUpdated.Broadcast(Bank);
}

void UMyMaruInventoryComponent::AdjustAvailable(EHealthDataType Type, int32 dValue)
{
	Bank->AdjustAvailable(Type, dValue);
	RegisterDate();
}

void UMyMaruInventoryComponent::RegisterDate()
{
	if (!GetOwner()) { return; }
	AMyMaruCharacter* maru =Cast<AMyMaruCharacter>(GetOwner());
	if (maru && maru->SaveGame)
	{
		maru->SaveGame->RegisterBank(GetBank(), maru->SavedDate);
		maru->TriggerSave();
	}
}

void UMyMaruInventoryComponent::SetGoal(EHealthDataType Type, int32 Amount)
{
	Bank->SetGoal(Type, Amount);
}

void UMyMaruInventoryComponent::SetBank(TObjectPtr<UHealthData> bank)
{
	Bank = bank;
}
