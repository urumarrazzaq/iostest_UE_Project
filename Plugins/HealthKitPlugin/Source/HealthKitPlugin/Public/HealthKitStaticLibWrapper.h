#pragma once

#if PLATFORM_IOS
#include "HealthKitStaticLib.h"
#endif

class FHealthKitStaticLibWrapper
{
public:
    FHealthKitStaticLibWrapper();
    ~FHealthKitStaticLibWrapper();

    void PrintMessage();
    void RequestAuthorization(const TFunctionRef<void(bool, FString)>& Callback);
    void FetchStepCount(const TFunctionRef<void(float, FString)>& Callback);
    void FetchCaloriesBurned(const TFunctionRef<void(float, FString)>& Callback);
    void FetchStandTime(const TFunctionRef<void(float, FString)>& Callback);
    void FetchExerciseMinutes(const TFunctionRef<void(float, FString)>& Callback);
    void FetchActivitySummary(const TFunctionRef<void(float, float, float, FString)>& Callback);
private:
#if PLATFORM_IOS
    HealthKitStaticLib* HealthKitLib;
#endif
};
