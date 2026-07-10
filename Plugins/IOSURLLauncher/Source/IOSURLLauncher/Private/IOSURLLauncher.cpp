// Copyright Epic Games, Inc. All Rights Reserved.

#include "IOSURLLauncher.h"

#define LOCTEXT_NAMESPACE "FIOSURLLauncherModule"

void FIOSURLLauncherModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
}

void FIOSURLLauncherModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FIOSURLLauncherModule, IOSURLLauncher)
