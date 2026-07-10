// Fill out your copyright notice in the Description page of Project Settings.

#include "iostest.h"
#include "Modules/ModuleManager.h"
#include "Runtime/Core/Public/Misc/CoreDelegates.h"
#include "MyMaruCharacter.h"
#include "MyMaruInventoryComponent.h"

class FMyMaruGameModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Bind to the delegate when the app enters foreground (regains focus)
		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddRaw(this, &FMyMaruGameModule::OnAppRegainFocus);
	}

	virtual void ShutdownModule() override
	{
		// Unbind the delegate on shutdown to avoid crashes
		FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
	}

	// Function that gets called when app regains focus
	void OnAppRegainFocus()
	{
		APlayerController* PlayerController = GEngine->GetWorld()->GetFirstPlayerController();
		AMyMaruCharacter* MyMaruCharacter = PlayerController->GetPawn<AMyMaruCharacter>();
		MyMaruCharacter->SyncHealthData();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FMyMaruGameModule, iostest, "iostest" );
