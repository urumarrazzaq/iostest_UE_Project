// Fill out your copyright notice in the Description page of Project Settings.

#include "MyMaruCharacter.h"

#include "Action.h"
#include "InputCoreTypes.h"

#include "MyMaruInventoryComponent.h"
#include "MyMaruSaveGame.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/Button.h" // Include for UButton
#include "Misc/OutputDeviceNull.h"
#include "HAL/PlatformProcess.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#define LOCAL_M_PI 1
#endif

namespace TouchMasks
{
	constexpr uint8 kNoTouches = 0;
	constexpr uint8 kFirstTouch = (1 << ETouchIndex::Touch1);
	constexpr uint8 kSecondTouch = (1 << ETouchIndex::Touch2);
	constexpr uint8 kTwoTouches = kFirstTouch | kSecondTouch;
}

FString AMyMaruCharacter::SaveGameSlotName = TEXT("DefaultSaveGame");

// Sets default values
AMyMaruCharacter::AMyMaruCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("*** NEW CHARACTER ***" ));

	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	RootComponent = SpringArm;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);

	Inventory = CreateDefaultSubobject<UMyMaruInventoryComponent>(TEXT("Inventory"));

	ControlMode = EControlMode::Orbit;

	OrbitRotationStart = FQuat::Identity;
	OrbitRotation = FQuat::Identity;
	OrbitRotationTarget = FQuat::Identity;
	OrbitRotationDelta = FQuat::Identity;

	OrbitDistanceStart = OrbitDistanceMax;
	OrbitDistance = OrbitDistanceMax;
	OrbitDistanceTarget = OrbitDistanceMax;
	OrbitDistanceDelta = 0.0f;

	CameraTargetStart = FVector::ZeroVector;
	CameraTarget = FVector::ZeroVector;
	CameraTargetTarget = FVector::ZeroVector;
	CameraPanningDelta = FVector::ZeroVector;

	CameraTarget = FVector::ZeroVector;

	CameraInterpolationTimes = {.5f, .5f, .1f};
	OrbitDistances = {4000.0f, 2000.0f};
}


// Called when the game starts or when spawned
void AMyMaruCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AMyMaruCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void AMyMaruCharacter::TriggerSave()
{
	WaitTillSave = 0.25f;
}

void AMyMaruCharacter::InitSaveGame()
{
	// First setup the action system with no savegame
	ActionInitialize(nullptr, this);

	bool saveSuccess = false;
	Load(saveSuccess);

	if (saveSuccess)
	{
		const FDateTime today = FDateTime::Now();

		SetCoreState(today);
	}
	else
	{
		SaveGame = NewObject<UMyMaruSaveGame>(GetTransientPackage());
	}

	// Now setup the action system with the savegame
	ActionInitialize(SaveGame, this);

	ActionPlayback_Update();
}

