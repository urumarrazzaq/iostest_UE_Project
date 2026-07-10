// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreManagerSubSystem.h"
#include "Engine/DataAsset.h"
#include "Growable.h"
#include "HealthData.h"
#include "CoreConfig.generated.h"

enum class EHealthDataType : uint8;

USTRUCT(BlueprintType)
struct FLandscapeConfig
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName MaterialParameterName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ParameterEnhanceValue;
};

USTRUCT(BlueprintType)
struct FLandscapeMaterialParameters
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	class UMaterialParameterCollection* MaterialParameterCollection;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FLandscapeConfig> MaterialParameters;
};

USTRUCT(BlueprintType)
struct FGrowableConfig
{
	GENERATED_BODY()

	FGrowableConfig() = default;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AGrowable> GrowableClass = AGrowable::StaticClass();

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere)
	float DelayScale = 0.0f;

	UPROPERTY(EditAnywhere)
	bool ShouldInstantGrowth = false;

	UPROPERTY(EditAnywhere, meta=(EditCondition="!ShouldInstantGrowth"))
	float GrowthTime = 8.0f;

	UPROPERTY(EditAnywhere)
	float FinalScale = 1.0f;

	UPROPERTY(EditAnywhere)
	float FinalScaleRandom = 0.0f;

	// Inner radius will be scaled-down outer radius.
	constexpr static float kInnerRadiusScale = 0.75f;

	UPROPERTY(EditAnywhere)
	float InnerRadius = 0.0f;

	UPROPERTY(EditAnywhere)
	float OuterRadius = 0.0f;

	UPROPERTY(EditAnywhere)
	float BlockingRadius = 0.0f;

	UPROPERTY(EditAnywhere)
	float InnerRadiusScaled = 0.0f;

	UPROPERTY(EditAnywhere)
	float OuterRadiusScaled = 0.0f;

	FName Key;

	float GetInnerRadius() const
	{
		return InnerRadius;
	}

	float GetInnerRadiusScaled() const
	{
		return InnerRadiusScaled;
	}

	float GetOuterRadius() const
	{
		return OuterRadius;
	}

	float GetOuterRadiusScaled() const
	{
		return OuterRadiusScaled;
	}

	float GetBlockingRadius() const
	{
		return BlockingRadius;
	}

	void CalculateBounds(float LandscapeScale)
	{
		// Get the X and Y edges scaled by the worst-case final scale.
		float LengthX = (StaticMesh->GetBoundingBox().Max.X - StaticMesh->GetBoundingBox().Min.X) * (FinalScale +
			FinalScaleRandom);
		float LengthY = (StaticMesh->GetBoundingBox().Max.Y - StaticMesh->GetBoundingBox().Min.Y) * (FinalScale +
			FinalScaleRandom);

		// Check if we have a XY footprint that can't be well-decribed by a circle.
		float DimensionRatio = FMath::Abs(1.0f - LengthX / LengthY);
		if (DimensionRatio > 0.5f)
		{
			UE_LOG(LogCoreManager, Warning,
			       TEXT("The StaticMesh %s has a bounding box footprint which can't be well-described by a circle!\n"),
			       *StaticMesh->GetName());
		}

		float InvLandscapeScale = 1.f / LandscapeScale;

		// If the inner radius is set to 0, we need to calculate it based on the static mesh.		
		if (InnerRadius == 0.0f)
		{
			UE_LOG(LogCoreManager, Warning,
			       TEXT("The inner radius is set to 0.0f for %s. This will be calculated based on the static mesh."),
			       *Key.ToString());
			OuterRadius = FMath::Min(LengthX, LengthY) / 2.0f;
			OuterRadiusScaled = OuterRadius * InvLandscapeScale;
			InnerRadius = OuterRadius * kInnerRadiusScale;
			InnerRadiusScaled = InnerRadius * InvLandscapeScale;
		}
		else
		{
			// If the inner radius is set, we need to calculate the outer radius based on the static mesh.
			OuterRadius = FMath::Max(LengthX, LengthY) / 2.0f;
			OuterRadiusScaled = OuterRadius * InvLandscapeScale;
			InnerRadiusScaled = InnerRadius * InvLandscapeScale;
		}
	}
};

USTRUCT(BlueprintType)
struct FEnhancedGrowableConfig
{
	GENERATED_BODY()
	;

	FEnhancedGrowableConfig()
	{
		AsArray.Add(&Red);
		AsArray.Add(&Green);
		AsArray.Add(&Blue);
		AsArray.Add(&Yellow);
	}

