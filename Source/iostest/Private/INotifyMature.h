// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INotifyMature.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UINotifyMature : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class IINotifyMature
{
	GENERATED_BODY()

public:
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void NotifyMature();
public:
};