// Called every frame
void AMyMaruCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);


	if (ControlMode == EControlMode::Sowing)
	{
		HoldTime = FMath::Max(HoldTime - DeltaTime, 0.0f);

		if (bCheckHold)
		{
			bHoldTimeReached = FMath::IsNearlyZero(FMath::Abs(HoldTime));
			if (bHoldTimeReachedInSameTouchPosition)
			{
				// Fire hold event.
				FVector2D Location2D(LastLocation[ETouchIndex::Touch1].X, LastLocation[ETouchIndex::Touch1].Y);
				OnTouchHeld(Location2D);
				if (!InsufficientHealthData)
				{
					return;
				}
			}
		}
	}

	switch (OrbitState)
	{
	case EOrbitState::OrbitState_Idle:
		{
			if (FMath::CountBits(TouchMask) == 1)
			{
				OrbitState = EOrbitState::OrbitState_Rotating;
			}
			else if (AreTwoFingersTouching() && !CanPinchToZoom)
			{
				OrbitState = EOrbitState::OrbitState_Panning;
			}
			else if (FMath::CountBits(TouchMask) == 2)
			{
				// This is incredibly rare (2 fingers touching within the same window).
				FVector2D midpointPosition;
				float PinchAmount = GetPinchAmount(midpointPosition);
				LastPinchAmount = PinchAmount;

				OrbitState = EOrbitState::OrbitState_Zooming;
			}
			break;
		}

	case EOrbitState::OrbitState_Panning:
		{
			if (FMath::CountBits(TouchMask) == 0 || FMath::CountBits(TouchMask) == 1)
			{
				OrbitState = EOrbitState::OrbitState_Idle;
			}
			else
			{
				uint8 FingerIndex = 0b111 - FMath::CountLeadingZeros8(TouchMask);
				if (AreTwoFingersTouching())
				{
					OnCameraPanningStarted(GetPanningBetweenPoints(StartLocation, LastLocation[FingerIndex]));
				}
			}
		}
	case EOrbitState::OrbitState_Rotating:
		{
			if (FMath::CountBits(TouchMask) == 0)
			{
				OrbitState = EOrbitState::OrbitState_Idle;
			}
			else if (AreTwoFingersTouching() && !CanPinchToZoom)
			{
				OrbitState = EOrbitState::OrbitState_Panning;
			}
			else if (FMath::CountBits(TouchMask) == 2)
			{
				FVector2D midpointPosition;
				float PinchAmount = GetPinchAmount(midpointPosition);
				LastPinchAmount = PinchAmount;

				OrbitState = EOrbitState::OrbitState_Zooming;
			}
			else
			{
				// One finger touching. Extract the finger index so we can track the appropriate touch position.
				uint8 FingerIndex = 0b111 - FMath::CountLeadingZeros8(TouchMask);
				switch (MaruCameraMode)
				{
				case ECameraMode::Panning:
					OnCameraPanningStarted(GetPanningBetweenPoints(StartLocation, LastLocation[FingerIndex]));
					break;
				case ECameraMode::Rotation:
					OnCameraRotationStarted(GetRotationBetweenPoints(StartLocation, LastLocation[FingerIndex]));
					break;
				}
			}
			break;
		}
	case EOrbitState::OrbitState_Zooming:
		{
			if (FMath::CountBits(TouchMask) == 0)
			{
				OrbitState = EOrbitState::OrbitState_Idle;
			}
			else if (CanPinchToZoom)
			{
				// Handle pinch.
				// Pinch amount is length between points in NDC.
				FVector2D midpointPosition;
				float PinchAmount = GetPinchAmount(midpointPosition);
				float dPinch = LastPinchAmount - PinchAmount;
				OrbitDistanceDelta = FMath::Clamp(OrbitDistanceDelta + dPinch * OrbitZoomRate,
				                                  OrbitDistanceMin - OrbitDistance,
				                                  OrbitDistanceMax - OrbitDistance);
				// Call BP event.
				OnPinchHeld(-dPinch, midpointPosition);
				LastPinchAmount = PinchAmount;
			}
			break;
		}
	}

	// Handle camera interpolation.
	if (InterpolationTimeRemaining > 0.0f)
	{
		// Interpolate with cubic ease-out.
		float T = 1.0f - (InterpolationTimeRemaining / CameraInterpolationTimes[StaticCast<uint8>(ControlMode)]);
		T = 1.0f - FMath::Pow(1.0f - T, 3.0f);
		OrbitRotation = FQuat::Slerp(OrbitRotationStart, OrbitRotationTarget, T);
		OrbitDistance = FMath::Lerp(OrbitDistanceStart, OrbitDistanceTarget, T);
		CameraTarget = FMath::Lerp(CameraTargetStart, CameraTargetTarget, T);
		InterpolationTimeRemaining = FMath::Max(InterpolationTimeRemaining - DeltaTime, 0.0f);

		if (FMath::IsNearlyZero(InterpolationTimeRemaining))
		{
			OrbitRotationStart = OrbitRotationTarget;
			OrbitDistanceStart = OrbitDistanceTarget;
			CameraTargetStart = CameraTargetTarget;
		}
	}


	// If were still counting the wait down, then we can trigger a save. Otherwise we cant
	bool bSaveWaitGTZero = WaitTillSave > 0.0f;

	WaitTillSave -= DeltaTime;

	bool bShouldSave = bSaveWaitGTZero & (WaitTillSave <= 0.0f);

	if (bShouldSave)
	{
		UE_LOG(LogMyMaruCharacter, Display, TEXT("Save game triggered" ));
		bool saveSuccess = false;
		Save(saveSuccess);
		if (saveSuccess)
		{
			UE_LOG(LogMyMaruCharacter, Display, TEXT("Save game success" ));
		}
		else
		{
			UE_LOG(LogMyMaruCharacter, Error, TEXT("Save game failed" ));
		}
	}
}

