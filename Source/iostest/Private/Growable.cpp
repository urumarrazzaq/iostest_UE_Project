// Fill out your copyright notice in the Description page of Project Settings.


#include "Growable.h"

#include "AKernel.h"
#include "CoreConfig.h"

// Sets default values
AGrowable::AGrowable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
}

void AGrowable::Initialize(TObjectPtr<UAction_SpawnGrowable> SpawnGrowableAction, TObjectPtr<UStaticMesh> DefaultMesh)
{
	Kernel = SpawnGrowableAction->Kernel;

	// If this doesn't have a mesh specified, use the default one.
	if (!Mesh->GetStaticMesh())
	{
		Mesh->SetStaticMesh(DefaultMesh);
	}
	
	DelayScale = SpawnGrowableAction->DelayScale;
	FinalScale = SpawnGrowableAction->FinalScale;
	GrowthTime = SpawnGrowableAction->GrowthTime;
	bSkipGrowth = SpawnGrowableAction->bSkipGrowth;
	HealthDataType = SpawnGrowableAction->HealthDataType;
	GrowableRadius = SpawnGrowableAction->GrowableRadius;

}

// Called when the game starts or when spawned
void AGrowable::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AGrowable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}


DEFINE_LOG_CATEGORY( Growable );
