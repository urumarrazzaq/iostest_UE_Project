// Fill out your copyright notice in the Description page of Project Settings.


#include "iostest/Public/AKernel.h"

#include "Action.h"
#include "CoreConfig.h"
#include "CoreManagerSubSystem.h"
#include "Growable.h"
#include "MyMaruCharacter.h"
#include "MyMaruSaveGame.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Slate/SGameLayerManager.h"

// Sets default values
AKernel::AKernel(const FObjectInitializer& ObjectInitializer)
	:
	Super(ObjectInitializer)
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

void AKernel::Initialize(int32 InLayer, EHealthDataType InType, float growableRadius, FName InGrowableKey,
                         float InLightAttackTime, float InLightDecayTime)
{
	Layer = InLayer;
	Type = InType;
	LightDecayTime = InLightDecayTime;
	LightAttackTime = InLightAttackTime;
	State = EKernelState::Sown;
	GrowableKey = InGrowableKey;
	GrowableRadius = growableRadius;

	UE_LOG(Growable, Display, TEXT("Kernel initialized with Layer %d and Type %s"), Layer,
	       *UEnum::GetValueAsName(Type).ToString());
}

// Called when the game starts or when spawned
void AKernel::BeginPlay()
{
	Super::BeginPlay();
}

void AKernel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	AMyMaruCharacter* maru = Cast<AMyMaruCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (maru)
	{
		maru->OnKernelDestroyed();
	}
	else
	{
		UE_LOG(Growable, Warning, TEXT("Kernel destroyed but no maru found!"));
	}
}

// Called every frame
void AKernel::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	switch (State)
	{
	// Kernel has been placed in the world but hasn't sprouted.
	case EKernelState::Sown:
		if (LightTimeRemaining > 0)
		{
			float T = 1.0f - LightTimeRemaining / LightAttackTime;
			PointLight->SetIntensity(LightIntensity * T);
			LightTimeRemaining = FMath::Max(0, LightTimeRemaining - DeltaTime);
		}

		break;

	// Growable is growing from the kernel.
	case EKernelState::Growing:
		break;

	// Growable has finished growing and kernel is dying.
	case EKernelState::Dying:
		LightTimeRemaining -= DeltaTime;
		if (LightTimeRemaining <= 0.0f)
		{
			State = EKernelState::Dead;
		}
		else
		{
			// Dim light over decay time.
			PointLight->SetIntensity(LightIntensity * (LightTimeRemaining / LightDecayTime));
		}

		break;

	case EKernelState::Dead:
		break;
	}
}

void AKernel::Sprout(float Delay)
{
	UE_LOG(Growable, Display, TEXT("Sprouting %s"), *GetName());

	TObjectPtr<UCoreManagerSubSystem> CoreManagerSubSystem = GetWorld()->GetSubsystem<UCoreManagerSubSystem>();

	check(Layer != kNoLayer);

	const TObjectPtr<UCoreConfig> CoreConfig = CoreManagerSubSystem->GetConfig();
	const FLayerConfig& LayerConfig = CoreConfig->Layers[Layer];

	// Fade in the kernel.
	SetActorHiddenInGame(false);
	LightIntensity = PointLight->Intensity;
	PointLight->SetIntensity(0.0f);
	LightTimeRemaining = LightAttackTime;

	// Lookup growable by key.
	if (TObjectPtr<const FGrowableConfig> GrowableConfig = CoreConfig->FindGrowableConfig(GrowableKey))
	{
		TObjectPtr<UAction_SpawnGrowable> SpawnGrowableAction = NewObject<UAction_SpawnGrowable>(GetWorld());

		// Define action.
		SpawnGrowableAction->GrowableKey = GrowableKey;
		SpawnGrowableAction->Location = GetActorLocation();
		SpawnGrowableAction->RandRotation = FMath::RandRange(0.0f, 2.0f * UE_PI);
		SpawnGrowableAction->FinalScale = GrowableConfig->FinalScale + FMath::RandRange(
			-GrowableConfig->FinalScaleRandom, GrowableConfig->FinalScaleRandom);
		SpawnGrowableAction->DelayScale = GrowableConfig->ShouldInstantGrowth
			                                  ? 0.01
			                                  : Delay > 0.0f
			                                  ? Delay
			                                  : GrowableConfig->DelayScale;
		SpawnGrowableAction->GrowthTime = GrowableConfig->ShouldInstantGrowth
			                                  ? 0.01
			                                  : GrowableConfig->GrowthTime * (1.0f + FMath::RandRange(
				                                  -CoreConfig->SpeedVariance, CoreConfig->SpeedVariance));
		SpawnGrowableAction->Layer = Layer;
		SpawnGrowableAction->Kernel = this;
		SpawnGrowableAction->HealthDataType = Type;
		SpawnGrowableAction->GrowableRadius = GrowableRadius;

		AMyMaruCharacter* maruPtr = Cast<AMyMaruCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
		if (maruPtr)
		{
			SpawnGrowableAction->RegisterDate(maruPtr->SavedDate);
		}
		// Execute action.
		SpawnGrowableAction->Execute();

		bool saveSuccess = false;

		AMyMaruCharacter* pMaru = Cast<AMyMaruCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
		if (pMaru)
		{
			pMaru->TriggerSave();
		}
	}
	else
	{
		UE_LOG(Growable, Warning, TEXT("Growable %s not found in config."), *GrowableKey.ToString());
	}

	State = EKernelState::Growing;
}

void AKernel::NotifyMature_Implementation()
{
	LightTimeRemaining = LightDecayTime;
	State = EKernelState::Dying;
}