struct Args
{
	FString Value;
};

void AMyMaruCharacter::ActionPlayback_Update()
{
	//UE_LOG(LogMyMaruCharacter, Display, TEXT("Update action playback called" ));

	if (!ActionPlaybackUI)
	{
		//UE_LOG( LogMyMaruCharacter, Error, TEXT("Action Playback UI not found") );
		return;
	}

	const auto Timestamp = FDateTime::Now();

	FDateTime RequestedDate = FDateTime(Timestamp.GetYear(), Timestamp.GetMonth(), Timestamp.GetDay(), 0, 0, 0);

	TObjectPtr<UDaySave> Day = SaveGame->GetDay(RequestedDate);

	//UE_LOG(LogMyMaruCharacter, Display, TEXT("Update action playback called for day" ));


	if (Day)
	{
		//FString actTotal = FString::Printf(TEXT("%d"), Day->Actions.Num());

		//FOutputDeviceNull ar;


		const FString command = FString::Printf(TEXT("SetTotal \"%d\""), Day->Actions.Num());
		const FString testCmd = FString::Printf(TEXT("TestFunction"));

		//UE_LOG(LogMyMaruCharacter, Display, TEXT("Update action playback called for day with command" ));

		//ActionPlaybackUI->CallFunctionByNameWithArguments(*command, *GLog, NULL, true);	

		//ActionPlaybackUI->CallFunctionByNameWithArguments(*testCmd, *GLog, NULL, true);	

		const auto setTotalFName(TEXT("SetTotal"));
		auto* fnSetTotal = ActionPlaybackUI->FindFunction(setTotalFName);

		//UE_LOG(LogMyMaruCharacter, Display, TEXT("Update action playback called for day with command FindFunction" ));

		Args args;
		args.Value = FString::Printf(TEXT("%d"), Day->Actions.Num());

		ActionPlaybackUI->ProcessEvent(fnSetTotal, &args);

		//UE_LOG(LogMyMaruCharacter, Display, TEXT("Update action playback called for day with command and ProcessEvent" ));	

		//auto actTotalText = Cast<UTextBlock>( ActionPlaybackUI->GetWidgetFromName(TEXT("Act_Total")) );
		//actCurrentText.SetText( FText::FromString( actTotal ) );
	}
	else
	{
		UE_LOG(LogMyMaruCharacter, Error, TEXT("Day not found"));
	}
}

void AMyMaruCharacter::ActionPlayback_Right()
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("Right action playback called" ));
}

void AMyMaruCharacter::ActionPlayback_Left()
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("Left action playback called" ));
}

void AMyMaruCharacter::ActionPlayback_Start()
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("Start action playback called" ));
}

void AMyMaruCharacter::ActionPlayback_Endt()
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("Endt action playback called" ));
}

// Example function to launch a browser
void AMyMaruCharacter::LaunchBrowser_TEMP(const FString& URL)
{
	FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
}

// Called to bind functionality to input
void AMyMaruCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyMaruCharacter::HandleTouchBegin);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyMaruCharacter::HandleTouchEnd);
	PlayerInputComponent->BindTouch(IE_Repeat, this, &AMyMaruCharacter::HandleTouchRepeat);
}

void AMyMaruCharacter::DrawVector(const FVector& Location, const FVector& Direction, float Length, FColor Color)
{
	FVector Start = Location;
	FVector End = Location + Direction * Length;

	DrawDebugDirectionalArrow(GetWorld(), Start, End, 10, Color, false, -1, 0, 10.0f);
}

