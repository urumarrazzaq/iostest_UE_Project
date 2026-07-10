// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HealthData.generated.h"

UENUM(BlueprintType)
enum class EHealthDataType : uint8
{
	MoveCalories,
	ExerciseMinutes,
	StandHoursCount,
	SocialEngagement,
};

ENUM_RANGE_BY_FIRST_AND_LAST(EHealthDataType, EHealthDataType::MoveCalories, EHealthDataType::SocialEngagement);

typedef TMap<EHealthDataType, int32> THealthDataValues;
typedef TMap<EHealthDataType, float> THealthDataProgress;

UCLASS(Blueprintable)
class UHealthData : public UObject
{
	GENERATED_BODY()

public:
	UHealthData(const FObjectInitializer& ObjectInitializer);

	UHealthData& Add(const THealthDataValues dValues);

	void Initialize();
	//used in save game
	void SetAllEarned(TMap<EHealthDataType, int32> earned);
	//used in save game
	void SetAllAvailable(TMap<EHealthDataType, int32> available);

	// this function will adjust the Available point since the player can close the app while still the Kernel did start to do the growing action
	void AdjustAvailable(EHealthDataType Type, int32 dValue);

	UHealthData& Add(EHealthDataType Type, int32 dValue);
	
	bool TrySpend(EHealthDataType Type, int32 Amount);
	
	UFUNCTION(BlueprintCallable, Category = "HealthData")
	int32 GetAvailable(EHealthDataType Type) const
	{
		int32 OutAvailable = 0;

		if (StaticCast<int32>(Type) < StaticEnum<EHealthDataType>()->NumEnums())
		{
			OutAvailable = Available[Type];
		}
		return OutAvailable;
	}

	UFUNCTION(BlueprintCallable, Category = "HealthData")
	int32 GetEarned(EHealthDataType Type) const
	{
		int32 OutEarned = 0;
		if (StaticCast<int32>(Type) < StaticEnum<EHealthDataType>()->NumEnums())
		{
			OutEarned = Earned[Type];
		}

		return OutEarned;
	}

	void SetGoal(EHealthDataType Type, int32 Goal);
	
	void SetGoals(THealthDataProgress newGoals);
	
	// ScottT: Add health data differences to the HealthDataDelta below. 
	// ( This is the data that is sent to the server

	UFUNCTION(BlueprintCallable, Category = "HealthData")
	void GetValue(EHealthDataType Type, int32& OutAvailable, float& OutProgress)
	{
		OutAvailable = Available[Type];
		OutProgress = StaticCast<float>(Earned[Type]) / StaticCast<float>(Goals[Type]);
	}

	TMap<EHealthDataType, int32> GetAllEarned() const { return Earned; }
	TMap<EHealthDataType, int32> GetAllAvailable() const { return Available; }
private:
	UPROPERTY(meta = (AllowPrivateAccess = true), Transient)
	TMap<EHealthDataType, int32> Earned;
	UPROPERTY(meta = (AllowPrivateAccess = true), Transient)
	TMap<EHealthDataType, int32> Available;
	TMap<EHealthDataType, int32> Goals;
};
