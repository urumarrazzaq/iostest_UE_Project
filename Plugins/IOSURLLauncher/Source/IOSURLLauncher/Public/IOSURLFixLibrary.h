// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IOSURLFixLibrary.generated.h"

/**
 *
 */
UCLASS()
class IOSURLLAUNCHER_API UIOSURLFixLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Opens the specified URL using the modern iOS API. Does nothing on other platforms.
     * @param UrlString The URL to open.
     * @return True if the URL format seems valid and the open attempt was initiated on iOS, false otherwise.
     */
    UFUNCTION(BlueprintCallable, Category = "Utilities|iOS", meta = (DisplayName = "Open URL Modern (iOS Only)"))
    static bool OpenURLModernIOS(const FString& UrlString);
};