FQuat AMyMaruCharacter::GetRotationBetweenPoints_Arcball(FVector2D InStart, FVector2D InEnd)
{
	FVector2D ViewportSize;
	TObjectPtr<APlayerController> PlayerController = GetWorld()->GetFirstPlayerController();
	TObjectPtr<UGameViewportClient> ViewportClient = PlayerController->GetLocalPlayer()->ViewportClient;
	ViewportClient->GetViewportSize(ViewportSize);

	// 1 / (viewport_center)
	FVector ViewportCenter(ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f, 0);

	// Project a 2D point onto a 3D unit sphere.
	auto ProjectPointOnUnitSphere = [&ViewportCenter](FVector2D Location) -> FVector
	{
		FVector LocationProjected;
		LocationProjected.Y = (Location.X - ViewportCenter.X) / ViewportCenter.X;
		LocationProjected.Z = (Location.Y - ViewportCenter.Y) / ViewportCenter.Y;
		float LengthSquared = FMath::Square(LocationProjected.Y) + FMath::Square(LocationProjected.Z);
		LocationProjected.X = LengthSquared > .5f
			                      ? 1.0f / (2.0f * FMath::Sqrt(LengthSquared))
			                      : FMath::Sqrt(1.0f - LengthSquared);

		LocationProjected.Normalize();
		return LocationProjected;
	};

	FVector StartProjected = ProjectPointOnUnitSphere(InStart);
	FVector EndProjected = ProjectPointOnUnitSphere(InEnd);

	float CosineTheta = StartProjected.Dot(EndProjected);
	float Angle = FMath::Acos(CosineTheta);

	FVector Axis = StartProjected.Cross(EndProjected);
	Axis.Normalize();

	// This normalize shouldn't be required but what the heck.
	FQuat Rotation(Axis, Angle);
	Rotation.Normalize();

	return Rotation;
}

FQuat AMyMaruCharacter::GetRotationBetweenPoints(FVector2D Start, FVector2D End)
{
	FVector2D ViewportSize;
	TObjectPtr<APlayerController> PlayerController = GetWorld()->GetFirstPlayerController();
	TObjectPtr<UGameViewportClient> ViewportClient = PlayerController->GetLocalPlayer()->ViewportClient;
	ViewportClient->GetViewportSize(ViewportSize);

	float dx = (End.X - Start.X) / ViewportSize.X * M_PI;
	float dy = (End.Y - Start.Y) / ViewportSize.Y * M_PI;
	if (FMath::IsNearlyZero(dx) || FMath::IsNearlyZero(dy))
	{
		return FQuat::Identity;
	}
	FQuat RotationY(FVector(0.0f, 1.0f, 0.0f), dy);
	FQuat RotationZ(FVector(0.0f, 0.0f, 1.0f), dx);

	FQuat finalRotationQuat = RotationZ * OrbitRotation * RotationY * OrbitRotation.Inverse();
	return finalRotationQuat;
}

FVector AMyMaruCharacter::GetPanningBetweenPoints(FVector2D Start, FVector2D End)
{
	FVector2D ViewportSize;
	TObjectPtr<APlayerController> PlayerController = GetWorld()->GetFirstPlayerController();
	TObjectPtr<UGameViewportClient> ViewportClient = PlayerController->GetLocalPlayer()->ViewportClient;
	ViewportClient->GetViewportSize(ViewportSize);

	float dx = (End.X - Start.X);
	float dy = (End.Y - Start.Y);
	return FVector(-dx, -dy, 0);
}

void AMyMaruCharacter::GetCameraTransform(FRotator& OutRotation, FVector& OutLocation) const
{
	float OrbitDistanceTotal = OrbitDistance + OrbitDistanceDelta;
	FVector CameraOffset(-OrbitDistanceTotal, 0.f, 0.f);

	FQuat OrbitRotationTotal = OrbitRotationDelta * OrbitRotation;

	FVector location = CameraTarget + OrbitRotationTotal.RotateVector(CameraOffset);
	// Location is WS offset rotated based on orbit rotation.
	OutLocation = location + CameraPanningDelta + OldCameraPanningDelta;

	// Rotation is look-at from location to target.
	OutRotation = OrbitRotationTotal.Rotator();
}

