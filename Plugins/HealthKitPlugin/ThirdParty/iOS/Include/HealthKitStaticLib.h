//
//  HealthKitStaticLib.h
//  HealthKitStaticLib
//
//  Created by Scott Tongue on 1/19/25.
//

#import <Foundation/Foundation.h>

@interface HealthKitStaticLib : NSObject
NS_ASSUME_NONNULL_BEGIN

// Swift methods exposed to Objective-C
- (void)printMessage;
- (NSString *)getMessage;

/// Requests HealthKit authorization to read/write specific data types.
/// @param completion Completion block called when the request is complete.
///        - success: `YES` successful; `NO` otherwise.
///        - errorDescription: A description of the error, nil` if there was no error.
- (void)requestAuthorizationWithCompletion:(void (^)(BOOL success, NSString * _Nullable errorDescription))completion;

/// Fetches the user's step count from HealthKit.
/// @param completion Completion block called with the step count or an error message.
///        - stepCount: The user's step count as a float value.
///        - errorDescription: A description of the error, or `nil` if there was no error.
- (void)fetchStepCountWithCompletion:(void (^)(float stepCount, NSString * _Nullable errorDescription))completion;

/// Fetches the user's active calories burned from HealthKit.
/// @param completion Completion block called with the calorie count or an error message.
///        - calorieCount: The user's calorie count as a float value.
///        - errorDescription: A description of the error, or `nil` if there was no error.
- (void)fetchCaloriesBurnedWithCompletion:(void (^)(float calorieCount, NSString * _Nullable errorDescription))completion;

/// Fetches the user's stand time (in minutes) from HealthKit.
/// @param completion Completion block called with the stand time in minutes or an error message.
///        - standMinutes: The user's stand time in minutes as a float value.
///        - errorDescription: A description of the error, or `nil` if there was no error.
- (void)fetchStandTimeWithCompletion:(void (^)(float standMinutes, NSString * _Nullable errorDescription))completion;

/// Fetches the user's exercise minutes from HealthKit.
/// @param completion Completion block called with the exercise minutes or an error message.
///        - exerciseMinutes: The user's exercise minutes as a float value.
///        - errorDescription: A description of the error, or `nil` if there was no error.
- (void)fetchExerciseMinutesWithCompletion:(void (^)(float exerciseMinutes, NSString * _Nullable errorDescription))completion;

- (void)fetchActivitySummaryWithCompletion:(void (^)(float, float, float, NSString* _Nullable errorDescription))completion;

@end

NS_ASSUME_NONNULL_END

