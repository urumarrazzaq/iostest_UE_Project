// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HelperBlueprintFunction.generated.h"



/**
 * 
 */
UCLASS()
class IOSTEST_API UHelperBlueprintFunction : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 *	Checks if we are playing inside the editor.
	 *	@returns true when playing in the editor.
	 */
	UFUNCTION(BlueprintPure, Category = "Utilities")
	static bool IsEditor();

	UFUNCTION(BlueprintPure, Category = "Utilities")
	static bool IsDebuggerCommandActive();
};