void AMyMaruCharacter::OrbitAroundObject(FRotator& OutRotation, FVector& OutLocation)
{
	float OrbitDistanceTotal = OrbitDistance + OrbitDistanceDelta;
	FVector CameraOffset(-OrbitDistanceTotal, 0.f, 0.f);
	float DeltaTime = GetWorld()->GetDeltaSeconds();

	// Smooth swipe input to avoid flick
	SmoothedSwipeInput = FMath::FInterpTo(SmoothedSwipeInput, SwipeInputRotation, DeltaTime, SwipeSmoothingSpeed);
	float RotationDirection = FMath::Clamp(SmoothedSwipeInput, -1.f, 1.f);

	// Handle tap-released momentum preservation
	if (!bIsTouching)
	{
		if (bAutoRotate)
		{
			CurrentRotationSpeed = FMath::FInterpTo(CurrentRotationSpeed, MinRotationSpeed, DeltaTime, Friction);
		}

		// If recently released tap, preserve momentum briefly
		if (bPreserveMomentum)
		{
			MomentumPreservationTimer += DeltaTime;
			if (MomentumPreservationTimer > MomentumPreservationTime)
			{
				bPreserveMomentum = false;
				MomentumPreservationTimer = 0.f;
			}
		}
		else
		{
			// Full decay when not preserving
			CurrentRotationSpeed = FMath::FInterpTo(CurrentRotationSpeed, 0.f, DeltaTime, Friction);
		}
	}
	else // if touching
	{
		CurrentRotationSpeed = FMath::FInterpTo(CurrentRotationSpeed, 0.f, DeltaTime, Friction);
		bPreserveMomentum = true; // quick tap release will carry momentum
		MomentumPreservationTimer = 0.f;
	}

	// Threshold to prevent lock-up freeze
	if (CurrentRotationSpeed < MinRotationThreshold)
	{
		CurrentRotationSpeed = 0.f;
	}

	CurrentRotationSpeed = FMath::Clamp(CurrentRotationSpeed, 0.f, MaxRotationSpeed);

	// Apply rotation
	float RotationDelta = RotationDirection * CurrentRotationSpeed * DeltaTime;
	FQuat AutoRotationQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(RotationDelta));
	OrbitRotation = AutoRotationQuat * OrbitRotation;

	FQuat OrbitRotationTotal = OrbitRotationDelta * OrbitRotation;
	FVector Location = CameraTarget + OrbitRotationTotal.RotateVector(CameraOffset);
	OutLocation = Location + CameraPanningDelta + OldCameraPanningDelta;
	OutRotation = OrbitRotationTotal.Rotator();
}







void AMyMaruCharacter::SetSwipeInputRotationFromRotation(const FRotator& CurrentRotation)
{
	static float LastYaw = CurrentRotation.Yaw;

	float YawDelta = CurrentRotation.Yaw - LastYaw;

	if (YawDelta > 180.f) YawDelta -= 360.f;
	if (YawDelta < -180.f) YawDelta += 360.f;

	if (FMath::Abs(YawDelta) > KINDA_SMALL_NUMBER)
	{
		SwipeInputRotation = YawDelta > 0.f ? 1.f : -1.f;
	}
	else
	{
		SwipeInputRotation = 0.f;
	}

	LastYaw = CurrentRotation.Yaw;
}




float AMyMaruCharacter::GetOrbitDistance() const
{
	return OrbitDistance;
}

float AMyMaruCharacter::GetOrbitDistanceDelta() const
{
	return OrbitDistanceDelta;
}

FQuat AMyMaruCharacter::GetOrbitRotation() const
{
	return OrbitRotation;
}

FQuat AMyMaruCharacter::GetOrbitRotationDelta() const
{
	return OrbitRotationDelta;
}

FVector AMyMaruCharacter::GetCameraPanningDelta() const
{
	return CameraPanningDelta;
}

FVector AMyMaruCharacter::GetCameraTarget() const
{
	return CameraTarget;
}

bool AMyMaruCharacter::GetInsufficientHealthData() const
{
	return InsufficientHealthData;
}

void AMyMaruCharacter::SetInsufficientHealthData(bool newTarget)
{
	InsufficientHealthData = newTarget;
}

void AMyMaruCharacter::SetCameraTargetStart(const FVector& NewTarget, bool bInterpolate)
{
	CameraTargetStart = CameraTarget;
	CameraTargetTarget = NewTarget;
	if (bInterpolate)
	{
		InterpolationTimeRemaining = CameraInterpolationTimes[StaticCast<uint8>(EControlMode::Orbit)];
	}
}

void AMyMaruCharacter::SetCameraTarget(const FVector& NewTarget)
{
	CameraTarget = NewTarget;
}

