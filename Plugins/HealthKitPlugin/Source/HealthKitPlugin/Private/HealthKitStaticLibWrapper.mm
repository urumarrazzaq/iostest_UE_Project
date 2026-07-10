#include "HealthKitStaticLibWrapper.h"
#include "Misc/ScopeLock.h"


FHealthKitStaticLibWrapper::FHealthKitStaticLibWrapper()
{
#if PLATFORM_IOS
    HealthKitLib = [[HealthKitStaticLib alloc] init];
#endif
}

FHealthKitStaticLibWrapper::~FHealthKitStaticLibWrapper()
{
#if PLATFORM_IOS
    [HealthKitLib release];
    HealthKitLib = nullptr;
#endif
}

void FHealthKitStaticLibWrapper::PrintMessage()
{
#if PLATFORM_IOS
    if (HealthKitLib)
    {
        [HealthKitLib printMessage];
    }
#endif
}

void FHealthKitStaticLibWrapper::RequestAuthorization(const TFunctionRef<void(bool, FString)>& Callback)
{
#if PLATFORM_IOS
    if (HealthKitLib)
    {
        [HealthKitLib requestAuthorizationWithCompletion:^(BOOL success, NSString* errorDescription) 
        {
            FString ErrorStr = errorDescription ? FString(UTF8_TO_TCHAR([errorDescription UTF8String])) : FString();
                        Callback(success, ErrorStr);
        }];
    }
#endif
}

void FHealthKitStaticLibWrapper::FetchStepCount(const TFunctionRef<void(float, FString)>& Callback)
{
#if PLATFORM_IOS

    if (HealthKitLib)
    {
        [HealthKitLib fetchStepCountWithCompletion:^(float stepCount, NSString* errorDescription)
        {
            FString ErrorStr = errorDescription ? FString(UTF8_TO_TCHAR([errorDescription UTF8String])) : FString();
                        Callback(stepCount, ErrorStr);
        }];
    }
#endif
}

void FHealthKitStaticLibWrapper::FetchCaloriesBurned(const TFunctionRef<void(float, FString)>& Callback)
{
#if PLATFORM_IOS
    if (HealthKitLib)
    {
        [HealthKitLib fetchCaloriesBurnedWithCompletion:^(float calorieCount, NSString* errorDescription)
         {
            FString ErrorStr = errorDescription ? FString(UTF8_TO_TCHAR([errorDescription UTF8String])) : FString();
            Callback(calorieCount, ErrorStr);
        }];
    }
#endif
}
    
void FHealthKitStaticLibWrapper::FetchStandTime(const TFunctionRef<void(float, FString)>& Callback)
{
#if PLATFORM_IOS
    if (HealthKitLib)
    {
        [HealthKitLib fetchStandTimeWithCompletion:^(float standMinutes, NSString * _Nullable errorDescription)
         {
            FString ErrorStr = errorDescription ? FString(UTF8_TO_TCHAR([errorDescription UTF8String])) : FString();
                        Callback(standMinutes, ErrorStr);
        }];
    }
#endif
}

void FHealthKitStaticLibWrapper::FetchExerciseMinutes(const TFunctionRef<void(float, FString)>& Callback)
{
#if PLATFORM_IOS
    if (HealthKitLib)
    {
        [HealthKitLib fetchExerciseMinutesWithCompletion:^(float exerciseMinutes, NSString * _Nullable errorDescription)
         {
            FString ErrorStr = errorDescription ? FString(UTF8_TO_TCHAR([errorDescription UTF8String])) : FString();
            Callback(exerciseMinutes, ErrorStr);
        }];
    }
#endif
}

void FHealthKitStaticLibWrapper::FetchActivitySummary(const TFunctionRef<void(float, float, float, FString)>& Callback)
{
#if PLATFORM_IOS
    if (HealthKitLib)
    {
        [HealthKitLib fetchActivitySummaryWithCompletion:^(float activeEnergyBurned, float appleExerciseTime, float appleStandHours, NSString* _Nullable errorDescription)
        {
            FString ErrorStr = errorDescription ? FString(UTF8_TO_TCHAR([errorDescription UTF8String])) : FString();
            Callback(activeEnergyBurned, appleExerciseTime, appleStandHours, ErrorStr);
        }];
    }
#endif
}




