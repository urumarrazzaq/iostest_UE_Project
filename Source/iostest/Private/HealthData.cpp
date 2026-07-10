// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthData.h"

UHealthData::UHealthData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	for (EHealthDataType healthDataType : TEnumRange<EHealthDataType>())
	{
		Goals.Add(StaticCast<EHealthDataType>(healthDataType), 0);
	}
	Initialize();
}

UHealthData& UHealthData::Add(const THealthDataValues dValues)
{
	for (auto& dValue : dValues)
	{
		Add(dValue.Key, dValue.Value);
	}

	return *this;
}

void UHealthData::Initialize()
{
	for (EHealthDataType healthDataType : TEnumRange<EHealthDataType>())
	{
		Earned.Add(StaticCast<EHealthDataType>(healthDataType), 0);
		Available.Add(StaticCast<EHealthDataType>(healthDataType), 0);
	}
}

void UHealthData::SetAllEarned(TMap<EHealthDataType, int32> earned)
{
	Earned = earned;
}

void UHealthData::SetAllAvailable(TMap<EHealthDataType, int32> available)
{
	Available = available;
}

void UHealthData::AdjustAvailable(EHealthDataType Type, int32 dValue)
{
	if (StaticCast<int32>(Type) < StaticEnum<EHealthDataType>()->NumEnums())
	{
		// Ensure users can't earn more than their daily goal.
		int32 ToAdd = FMath::Min(Goals[Type] - Earned[Type], dValue);
		Available[Type] = Available[Type] + ToAdd;
	}
}

UHealthData& UHealthData::Add(EHealthDataType Type, int32 dValue)
{
	if (StaticCast<int32>(Type) < StaticEnum<EHealthDataType>()->NumEnums())
	{
		// Ensure users can't earn more than their daily goal.
		int32 ToAdd = FMath::Min(Goals[Type] - Earned[Type], dValue);
		Earned[Type] = Earned[Type] + ToAdd;
		Available[Type] = Available[Type] + ToAdd;
	}

	return *this;
}

bool UHealthData::TrySpend(EHealthDataType Type, int32 Amount)
{
	bool bSpent = false;
	if (StaticCast<int32>(Type) < StaticEnum<EHealthDataType>()->NumEnums())
	{
		if (Amount <= Available[Type])
		{
			Available[Type] = Available[Type] - Amount;
			bSpent = true;
		}
	}

	return bSpent;
}

void UHealthData::SetGoal(EHealthDataType Type, int32 Goal)
{
	if (StaticCast<int32>(Type) < StaticEnum<EHealthDataType>()->NumEnums())
	{
		Goals[Type] = Goal;
	}
}

void UHealthData::SetGoals(THealthDataProgress newGoals)
{
	for (const auto& Goal : newGoals)
	{
		SetGoal(Goal.Key, Goal.Value);
	}
}