void AMyMaruCharacter::SetOrbitDistance(const float& NewTarget)
{
	if (FMath::Abs(NewTarget) > KINDA_SMALL_NUMBER) // Check if there's significant input
	{
		float TickValue = (OrbitDistance - OrbitDistanceMin) / 10.f;
		float MinTickValue = (OrbitDistanceMax - OrbitDistanceMin) / 50.f;

		OrbitDistance += FMath::Sign(-NewTarget) * FMath::Max(TickValue, MinTickValue);
		OrbitDistance = FMath::Clamp(OrbitDistance, OrbitDistanceMin, OrbitDistanceMax);
	}
}

void AMyMaruCharacter::SetOrbitRotation(const FQuat& NewTarget)
{
	OrbitRotation = NewTarget;
}

void AMyMaruCharacter::SetOrbitRotationDelta(const FQuat& NewTarget)
{
	OrbitRotationDelta = NewTarget;
}

void AMyMaruCharacter::SetCameraPanningDelta(const FVector& NewTarget)
{
	CameraPanningDelta = NewTarget;
}

void AMyMaruCharacter::AdjustOldCameraPanningDelta()
{
	if (CameraPanningDelta.IsNearlyZero() || OrbitState != EOrbitState::OrbitState_Idle)
	{
		return;
	}
	OldCameraPanningDelta = OldCameraPanningDelta + CameraPanningDelta;
	CameraPanningDelta = FVector::Zero();
}

void AMyMaruCharacter::SetCameraNewDistinction(FVector newTarget, float NewDistance, FQuat NewRotaion,
                                               int32 CameraInterpolationIndex)
{
	OldCameraPanningDelta = newTarget;
	OrbitDistanceStart = OrbitDistance;
	OrbitDistanceTarget = NewDistance;
	OrbitRotationTarget = NewRotaion;
	InterpolationTimeRemaining = CameraInterpolationTimes[CameraInterpolationIndex];
}

void AMyMaruCharacter::SetControlMode(EControlMode NewControlMode)
{
	ControlMode = NewControlMode;
	//SetCameraNewDistinction(FVector::ZeroVector, OrbitDistances[StaticCast<uint8>(ControlMode)], OrbitRotationDefault);
	//OrbitRotationStart = OrbitRotation;
	//InterpolationTimeRemaining = CameraInterpolationTimes[StaticCast<uint8>(ControlMode)];
	bCheckHold = false;
	bHoldTimeReached = false;
	HoldTime = 0.0f;
}

void AMyMaruCharacter::Save(bool& bSuccess)
{
	FArchive& Ar = *(new FArchive);

	UE_LOG(LogMyMaruCharacter, Display, TEXT("Maru::Save() proceeding..g" ));
	bSuccess = UGameplayStatics::SaveGameToSlot(SaveGame, SaveGameSlotName, 0);
	if (bSuccess)
	{
		UE_LOG(LogMyMaruCharacter, Display, TEXT("Success" ));
		SaveGame->PrintAllActions();
	}
	else
	{
		UE_LOG(LogMyMaruCharacter, Warning, TEXT("Failure saving game" ));
	}
}

void AMyMaruCharacter::Load(bool& bSuccess)
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("Loading save game..." ));
	bSuccess = false;
	if (UMyMaruSaveGame* LoadedSave = Cast<UMyMaruSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveGameSlotName, 0)))
	{
		UE_LOG(LogMyMaruCharacter, Display, TEXT("Success" ));

		SaveGame = *LoadedSave;
		SaveGame->PrintAllActions();

		bSuccess = true;
	}
	else
	{
		UE_LOG(LogMyMaruCharacter, Display, TEXT("FAILURE" ));
	}
}

void AMyMaruCharacter::GetValidDays(TArray<FDateTime>& OutDays)
{
	SaveGame->GetValidDays(OutDays);
}

//static UDaySave s_blankDay;

UDaySave* AMyMaruCharacter::GetCoreState(const FDateTime& DateTime)
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("GetCoreState called" ));

	// Validate input.

	TObjectPtr<UDaySave> Day = SaveGame->GetDay(DateTime);
	if (Day)
	{
		return Day;
	}
	else
	{
		UE_LOG(LogMyMaruCharacter, Display, TEXT("Day not found" ));
		//return &s_blankDay;
	}

	return nullptr;
}

