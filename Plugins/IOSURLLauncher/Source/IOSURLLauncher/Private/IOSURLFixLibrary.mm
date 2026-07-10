// Fill out your copyright notice in the Description page of Project Settings.

#include "IOSURLFixLibrary.h"
#include "Misc/EngineVersionComparison.h"
#include "Logging/LogMacros.h"
#include "HAL/PlatformProcess.h"

#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#endif


// Define a log category for easier filtering (Optional but recommended)
DEFINE_LOG_CATEGORY_STATIC(LogIOSURLFix, Log, All);

bool UIOSURLFixLibrary::OpenURLModernIOS(const FString& UrlString)
{
#if PLATFORM_IOS
    UE_LOG(LogIOSURLFix, Log, TEXT("OpenURLModernIOS called on iOS with URL: %s"), *UrlString);

    // --- Convert FString to NSString ---
    NSString* nsUrlString = UrlString.GetNSString();
    if (!nsUrlString || [nsUrlString length] == 0)
    {
        UE_LOG(LogIOSURLFix, Warning, TEXT("Invalid or empty URL string provided."));
        return false;
    }

    // --- Create NSURL ---
    NSURL* nsUrl = [NSURL URLWithString : nsUrlString];
    if (!nsUrl)
    {
        UE_LOG(LogIOSURLFix, Warning, TEXT("Could not create NSURL from string: %s"), *UrlString);
        return false;
    }

    // --- Get UIApplication ---
    // Check needed because sharedApplication might not be available in all contexts (e.g. extensions)
    // Although unlikely for typical UE game launch URL usage.
    if (![UIApplication respondsToSelector : @selector(sharedApplication)]) {
        UE_LOG(LogIOSURLFix, Error, TEXT("UIApplication sharedApplication not available."));
        return false;
    }
    UIApplication* SharedApplication = [UIApplication sharedApplication];


    // --- Check if the URL can be opened (Best Practice) ---
    if (![SharedApplication canOpenURL : nsUrl])
    {
        UE_LOG(LogIOSURLFix, Warning, TEXT("iOS reports it cannot open URL"), *UrlString);
        // You might still want to try opening it depending on the URL type,
        // but often this indicates a malformed URL or missing app handler.
        // Returning false here for safety. Consider changing if needed.
        return false;
    }

    // --- Dispatch to Main Thread & Open URL ---
    // UIKit operations must happen on the main thread.
    dispatch_async(dispatch_get_main_queue(), ^ {
        [SharedApplication openURL : nsUrl options : @{} completionHandler: ^ (BOOL success) {
            if (success)
            {
                // Use UE_LOG inside the block for thread safety with logging
                FString SuccessMsg = FString::Printf(TEXT("Successfully initiated opening URL"), *UrlString);
                UE_LOG(LogIOSURLFix, Log, TEXT("%s"), *SuccessMsg);
            }
            else
            {
                FString FailMsg = FString::Printf(TEXT("Failed to initiate opening URL"), *UrlString);
                UE_LOG(LogIOSURLFix, Warning, TEXT("%s"), *FailMsg);
            }
        }];
        });

    // We return true here because the *asynchronous* attempt was initiated.
    // The actual success/failure is reported in the completion handler log messages.
    return true;

#else // Handle non-iOS platforms
    FPlatformProcess::LaunchURL(*UrlString, nullptr, nullptr);
    return true; // Indicate it didn't run the iOS-specific code
#endif // PLATFORM_IOS
}

