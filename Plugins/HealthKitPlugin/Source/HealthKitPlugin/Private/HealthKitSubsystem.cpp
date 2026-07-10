#include "HealthKitSubsystem.h"
#include "Engine/World.h"
#include "Async/Async.h"

UHealthKitSubsystem::UHealthKitSubsystem()
{
    
    HealthKitWrapper = MakeUnique<FHealthKitStaticLibWrapper>();
}

TStatId UHealthKitSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UHealthKitSubsystem, STATGROUP_Tickables);   
}

void UHealthKitSubsystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (AuthorizationFuture.IsReady())
    {
        // Retrieve result and invalidate future.
        bIsAuthorized = AuthorizationFuture.Consume();

        // Call user-supplied delegate.
        (void) OnAuthorizationComplete.ExecuteIfBound( bIsAuthorized);
        OnAuthorizationComplete.Unbind();
    }
}

void UHealthKitSubsystem::PrintHealthKitMessage()
{
#if PLATFORM_IOS
    if (HealthKitWrapper.IsValid())
    {
        HealthKitWrapper->PrintMessage();
    }
#endif
}

void UHealthKitSubsystem::RequestAuthorization(FOnAuthorizationComplete AuthorizationCallback)
{
    if (HealthKitWrapper.IsValid())
    {
        OnAuthorizationComplete = AuthorizationCallback;
        AuthorizationFuture = Async(EAsyncExecution::Thread, [this]() -> bool {
            bool bSuccess = false;
            FString ErrorDescription;
            
#if PLATFORM_IOS
            UE_LOG(LogHealthKit, Log, TEXT("Requesting authorization..."));
            TPromise<bool> Promise;
            TFuture<bool> Future = Promise.GetFuture();
            
            HealthKitWrapper->RequestAuthorization([&Promise, &ErrorDescription](const bool success, FString errorDescription) -> void {
                ErrorDescription = errorDescription;
                Promise.SetValue(success);
            });

            bSuccess = Future.Get();
            if (!bSuccess)
            {
                UE_LOG(LogHealthKit, Error, TEXT("Authorization failed: %s."), *ErrorDescription);
            }
            else
            {
                UE_LOG(LogHealthKit, Log, TEXT("Authorization succeeded."));
            }
            
#elif PLATFORM_MAC
            // Simulate a slow background process...
            bSuccess = true;
            FPlatformProcess::Sleep(2.0);
#endif
           
            return bSuccess;
        });
    }

}


bool UHealthKitSubsystem::FetchHealthData()
{
#if PLATFORM_IOS
    if (bIsAuthorized && HealthKitWrapper.IsValid())
    {
        UE_LOG(LogHealthKit, Log, TEXT("Fetching health data..."));
        FString ErrorDescription;
        
        // Fetch activity summary.
        UE_LOG(LogHealthKit, Log, TEXT("  Fetching activity summary"));
        float CaloriesBurned = 0.0f, ExerciseMinutes = 0.0f, StandTime = 0.0f;
        TPromise<bool> ActivitySummaryPromise;
        TFuture<bool> ActivitySummaryFuture = ActivitySummaryPromise.GetFuture();
        HealthKitWrapper->FetchActivitySummary([this, &ActivitySummaryPromise, &CaloriesBurned, &ExerciseMinutes, &StandTime, &ErrorDescription](float activeEnergyBurned, float appleExerciseTime, float appleStandHours, FString errorDesc) -> void
        {
            UE_LOG(LogHealthKit, Log, TEXT("  Fetching activity summary complete"));
            bool bSuccess = false;
            if (errorDesc.IsEmpty())
            {
                CaloriesBurned = activeEnergyBurned;
                ExerciseMinutes = appleExerciseTime;
                StandTime = appleStandHours;
                bSuccess = true;
            }
            else
            {
                ErrorDescription = errorDesc;
            }

            ActivitySummaryPromise.SetValue(bSuccess);
        });
        
        ActivitySummaryFuture.Wait();

        
        UE_LOG(LogHealthKit, Log, TEXT("  Waiting for results"));
        
        // Wait until preceding tasks have completed.
        {
            UE_LOG(LogHealthKit, Log, TEXT("  Processing results"));
            
            CaloriesBurnedDelta = CaloriesBurned - LastCaloriesBurned;
            StandTimeDelta = StandTime - LastStandTime;
            ExerciseMinutesDelta = ExerciseMinutes - LastExerciseMinutes;
                    
            LastCaloriesBurned = CaloriesBurned;
            LastStandTime = StandTime;
            LastExerciseMinutes = ExerciseMinutes;

            return  true;
        }
        
    }
    else
    {
        
        UE_LOG(LogHealthKit, Warning, TEXT("HealthKit not authorized. Cannot fetch health data."));
        return false;
    }
   
#endif
    return true;
}

DEFINE_LOG_CATEGORY(LogHealthKit);
