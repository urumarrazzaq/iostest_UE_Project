// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyMaruCharacter.generated.h"

enum class EHealthDataType : uint8;
class UMyMaruInventoryComponent;
class USpringArmComponent;
class UCameraComponent;
class UMyMaruSaveGame;

// We only pay attention if there are <= 2 touches. 
constexpr uint32 kMaxTouches = 2;

UENUM()
enum class EOrbitState : uint8
{
	OrbitState_Idle,
	OrbitState_Rotating,
	OrbitState_Panning,
	OrbitState_Zooming
};

UENUM(BlueprintType)
enum class EControlMode : uint8
{
	Orbit,
	Sowing,
	Cinematic
};

UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	Panning,
	Rotation
};

/**
 * @class AMyMaruCharacter
 * @brief A custom character class inheriting from APawn, designed for handling touch and pinch input events, along with camera and control interactions.
 *
 * This class provides functionality for touch interactions, camera manipulation, and inventory management. It supports multiple touch gestures
 * and provides blueprint customizability for event handling.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLoadDone);

UCLASS()
class AMyMaruCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyMaruCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Configurable
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	bool bAutoRotate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	float MaxRotationSpeed = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	float Friction = 1.2f; // how fast it slows down

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	float MinRotationSpeed = 10.f; // never fully stop

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	bool bIsTouching = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	float CurrentRotationSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	float SwipeInputRotation = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Rotation")
	FVector2D StartingPositionFirstFinger;


	// Header File Changes (if needed)
	float SmoothedSwipeInput = 0.f;
	float MomentumPreservationTimer = 0.f;
	bool bPreserveMomentum = true;

	// These can be exposed to editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SwipeSmoothingSpeed = 4.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MomentumPreservationTime = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinRotationThreshold = 0.0025f;



protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void PostInitializeComponents() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void SyncHealthData();

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnTouchPressed(FVector2D Location);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnTouchHeld(FVector2D Location);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnTouchReleased(FVector2D Location, bool bFromHold);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnCameraRotationStarted(FQuat rotationDelta);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnCameraRotationEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnCameraPanningStarted(FVector locationDelta);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPinchStarted(float Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPinchHeld(float Amount, FVector2D midPointLocation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Events")
	void OnPinchEnded(float Amount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Kernal")
	void OnKernelDestroyed();

	UFUNCTION(BlueprintCallable)
	FQuat GetRotationBetweenPoints_Arcball(FVector2D Start, FVector2D End);

	UFUNCTION(BlueprintCallable)
	FQuat GetRotationBetweenPoints(FVector2D Start, FVector2D End);

	UFUNCTION(BlueprintCallable)
	FVector GetPanningBetweenPoints(FVector2D Start, FVector2D End);

	UFUNCTION(BlueprintCallable)
	void GetCameraTransform(FRotator& OutRotation, FVector& OutLocation) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Camera Rotation")
	void OrbitAroundObject(FRotator& OutRotation, FVector& OutLocation);

	UFUNCTION(BlueprintCallable, Category = "Camera Rotation")
	void SetSwipeInputRotationFromRotation(const FRotator& CurrentRotation);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetOrbitDistance() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	float GetOrbitDistanceDelta() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FQuat GetOrbitRotation() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FQuat GetOrbitRotationDelta() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetCameraPanningDelta() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector GetCameraTarget() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool GetInsufficientHealthData() const;

	UFUNCTION(BlueprintCallable)
	void SetInsufficientHealthData(bool newTarget);

	UFUNCTION(BlueprintCallable)
	void SetCameraTargetStart(const FVector& NewTarget, bool bInterpolate = true);

	UFUNCTION(BlueprintCallable)
	void SetCameraTarget(const FVector& NewTarget);

	UFUNCTION(BlueprintCallable)
	void SetOrbitDistance(const float& NewTarget);

	UFUNCTION(BlueprintCallable)
	void SetOrbitRotation(const FQuat& NewTarget);

	UFUNCTION(BlueprintCallable)
	void SetOrbitRotationDelta(const FQuat& NewTarget);

	UFUNCTION(BlueprintCallable)
	void SetCameraPanningDelta(const FVector& NewTarget);

	UFUNCTION(BlueprintCallable)
	void AdjustOldCameraPanningDelta();

	UFUNCTION(BlueprintCallable)
	void SetCameraNewDistinction(FVector newTarget, float NewDistance, FQuat NewRotaion,
	                             int32 CameraInterpolationIndex = 1);

	UFUNCTION(BlueprintCallable)
	void SetCameraRotationDefault(float Pitch, float Yaw);

	UFUNCTION(BlueprintCallable)
	void SetControlMode(EControlMode NewControlMode);

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly)
	float WaitTillSave = 0.0f;

	UFUNCTION(BlueprintCallable)
	void TriggerSave();

	UFUNCTION(BlueprintCallable)
	void InitSaveGame();

	UFUNCTION(BlueprintCallable)
	void Save(bool& bSuccess);

	UFUNCTION(BlueprintCallable)
	void Load(bool& bSuccess);

	UFUNCTION(BlueprintCallable)
	void GetValidDays(TArray<FDateTime>& OutDays);

	UFUNCTION(BlueprintCallable)
	UDaySave* GetCoreState(const FDateTime& DateTime);

	UFUNCTION(BlueprintCallable)
	bool SetCoreState(const FDateTime& DateTime);

	UFUNCTION(BlueprintCallable)
	void UndoGrowable();
	
	UFUNCTION(BlueprintCallable)
	EControlMode ToggleControlMode();

	UFUNCTION(BlueprintCallable)
	EControlMode GetControlMode() const;

	UFUNCTION(BlueprintCallable)
	void Spend(EHealthDataType Type, int32 Amount);

	UFUNCTION(BlueprintCallable)
	void Grant(EHealthDataType Type, int32 Amount);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HoldThreshold = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHoldTimeReached = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHoldTimeReachedInSameTouchPosition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool CanPinchToZoom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UMyMaruInventoryComponent> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TObjectPtr<UMyMaruSaveGame> SaveGame;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Playback")
	UUserWidget* ActionPlaybackUI;
	
	UPROPERTY(BlueprintCallable, BlueprintAssignable)
	FLoadDone OnLoadDone;
	
	UFUNCTION(BlueprintCallable, Category = "Playback")
	void ActionPlayback_Update();
	
	UFUNCTION(BlueprintCallable, Category = "Playback")
	void ActionPlayback_Left();

	UFUNCTION(BlueprintCallable, Category = "Playback")
	void ActionPlayback_Right();

	UFUNCTION(BlueprintCallable, Category = "Playback")
	void ActionPlayback_Start();

	UFUNCTION(BlueprintCallable, Category = "Playback")
	void ActionPlayback_Endt();


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TestTestTest = 10.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TObjectPtr<UMyMaruSaveGame> SaveGameTempTempTemp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Transient)
	TObjectPtr<UMyMaruSaveGame> DisplayedGame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OrbitDistanceMin = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OrbitDistanceMax = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> OrbitDistances;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECameraMode MaruCameraMode = ECameraMode::Rotation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool InsufficientHealthData = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Tooltip = "Cm to zoom for a full diagonal pinch."))
	float OrbitZoomRate = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (Tooltip = "Time to interpolate camera, in seconds, for each EControlMode."))
	TArray<float> CameraInterpolationTimes;

	FDateTime SavedDate;
	
	UFUNCTION(BlueprintCallable, Category = "Online")
	void LaunchBrowser_TEMP(const FString& URL);

protected:
	void HandleTouchBegin(ETouchIndex::Type FingerIndex, FVector Location);
	void HandleTouchRepeat(ETouchIndex::Type FingerIndex, FVector Location);
	void HandleTouchEnd(ETouchIndex::Type FingerIndex, FVector Location);

	void DrawVector(const FVector& Location, const FVector& Direction, float Length, FColor Color);
	bool AreTwoFingersTouching() const;
	float GetPinchAmount(FVector2D& deltaLocation) const;

	uint8 TouchMask = 0;
	bool bCheckHold = false;
	float HoldTime = 0;
	bool bPinch = false;

	// Per-finger array of last location touched.
	FVector2D LastLocation[kMaxTouches];
	FVector2D LastLocation_Rotation;

	// Used for drag behavior.
	FVector2D StartLocation;

	// Last pinch distance. Used for zoom.
	float LastPinchAmount = 0.0f;

	// Camera settings.
	// Camera OrbitRotationStart.
	UPROPERTY(VisibleAnywhere, meta=(Category="MaruOrbitRotation"))
	FQuat OrbitRotationStart;
	// Camera OrbitRotation.
	UPROPERTY(VisibleAnywhere, meta=(Category="MaruOrbitRotation"))
	FQuat OrbitRotation;
	// Camera OrbitRotationDelta.
	UPROPERTY(VisibleAnywhere, meta=(Category="MaruOrbitRotation"))
	FQuat OrbitRotationDelta;
	// Camera OrbitRotationTarget.
	UPROPERTY(VisibleAnywhere, meta=(Category="MaruOrbitRotation"))
	FQuat OrbitRotationTarget;
	// Camera OrbitRotationDefault.
	UPROPERTY(VisibleAnywhere, meta=(Category="MaruOrbitRotation"))
	FQuat OrbitRotationDefault;

	UPROPERTY(VisibleAnywhere, meta=(Category="Distance"))
	float OrbitDistanceStart;
	UPROPERTY(VisibleAnywhere, meta=(Category="Distance"))
	float OrbitDistance;
	UPROPERTY(VisibleAnywhere, meta=(Category="Distance"))
	float OrbitDistanceDelta;
	UPROPERTY(VisibleAnywhere, meta=(Category="Distance"))
	float OrbitDistanceTarget;

	UPROPERTY(VisibleAnywhere, meta=(Category="Camera"))
	FVector CameraTargetStart;
	UPROPERTY(VisibleAnywhere, meta=(Category="Camera"))
	FVector CameraTarget;
	UPROPERTY(VisibleAnywhere, meta=(Category="Camera"))
	FVector CameraTargetTarget;
	UPROPERTY(VisibleAnywhere, meta=(Category="Camera|Panning"))
	FVector CameraPanningDelta;
	UPROPERTY(VisibleAnywhere, meta=(Category="Camera|Panning"))
	FVector OldCameraPanningDelta;
	UPROPERTY(VisibleAnywhere)
	float InterpolationTimeRemaining;
	UPROPERTY(VisibleAnywhere)
	EControlMode ControlMode = EControlMode::Orbit;
	UPROPERTY(VisibleAnywhere)
	EOrbitState OrbitState = EOrbitState::OrbitState_Idle;

	static FString SaveGameSlotName;
};

inline void AMyMaruCharacter::SetCameraRotationDefault(float Pitch, float Yaw)
{
	OrbitRotationDefault = FQuat(FRotator(Pitch, Yaw, 0.0f));
	OrbitRotation = OrbitRotationDefault;
	OrbitRotationStart = OrbitRotationDefault;
	OrbitRotationTarget = OrbitRotationDefault;
}

DECLARE_LOG_CATEGORY_EXTERN(LogMyMaruCharacter, Log, All);
