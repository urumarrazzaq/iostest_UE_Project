// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Action.h"
#include "HealthData.h"
#include "GameFramework/Actor.h"
#include "Growable.generated.h"

class AKernel;
struct FGrowableConfig;
enum class EHealthDataType : uint8;

UCLASS(Blueprintable)
class AGrowable : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGrowable(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<AActor> Kernel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DelayScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FinalScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GrowthTime = 1.0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GrowableRadius = 100.0;
	
	void Initialize(TObjectPtr<UAction_SpawnGrowable> SproutAction, TObjectPtr<UStaticMesh> OverrideMesh);

	UPROPERTY(BlueprintReadOnly)
	bool bSkipGrowth = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EHealthDataType HealthDataType;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};

DECLARE_LOG_CATEGORY_EXTERN(Growable, Display, All);
