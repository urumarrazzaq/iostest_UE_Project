// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CoreManagerSubSystem.generated.h"

class AGrowable;
class UCoreConfig;
struct FGrowableConfig;
class ULandscapeComponent;
class ALandscape;
class AActor;
class AKernel;

constexpr uint8 kNoLayer = 255;

struct LandscapeSampler
{
	LandscapeSampler(TArray<uint8>& InTarget, TObjectPtr<ULandscapeComponent> InLandscapeComponent);

	float Sample(FVector Coordinates);

	TArray<uint8>& Target;
	int32 Stride;
	TObjectPtr<ULandscapeComponent> LandscapeComponent;
};

USTRUCT(BlueprintType)
struct FCheckResult
{
	GENERATED_BODY()

	FCheckResult(FVector InLocation = FVector::ZeroVector, uint8 InLayer = kNoLayer,
	             bool InOccupied = false) : Location(InLocation), Layer(InLayer), Occupied(InOccupied)
	{
	}

	UPROPERTY(BlueprintReadOnly)
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	uint8 Layer = kNoLayer;

	UPROPERTY(BlueprintReadOnly)
	bool Occupied = false;
};

USTRUCT(BlueprintType)
struct FSowResult
{
	GENERATED_BODY()

	FSowResult(FVector InLocation = FVector(), uint8 InLayer = kNoLayer) : Location(InLocation), Layer(InLayer),
	                                                                       bSuccess(false)
	{
	}

	UPROPERTY(BlueprintReadOnly)
	FVector Location;

	UPROPERTY(BlueprintReadOnly)
	uint8 Layer;

	UPROPERTY(BlueprintReadOnly)
	bool bSuccess;
};

UENUM(BlueprintType)
enum class EWidgetState : uint8
{
	kClear,
	kBlocked
};

USTRUCT(BlueprintType)
struct FSowingWidgetWhisker
{
	GENERATED_BODY()

	FVector Location = FVector::ZeroVector;
	bool bOccupied = false;
};


USTRUCT(BlueprintType)
struct FCoreData
{
	GENERATED_BODY()

	TSoftObjectPtr<ALandscape> Landscape;
	TArray<TArray<TObjectPtr<AGrowable>>> GrowableActorsByLayer;
	TArray<TObjectPtr<AKernel>> Kernels;
	TArray<TObjectPtr<AActor>> Objects;
};


namespace KernelState
{
	enum Type
	{
		kSown,
		kWaiting,
		kSprouted,
		kDead
	};
}

namespace VisualizationMode
{
	enum Type
	{
		kFirst,

		kNone = kFirst,
		kLayers,
		kPlacement,
		kLast
	};
}

