// Fill out your copyright notice in the Description page of Project Settings.


#include "MyMaruSaveGame.h"

#include "Action.h"
#include "HealthData.h"


void UMyMaruSaveGame::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	int32 NumDays = Days.Num();
	Ar << NumDays;

	if (Ar.IsSaving())
	{
		for (auto& Pair : Days)
		{
			FString Key = Pair.Key;
			Ar << Key;

			TObjectPtr<UDaySave> DaySave = Pair.Value;
			DaySave->Serialize(Ar);
		}
	}
	else if (Ar.IsLoading())
	{
		for (int32 i = 0; i < NumDays; ++i)
		{
			FString DateTimeString;
			Ar << DateTimeString;

			TObjectPtr<UDaySave> DaySave = NewObject<UDaySave>(GetTransientPackage());
			DaySave->Serialize(Ar);

			Days.Add(DateTimeString, DaySave);
		}
	}
}

void UMyMaruSaveGame::RegisterAction(TObjectPtr<UAction> Action, FDateTime DateTime)
{
	TObjectPtr<UDaySave> DaySave = FindOrCreateToday(DateTime);
	Action->Timestamp = DateTime;
	DaySave->AddAction(Action);
}

void UMyMaruSaveGame::RegisterBank(TObjectPtr<UHealthData> bank, FDateTime DateTime)
{
	TObjectPtr<UDaySave> DaySave = FindOrCreateToday(DateTime);
	DaySave->Earned = bank->GetAllEarned();
	DaySave->Available = bank->GetAllAvailable();
}

void UMyMaruSaveGame::PrintAllActions()
{
	FindOrCreateToday()->PrintActions();
}

void UDaySave::Serialize(FArchive& Ar)
{
	UObject::Serialize(Ar);

	FString DateTimeAsString = Date.ToString();
	Ar << DateTimeAsString;
	FDateTime::Parse(DateTimeAsString, Date);
	
	int32 NumActions = Actions.Num();
	Ar << NumActions;

	for (int32 i = 0; i < NumActions; ++i)
	{
		if (Ar.IsSaving())
		{
			FString ClassName = Actions[i]->GetClass()->GetName();
			Ar << ClassName;

			Actions[i]->Serialize(Ar);
		}
		else
		{
			FString ClassName;
			Ar << ClassName;

			FString ClassPath = TEXT("/Script/iostest.") + ClassName;

			// Create a new action of the appropriate type.
			UClass* ActionClass = FindObject<UClass>(nullptr, *ClassPath);
			if (ActionClass)
			{
				TObjectPtr<UAction> NewAction = NewObject<UAction>(GetTransientPackage(), ActionClass);

				// Read in the action.
				NewAction->Serialize(Ar);

				// Add it to the list. 
				Actions.Add(NewAction);
			}
			else
			{
				break;
			}
		}
	}

	Ar << Earned;
	Ar << Available;
}

void UDaySave::ExecuteActions(FTimespan End)
{
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("Executing actions for day: %s"), *Date.ToString());

	ForEachActionInRange(FTimespan(), End, [](TObjectPtr<UAction> Action)
	{
		UE_LOG(LogMyMaruSaveGame, Display, TEXT("Executing action: %s"), *Action->GetName());
		Action->Execute(true);
	});
}

void UDaySave::PrintActions(FTimespan Start, FTimespan End)
{
	ForEachActionInRange(Start, End, [](TObjectPtr<UAction> Action)
	{
		Action->Print();
	});
}

void UDaySave::AddAction(TObjectPtr<UAction> Action)
{
	Actions.Add(Action);
}

void UDaySave::RemoveLastActionSpawnable()
{
	TObjectPtr<UAction> toBeRemovedAction_Spawnable = nullptr;
	TObjectPtr<UAction> toBeRemovedAction_Spend = nullptr;
	for (int32 reverse = Actions.Num() - 1; reverse >= 0; reverse--)
	{
		if (Actions[reverse].IsA(UAction_SpawnGrowable::StaticClass()))
		{
			toBeRemovedAction_Spawnable = Actions[reverse];
		}
		if (Actions[reverse].IsA(UAction_Spend::StaticClass()))
		{
			toBeRemovedAction_Spend = Actions[reverse];
		}
		if (toBeRemovedAction_Spawnable && toBeRemovedAction_Spend)
		{
			break;
		}
	}
	if (toBeRemovedAction_Spawnable)
	{
		Actions.Remove(toBeRemovedAction_Spawnable);
	}
	if (toBeRemovedAction_Spend)
	{
		Actions.Remove(toBeRemovedAction_Spend);
	}
}

void UDaySave::ForEachActionInRange(FTimespan Start, FTimespan End,
                                    const TFunctionRef<void(TObjectPtr<UAction>)>& VisitAction)
{
	FDateTime StartDateTime = Date + Start;
	FDateTime EndDateTime = Date + End;

	UE_LOG(LogMyMaruSaveGame, Display, TEXT("ForEachActionInRange: %s to %s"), *Start.ToString(), *End.ToString());
	UE_LOG(LogMyMaruSaveGame, Display, TEXT("ForEachActionInRange: %s to %s"), *StartDateTime.ToString(),
	       *EndDateTime.ToString());

	for (auto& Action : Actions)
	{
		// HACK MH For now, just visit all actions.
		//if (Action->Timestamp >= StartDateTime && Action->Timestamp <= EndDateTime)
		{
			VisitAction(Action);
		}
	}
}

DEFINE_LOG_CATEGORY(LogMyMaruSaveGame);
