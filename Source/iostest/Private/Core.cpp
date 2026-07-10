// Fill out your copyright notice in the Description page of Project Settings.


#include "Core.h"

#include "Action.h"
#include "CoreConfig.h"
#include "CoreManagerSubSystem.h"
#include "Growable.h"
#include "MyMaruCharacter.h"
#include "MyMaruSaveGame.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACore::ACore(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	RootComponent = Scene;
	
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(Scene);

	PointLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PointLight"));
	PointLight->SetupAttachment(StaticMesh);
}

void ACore::Initialize(int32 InLayer, EHealthDataType InType, FName InGrowableKey, float InLightAttackTime, float InLightDecayTime)
{
	Layer = InLayer;
	Type = InType;
	LightDecayTime = InLightDecayTime;
	LightAttackTime = InLightAttackTime;
	GrowableKey = InGrowableKey;

	UE_LOG( Growable, Display, TEXT("Kernel initialized with Layer %d and Type %s"), Layer, *UEnum::GetValueAsName(Type).ToString() );
}

// Called when the game starts or when spawned
void ACore::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACore::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ACore::NotifyMature_Implementation()
{
	LightTimeRemaining = LightDecayTime;
}