/**
 * UCoreManagerSubSystem is a tickable subsystem that supports operations on
 * landscapes, such as sowing and weightmap sampling, and operates based on
 * configuration provided through a UCoreConfig object.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSigntureRestCore);

UCLASS()
class UCoreManagerSubSystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual TStatId GetStatId() const override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void Reset();

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void DebugCreateRandomEvents(const FDateTime date);

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	bool DEBUG_CreateAtDate = false;


	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void InitializeFromConfig(TSoftObjectPtr<ALandscape> InLandscape, UCoreConfig* NewCoreConfig);

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	bool BeginSowing(EHealthDataType HealthDataType, FVector2D ScreenPosition, FCheckResult& CheckResult);

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void EndSowing();

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	bool Sow(FVector2D ScreenPosition, FVector cameraOffset, FSowResult& SowResult);

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void Sprout();

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void SetVisualization(int32 Mode);

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void ExecuteSetVisualization(const TArray<FString>& Args);

	UFUNCTION()
	void ExecuteSetSowingWidgetPointCount(const TArray<FString>& Args);

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void GetCameraTarget(FVector& Target) const;

	TObjectPtr<UCoreConfig> GetConfig() const
	{
		return CoreConfig;
	}

	void RegisterGrowable(TObjectPtr<AGrowable> NewGrowable, int32 LayerIndex);

	UFUNCTION(BlueprintCallable, Category = "Subsystem")
	void GetSowingWidgetInfo(TArray<FSowingWidgetWhisker>& OutWhiskers, FVector& OutCenter, FVector& OutNormal,
	                         float& OutOccupiedPercentage, EWidgetState& OutState) const
	{
		OutWhiskers = SowingWidgetWhiskers;
		OutCenter = SowingWidgetLocation;
		OutNormal = SowingWidgetNormal;
		OutState = SowingWidgetState;
		OutOccupiedPercentage = SowingWidgetOccupiedPercentage;
	}

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	bool ScrubbingCores = false;

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	FDateTime CurrentDateTime;

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	FCoreData CoreData;

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	TMap<FDateTime, FCoreData> LoadedData;

	UPROPERTY(BlueprintAssignable,BlueprintCallable, Category = "Cores")
	FSigntureRestCore OnRestCore; 

	UFUNCTION(BlueprintCallable, Category = "Cores")
	void StartScrubbingCores();

	UFUNCTION(BlueprintCallable, Category = "Cores")
	void StopScrubbingCores();

	UFUNCTION(BlueprintCallable, Category = "Cores")
	bool TryRemoveLastGrowables();
	
	UFUNCTION(BlueprintCallable, Category = "Cores")
	void SetSowingAlreadyOccupied(bool newValue);
protected:
	void CreateMaps(int32 MapResolutionScape);
	void CreateDebugResources();
	bool Trace(FVector2D ScreenPosition,
		FVector cameraOffset,
	           const TFunctionRef<void(const FVector&, const FVector&, int32, int32)>& IntersectionAction) const;
	bool Trace(const FVector& Start, const FVector& End,
	           const TFunctionRef<void(const FVector&, const FVector&, int32, int32)>& IntersectionAction) const;

	static void ValidateConfig(TObjectPtr<UCoreConfig> CoreConfig);
	static bool RasterizeCircle(int32 Width, int32 Height, int32 CenterX, int32 CenterY, int32 Radius,
	                            const TFunctionRef<bool(int32, int32)>& EvaluatePixel);
	bool CheckPlacement(int32 I, int32 J, float Inner);
	void PaintPlacement(int32 I, int32 J, float Outer);

	void UpdateSowingWidget(float dt);
	void DEBUG_DrawSowingWidget();

	// Scale landscape-relative value to map-relative value.
	float GetMapScale() const
	{
		float Fudge = (StaticCast<float>(MapDimX) + 1.0f) / StaticCast<float>(MapDimX);
		Fudge *= StaticCast<float>(MapResolutionScale);
		return Fudge;
	}

	UFUNCTION(BlueprintCallable, Category = "Cores")
	UStaticMesh* GetGrowableStaticMesh() const;

	UFUNCTION(BlueprintCallable, Category = "Cores")
	float GetGrowableBlockingRadius() const;
	
	void GetCurrentCoreDataGrowables(TArray<TObjectPtr<AGrowable>>& allGrowables) const;

	void RemoveGrowableFromCoreData(TObjectPtr<AGrowable> growable);
	//// TSoftObjectPtr<ALandscape> Landscape;


	int32 MapResolutionScale;
	int32 MapDimX;
	int32 MapDimY;

	TArray<TArray<uint8>> LayerMap2D;
	TArray<TArray<uint8>> PlacementMap2D;

	bool bPlacementMapUpdated = false;

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	TObjectPtr<UTexture2D> LayerMapTexture;

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	TObjectPtr<UTexture2D> PlacementMapTexture;

	TObjectPtr<UMaterialInterface> LandscapeStandardMaterial;
	TObjectPtr<UMaterialInterface> LandscapeDebugMaterial;

	UPROPERTY(BlueprintReadWrite, Category = "Cores")
	TObjectPtr<UCoreConfig> CoreConfig;

	bool bUpdateSowingWidget;
	int32 SowingLayer;
	EHealthDataType SowingType;
	TObjectPtr<const FGrowableConfig> SowingGrowableConfig;

	//// TArray<TObjectPtr<AKernel>> Kernels;	
	//// TArray<TArray<TObjectPtr<AGrowable>>> GrowableActorsByLayer;

	// Sowing widget.
	int32 SowingWidgetWhiskerCount = 20;
	TArray<FSowingWidgetWhisker> SowingWidgetWhiskers;
	FVector SowingWidgetLocation;
	FVector SowingWidgetNormal;
	float SowingWidgetOccupiedPercentage = 0.0f;
	EWidgetState SowingWidgetState = EWidgetState::kClear;
	FIntVector2 SowingWidgetIndex;

	// DEBUG: Enables visualization of raytrace bounds.
	TArray<FVector> Starts;
	TArray<FVector> Ends;

	bool bVisualizationEnabled;

	bool bSowingAlreadyOccupied = false;
};

DECLARE_LOG_CATEGORY_EXTERN(LogCoreManager, Display, All);
