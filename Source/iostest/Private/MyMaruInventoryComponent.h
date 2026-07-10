// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HealthData.h"
#include "Components/ActorComponent.h"
#include "MyMaruInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDataUpdated, const UHealthData*, HealthData);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSyncComplete, bool, bSucceeded);

class UHealthKitSubsystem;
class HealthData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UMyMaruInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMyMaruInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "MyMaruInventory", meta=(AutoCreateRefTerm = "OnSyncComplete"))
	bool Sync(const FOnSyncComplete& OnSyncComplete);

	// Delegate for broadcasting when health data has changed.
	UPROPERTY(BlueprintCallable, BlueprintAssignable, Category = "MaruInventory")
	FOnHealthDataUpdated OnHealthDataUpdated;

	UFUNCTION(BlueprintCallable, Category = "MaruInventory", meta=(DevelopmentOnly))
	void SetGoal(EHealthDataType Type = EHealthDataType::MoveCalories, int32 Amount = 0);

	void SetBank(TObjectPtr<UHealthData> bank);

	UFUNCTION(BlueprintCallable, Category = "MaruInventory")
	UHealthData* GetBank() const { return Bank; };

	bool Spend(EHealthDataType Type, int32 Amount);
	void Grant(EHealthDataType Type = EHealthDataType::MoveCalories, int32 Amount = 0);
	// this function will adjust the Available point since the player can close the app while still the Kernel did start to do the growing action
	UFUNCTION(BlueprintCallable, Category = "MaruInventory")
	void AdjustAvailable(EHealthDataType Type, int32 dValue);
	//Register The Saved Date
	void RegisterDate();

protected:
	struct SyncResult
	{
		THealthDataValues HealthDataDelta;
		bool bSucceeded;
	};

	bool bSyncInProgress = false;
	TFuture<TSharedPtr<SyncResult>> SyncResultFuture;
	FOnSyncComplete OnSyncComplete;

	TObjectPtr<UHealthKitSubsystem> HealthKitSubsystem;

	// Bank of health data.
	UPROPERTY(Transient)
	TObjectPtr<UHealthData> Bank;
};
