// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "HealthData.h"
#include "MyMaruSaveGame.generated.h"

class AGrowable;
class UAction;
class UHealthData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRegisterAction, UAction*, Act);


UCLASS(Blueprintable)
class UDaySave : public UObject
{
	GENERATED_BODY()

public:
	UDaySave() : Super()
	{
	}


	UDaySave(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
	{
	}

	void Initialize(FDateTime InDate = FDateTime::Today())
	{
		Date = InDate;
	}

	virtual void Serialize(FArchive& Ar) override;

	// Plays back actions within a given time span.
	void ExecuteActions(FTimespan End = FTimespan(23, 59, 59));

	void PrintActions(FTimespan Start = FTimespan(0, 0, 0),
	                  FTimespan End = FTimespan(23, 59, 59));

	// Add a new action.
	void AddAction(TObjectPtr<UAction> Action);

	// Undo Action Will Always remove Last Growable
	void RemoveLastActionSpawnable();
	
	// Set bank EranedAvailable.
	void SetBankEarnedAvailable(TMap<EHealthDataType, int32> earned, TMap<EHealthDataType, int32> available);

	// Get bank Eraned.
	TMap<EHealthDataType, int32> GetBankEarned() const { return Earned; };
	// Get bank Available.
	TMap<EHealthDataType, int32> GetBankAvailable() const{ return Available; };

	UPROPERTY(BlueprintReadOnly, Transient)
	FDateTime Date;

	// A record of player actions performed on this day.
	UPROPERTY(BlueprintReadOnly, Transient)
	TArray<TObjectPtr<UAction>> Actions;

	// A record of player Inventory
	UPROPERTY(BlueprintReadOnly, Transient)
	TMap<EHealthDataType, int32> Earned;
	UPROPERTY(BlueprintReadOnly, Transient)
	TMap<EHealthDataType, int32> Available;

private:
	void ForEachActionInRange(FTimespan Start, FTimespan End, const TFunctionRef<void(TObjectPtr<UAction>)>& Action);
};


/**
 * 
 */
UCLASS(Blueprintable)
class UMyMaruSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FRegisterAction OnRegisterAction;


	UMyMaruSaveGame(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
	{
		FindOrCreateToday();
	}

	virtual void Serialize(FArchive& Ar) override;

	void RegisterAction(TObjectPtr<UAction> Action, FDateTime DateTime);

	void RegisterBank(TObjectPtr<UHealthData> bank, FDateTime DateTime);

	// Plays back actions within a given time span.
	void ExecuteActions(FTimespan Start = FTimespan(0, 0, 0),
	                    FTimespan End = FTimespan(23, 59, 59));

	void PrintActions(FTimespan Start = FTimespan(0, 0, 0),
	                  FTimespan End = FTimespan(23, 59, 59));

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void PrintAllActions();

	TObjectPtr<UDaySave> GetDay(FDateTime DateTime)
	{
		FDateTime DateOnly(DateTime.GetYear(), DateTime.GetMonth(), DateTime.GetDay());
		TObjectPtr<UDaySave>* Day = Days.Find(DateOnly.ToString());
		return Day ? *Day : nullptr;
	}

	void GetValidDays(TArray<FDateTime>& OutDays) const
	{
		TArray<FString> Keys;
		Days.GetKeys(Keys);
		for (auto& Key : Keys)
		{
			FDateTime DateOnly;
			FDateTime::Parse(Key, DateOnly);
			OutDays.Add(DateOnly);
		}

		OutDays.Sort();
	}

private:
	TObjectPtr<UDaySave> FindOrCreateToday(FDateTime DateTime = FDateTime::Today())
	{
		FDateTime DateOnly(DateTime.GetYear(), DateTime.GetMonth(), DateTime.GetDay());

		TObjectPtr<UDaySave>* Day = Days.Find(DateOnly.ToString());
		if (Day == nullptr)
		{
			TObjectPtr<UDaySave> NewDay = NewObject<UDaySave>(GetTransientPackage());
			NewDay->Initialize(DateOnly);
			return Days.Add(DateOnly.ToString(), NewDay);
		}
		else
		{
			return *Day;
		}
	}

	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UDaySave>> Days;
};

DECLARE_LOG_CATEGORY_EXTERN(LogMyMaruSaveGame, Log, All);
