// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CoreManagerSubSystem.h"
#include "Action.generated.h"

class UMyMaruSaveGame;
class UMyMaruInventoryComponent;
class AKernel;
enum class EHealthDataType : uint8;

UENUM()
enum class EActionType : uint8
{
	Invalid,
	Sprout,
	Spend,
	Grant,
};

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EActionFlags: uint8
{
	NONE       = 0 UMETA(Hidden),
	Collapse   = 1 << 0,
	ExecInitial    = 1 << 0,
	ExecLoad       = 1 << 0,	
};
ENUM_CLASS_FLAGS(EActionFlags);

// TODO Replace single bool
USTRUCT(BlueprintType)
struct FActionContext
{
	GENERATED_BODY()

	FActionContext(  )
	{
	}
	
	UPROPERTY(BlueprintReadOnly)
	bool RestoreMode = false;
};


UCLASS(Blueprintable)
class UAction : public UObject
{
	GENERATED_BODY()

public:
	UAction();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	int64 ID = -1;

	void RegisterDate( FDateTime InDateTime );

	virtual void Execute(bool = false) {}	

	UFUNCTION(BlueprintCallable, Category = "Action")
	static void DebugDuplicateEvent( UAction *pAction, const int prevDays = 1 );
	
	virtual void Serialize(FArchive& Ar) override;

	virtual void Print();

	UFUNCTION(BlueprintCallable, Category = "Action")	
	virtual FString GetDetails() const;

	UFUNCTION(BlueprintCallable, Category = "Action")	
	virtual EActionFlags GetFlags() const { return EActionFlags::NONE; }

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EActionType Type;
};


UCLASS()
class UAction_SpawnGrowable : public UAction
{
	GENERATED_BODY()

public:
	UAction_SpawnGrowable()
	{
		Kernel = nullptr;
		Type = EActionType::Sprout;
	}
	
	virtual void Print() override;

	virtual EActionFlags GetFlags() const override { return EActionFlags::Collapse | EActionFlags::ExecInitial | EActionFlags::ExecLoad; }
	

	virtual void Execute(bool bRestoreMode = false) override;
	
	virtual void Serialize(FArchive& Ar) override;

	FName GrowableKey;
	FVector Location = FVector::ZeroVector;
	float RandRotation = 0;
	float FinalScale = 1.0f;
	float GrowthTime = 1.0f;
	float DelayScale = 0;
	int32 Layer = -1;
	EHealthDataType HealthDataType;
	float GrowableRadius = 100.0f;
	bool bSkipGrowth = false;
	
	UPROPERTY(Transient)
	TObjectPtr<AKernel> Kernel;
};

UCLASS()
class UAction_Transact : public UAction
{
	GENERATED_BODY()

public:
	virtual void Print() override;

	virtual EActionFlags GetFlags() const override { return EActionFlags::ExecInitial; }

protected:

	virtual void Serialize(FArchive& Ar) override;
	
	TObjectPtr<UMyMaruInventoryComponent> GetInventory();

public:	
	EHealthDataType HealthDataType;
	int32 Amount;
};

UCLASS()
class UAction_Spend : public UAction_Transact
{
	GENERATED_BODY()

public:
	UAction_Spend()
	{
		Type = EActionType::Spend;
	}

	virtual EActionFlags GetFlags() const override { return EActionFlags::Collapse | EActionFlags::ExecInitial; }

	
	virtual void Execute(bool = false) override;
};

UCLASS()
class UAction_Grant : public UAction_Transact
{
	GENERATED_BODY()

public:
	UAction_Grant()
	{
		Type = EActionType::Grant;
	}


	virtual void Execute(bool = false) override;
};

void ActionInitialize(TObjectPtr<UMyMaruSaveGame> SaveGame, TObjectPtr<APawn> PlayerPawn);