bool AMyMaruCharacter::SetCoreState(const FDateTime& DateTime)
{
	UE_LOG(LogMyMaruCharacter, Display, TEXT("SetCoreState called" ));

	// Validate input.
	SavedDate = DateTime;
	//FDateTime(DateTime.GetYear(), DateTime.GetMonth(), DateTime.GetDay(), 0, 0, 0);

	bool bSuccess = false;

	Inventory->GetBank()->Initialize();
	TObjectPtr<UDaySave> Day = SaveGame->GetDay(SavedDate);
	if (Day)
	{
		// Reset the world.
		GetWorld()->GetSubsystem<UCoreManagerSubSystem>()->Reset();
		// End time defaults to the last second of the day.
		FTimespan EndTime(23, 59, 59);

		// If we're on the same today (today), use the requested time. 
		FDateTime Now = FDateTime::Now();
		FDateTime TodayDate = FDateTime(Now.GetYear(), Now.GetMonth(), Now.GetDay(), 0, 0, 0);

		if (SavedDate == TodayDate)
		{
			EndTime = FTimespan(DateTime.GetHour(), DateTime.GetMinute(), DateTime.GetSecond());
		}

		UE_LOG(LogMyMaruCharacter, Display, TEXT("End time: %s"), *EndTime.ToString());

		Day->ExecuteActions(EndTime);
		
		// it will set the saved bank since the Action_Grant is not used at all in the App.
		// Please if that came with an issue let Marc know he supported the save system he will know the best
		if (!Day->GetBankAvailable().IsEmpty() && !Day->GetBankEarned().IsEmpty())
		{
			Inventory->GetBank()->SetAllEarned(Day->GetBankEarned());
			Inventory->GetBank()->SetAllAvailable(Day->GetBankAvailable());
		}
		OnLoadDone.Broadcast();
		bSuccess = true;
	}
	else
	{
		UE_LOG(LogMyMaruCharacter, Display, TEXT("Day not found" ));
	}

	return bSuccess;
}
void AMyMaruCharacter::UndoGrowable()
{
	if (!SaveGame)
	{
		return;
	}
	TObjectPtr<UDaySave> Day = SaveGame->GetDay(SavedDate);
	if (Day)
	{
		TObjectPtr<UCoreManagerSubSystem> coreManagerSubSystem = GetWorld()->GetSubsystem<UCoreManagerSubSystem>();
		bool bIsRemoved = coreManagerSubSystem->TryRemoveLastGrowables();
		if (bIsRemoved)
		{
			Day->RemoveLastActionSpawnable();
			TriggerSave();
		}
	}
}

EControlMode AMyMaruCharacter::ToggleControlMode()
{
	EControlMode NewControlMode = (ControlMode == EControlMode::Orbit) ? EControlMode::Sowing : EControlMode::Orbit;
	SetControlMode(NewControlMode);
	return NewControlMode;
}

EControlMode AMyMaruCharacter::GetControlMode() const
{
	return ControlMode;
}

template <typename TActionType>
void ProcessHealthAction(AMyMaruCharacter* pMaru, EHealthDataType Type, int32 Amount)
{
	TObjectPtr<TActionType> Action = NewObject<TActionType>(pMaru->GetWorld());
	Action->HealthDataType = Type;
	Action->Amount = Amount;
	
	Action->RegisterDate(pMaru->SavedDate);

	Action->Execute();
	Action->Print();

	if (!Action.IsA(UAction_Spend::StaticClass()))
	{
		pMaru->TriggerSave();
	}
}

void AMyMaruCharacter::Spend(EHealthDataType Type, int32 Amount)
{
	ProcessHealthAction<UAction_Spend>(this, Type, Amount);
}

void AMyMaruCharacter::Grant(EHealthDataType Type, int32 Amount)
{
	ProcessHealthAction<UAction_Grant>(this, Type, Amount);
}

