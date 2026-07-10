// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "INotifyMature.h"
#include "CoreManagerSubSystem.h"
#include "Core.generated.h"

class UPointLightComponent;
enum class EHealthDataType : uint8;


UCLASS(Blueprintable)
class IOSTEST_API ACore : public AActor, public IINotifyMature
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	explicit ACore(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void NotifyMature_Implementation() override;

protected:

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void Initialize(int32 Layer, EHealthDataType Type, FName GrowableKey,float LightAttackTime = 1.0f, float LightDecayTime = 1.0f);

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<USceneComponent> Scene;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly);
	EHealthDataType Type;
	
private:
	int32 Layer = kNoLayer;

	float LightDecayTime = 1.0f;
	float LightAttackTime = 1.0f;
	float LightTimeRemaining = 0.0f;
	float LightIntensity = 400.0f;
	
	FName GrowableKey;
};
