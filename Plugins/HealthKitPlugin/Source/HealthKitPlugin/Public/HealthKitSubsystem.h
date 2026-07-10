#pragma once

#include "CoreMinimal.h"
#include "HealthKitStaticLibWrapper.h"
#include "HealthKitSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAuthorizationComplete, bool, bSucceeded);
UCLASS()

class HEALTHKITPLUGIN_API UHealthKitSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	UHealthKitSubsystem();

	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;
	
	//TEST FUNCTION
	UFUNCTION(BlueprintCallable, Category = "HealthKit")
	void PrintHealthKitMessage();
	
	UFUNCTION(BlueprintCallable, Category = "HealthKit")
	void RequestAuthorization(FOnAuthorizationComplete AuthorizationCallback);
    
	UFUNCTION(BlueprintCallable, Category = "HealthKit")
	bool FetchHealthData();
	
	UFUNCTION(BlueprintCallable, Category = "HealthKit")
	bool IsAuthorized() const { return bIsAuthorized; }

	float GetStepCountDelta() const { return StepCountDelta; }
	float GetCaloriesBurnedDelta() const { return CaloriesBurnedDelta; }
	float GetStandTimeDelta() const { return StandTimeDelta; }
	float GetExerciseMinutesDelta() const { return ExerciseMinutesDelta; }
	
private:
	
	bool bIsAuthorized = false;
    TUniquePtr<FHealthKitStaticLibWrapper> HealthKitWrapper;
	
	float LastStepCount;
	float LastCaloriesBurned;
    float LastStandTime;
    float LastExerciseMinutes;
	
	float StepCountDelta;
	float CaloriesBurnedDelta;
	float StandTimeDelta;
	float ExerciseMinutesDelta;

	FOnAuthorizationComplete OnAuthorizationComplete;
	TFuture<bool> AuthorizationFuture;
};

DECLARE_LOG_CATEGORY_EXTERN(LogHealthKit, Log, All);