	const TArray<FGrowableConfig>& GetGrowables(EHealthDataType HealthDataType) const
	{
		switch (HealthDataType)
		{
		case EHealthDataType::MoveCalories:
			return Red;
		case EHealthDataType::ExerciseMinutes:
			return Green;
		case EHealthDataType::StandHoursCount:
			return Blue;
		case EHealthDataType::SocialEngagement:
			return Yellow;
		}
		//TODO: it should be returning empty array
		return Yellow;
	}

	const FGrowableConfig* GetRandomGrowable(EHealthDataType HealthDataType) const
	{
		const FGrowableConfig* RandomGrowable = nullptr;
		const TArray<FGrowableConfig>& Growables = GetGrowables(HealthDataType);
		if (Growables.Num() > 0)
		{
			RandomGrowable = &Growables[FMath::RandRange(0, Growables.Num() - 1)];
		}

		return RandomGrowable;
	}

	UPROPERTY(EditAnywhere)
	TArray<FGrowableConfig> Red;

	UPROPERTY(EditAnywhere)
	TArray<FGrowableConfig> Green;

	UPROPERTY(EditAnywhere)
	TArray<FGrowableConfig> Blue;

	UPROPERTY(EditAnywhere)
	TArray<FGrowableConfig> Yellow;

	TArray<TArray<FGrowableConfig>*> AsArray;
};

USTRUCT(BlueprintType)
struct FLayerConfig
{
	GENERATED_BODY()

	void Initialize();

	/* Layer name. NOTE: Must correspond to a LandscapeLayer Name. */
	UPROPERTY(EditAnywhere)
	FString Name;

	/* Radius used to check if placement is possible. */
	UPROPERTY(meta=(HideInDetailPanel))
	float InnerRadius = 50.f;

	/* Radius used to set placement range. */
	UPROPERTY(meta=(HideInDetailPanel))
	float OuterRadius = 100.f;

	float InnerRadiusScaled;
	float OuterRadiusScaled;

	/* Growables supported by layer. */
	UPROPERTY(EditAnywhere)
	FEnhancedGrowableConfig EnhancedGrowables;

	const FGrowableConfig* GetRandomGrowable(EHealthDataType HealthDataType) const
	{
		return EnhancedGrowables.GetRandomGrowable(HealthDataType);
	}
};

/**
 * 	
 */
UCLASS(BlueprintType, Const)
class UCoreConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	void Initialize(float LandscapeScale);

	/* Scaling factor for layer and placement maps. */
	UPROPERTY(EditAnywhere)
	int32 MapResolutionScale;

	/* BP class for kernels. */
	UPROPERTY(EditAnywhere, Category = "Kernels")
	TObjectPtr<UClass> KernelClass;

	/* Time for kernel light to decay after growth is complete. */
	UPROPERTY(EditAnywhere, Category = "Kernels")
	float LightDecayTime = 1.0f;

	/* Time for kernel light to attack after growth begins. */
	UPROPERTY(EditAnywhere, Category = "Kernels")
	float LightAttackTime = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Growables")
	float SproutDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Growables")
	float SpeedVariance = .25f;

	/* Layer configurations. */
	UPROPERTY(EditAnywhere)
	TArray<FLayerConfig> Layers;

	/* Growable Cost configurations. */
	UPROPERTY(EditAnywhere)
	TMap<EHealthDataType, float> GrowableGoal;

	/* Growable Cost configurations. */
	UPROPERTY(EditAnywhere)
	TMap<EHealthDataType, float> GrowableCost;

	/* Each Growable represent single parameter value */
	UPROPERTY(EditAnywhere)
	TMap<EHealthDataType, FLandscapeMaterialParameters> LandscapeEnhanceValues;

	UFUNCTION(BlueprintCallable)
	TMap<EHealthDataType, float> GetGrowableGoal() const { return GrowableGoal; }

	UFUNCTION(BlueprintCallable)
	TMap<EHealthDataType, float> GetGrowableCost() const { return GrowableCost; }

	UFUNCTION(BlueprintCallable)
	TMap<EHealthDataType, FLandscapeMaterialParameters> GetLandscapeEnhanceValues() const
	{
		return LandscapeEnhanceValues;
	}

	TObjectPtr<const FGrowableConfig> FindGrowableConfig(FName Key)
	{
		return GrowablesLookup.FindRef(Key);
	}

private:
	TMap<FName, TObjectPtr<const FGrowableConfig>> GrowablesLookup;
};
