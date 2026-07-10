// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper/HelperBlueprintFunction.h"

// only needed if you are not in the same cpp file
static TAutoConsoleVariable<bool> CVarDebuggerMode(
	TEXT("Maru.DebuggerMode"),
	false,
	TEXT("Maru.DebuggerMode 0 for false and Maru.DebuggerMode 1 for ture"),
	ECVF_Default);

bool UHelperBlueprintFunction::IsEditor()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}

bool UHelperBlueprintFunction::IsDebuggerCommandActive()
{
	return  CVarDebuggerMode.GetValueOnGameThread();
}