void AMyMaruCharacter::HandleTouchBegin(ETouchIndex::Type FingerIndex, FVector Location)
{
	// We only support 2-finger touch.
	if (FingerIndex > ETouchIndex::Touch2)
	{
		return;
	}

	TouchMask |= (1 << FingerIndex);

	FVector2D Location2D(StaticCast<float>(Location.X), StaticCast<float>(Location.Y));
	LastLocation[FingerIndex] = Location2D;
	StartLocation = Location2D;

	if (FMath::CountBits(TouchMask) == 1)
	{
		// This press might be the start of the hold. 
		bCheckHold = true;
		HoldTime = HoldThreshold;

		OnTouchPressed(Location2D);
	}
	else if (FMath::CountBits(TouchMask) == 2)
	{
		FVector2D midpointPosition;
		OnPinchStarted(GetPinchAmount(midpointPosition));
	}
}

void AMyMaruCharacter::HandleTouchRepeat(ETouchIndex::Type FingerIndex, FVector Location)
{
	// We only support 2-finger touch.
	if (FingerIndex > ETouchIndex::Touch2)
	{
		return;
	}

	bCheckHold = true;
	LastLocation[FingerIndex] = FVector2D(Location.X, Location.Y);
}

void AMyMaruCharacter::HandleTouchEnd(ETouchIndex::Type FingerIndex, FVector Location)
{
	// We only support 2-finger touch.
	if (FingerIndex > ETouchIndex::Touch2)
	{
		return;
	}

	TouchMask &= ~(1 << FingerIndex);

	switch (ControlMode)
	{
	case EControlMode::Orbit:
		{
			if (bPinch)
			{
				bPinch = false;

				// Save off new distance and reset delta. 
				OrbitDistance = OrbitDistance + OrbitDistanceDelta;
				OrbitDistanceDelta = 0.0f;
				FVector2D midpointPosition;
				// Call BP event.
				OnPinchEnded(GetPinchAmount(midpointPosition));
			}
			else
			{
				// Store new rotation and reset delta. 
				OrbitRotation = OrbitRotationDelta * OrbitRotation;
				OrbitRotationDelta = FQuat::Identity;
				OnCameraRotationEnded();
			}

			break;
		}

	case EControlMode::Sowing:
		{
			bool bFromHold = FMath::IsNearlyZero(HoldTime) ? true : false;
			if (FingerIndex == ETouchIndex::Touch1)
			{
				// Store new rotation and reset delta. 
				OrbitRotation = OrbitRotationDelta * OrbitRotation;
				OrbitRotationDelta = FQuat::Identity;
				OnCameraRotationEnded();

				FVector2D Location2D(Location.X, Location.Y);
				OnTouchReleased(Location2D, bFromHold);
				ActionPlayback_Update();
			}
			else if (FingerIndex == ETouchIndex::Touch2 && FingerIndex == ETouchIndex::Touch1)
			{
				if (bPinch)
				{
					bPinch = false;

					// Save off new distance and reset delta. 
					OrbitDistance = OrbitDistance + OrbitDistanceDelta;
					OrbitDistanceDelta = 0.0f;
					FVector2D midpointPosition;

					// Call BP event.
					OnPinchEnded(GetPinchAmount(midpointPosition));
				}

				break;
			}
			bCheckHold = false;
			bHoldTimeReached = false;

			break;
		}
	}
}

bool AMyMaruCharacter::AreTwoFingersTouching() const
{
	return (TouchMask & TouchMasks::kTwoTouches) == TouchMasks::kTwoTouches;
}

float AMyMaruCharacter::GetPinchAmount(FVector2D& deltaLocation) const
{
	// Detect pinch.
	FVector2D ViewportSize;
	TObjectPtr<APlayerController> PlayerController = GetWorld()->GetFirstPlayerController();
	TObjectPtr<UGameViewportClient> ViewportClient = PlayerController->GetLocalPlayer()->ViewportClient;
	ViewportClient->GetViewportSize(ViewportSize);

	FVector2D LocationUnitized[kMaxTouches];
	LocationUnitized[ETouchIndex::Touch1] = LastLocation[ETouchIndex::Touch1] / ViewportSize;
	LocationUnitized[ETouchIndex::Touch2] = LastLocation[ETouchIndex::Touch2] / ViewportSize;
	deltaLocation = LocationUnitized[ETouchIndex::Touch1] - LocationUnitized[ETouchIndex::Touch2] / 2;
	FVector2D delta = LocationUnitized[ETouchIndex::Touch1] - LocationUnitized[ETouchIndex::Touch2];
	return delta.Length();
}

DEFINE_LOG_CATEGORY(LogMyMaruCharacter);
