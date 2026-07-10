// Fill out your copyright notice in the Description page of Project Settings.

#include "CoreManagerSubSystem.h"

#include "AKernel.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeComponent.h"
#include "Landscape.h"
#include "LandscapeLayerInfoObject.h"
#include "CoreConfig.h"
#include "ImageUtils.h"
#include "MyMaruCharacter.h"
#include "MyMaruInventoryComponent.h"
#include "Math/UnrealMathUtility.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"


void UCoreManagerSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    IConsoleManager::Get().RegisterConsoleCommand(TEXT("CoreManager.SetVisualization"),
                                                  TEXT("Core Manager debug visualization mode."),
                                                  FConsoleCommandWithArgsDelegate::CreateUObject(this, &UCoreManagerSubSystem::ExecuteSetVisualization));

    IConsoleManager::Get().RegisterConsoleCommand(TEXT("CoreManager.SetSowingWidget"),
                                                   TEXT("Number of points in the sowing widget."),
                                                  FConsoleCommandWithArgsDelegate::CreateUObject(this, &UCoreManagerSubSystem::ExecuteSetSowingWidgetPointCount));
   
    bUpdateSowingWidget = false;

		CurrentDateTime = FDateTime::MinValue();

}

void UCoreManagerSubSystem::Deinitialize()
{
    Super::Deinitialize();
}

TStatId UCoreManagerSubSystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCoreManagerSubSystem, STATGROUP_Tickables);
    
}

void UCoreManagerSubSystem::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

		// UE_LOG(LogCoreManager, Display, TEXT("Core Manager Subsystem Tick"));

    // Clean up "dead" kernels.
    for (TObjectPtr<AKernel> Kernel : CoreData.Kernels)
    {
        if (Kernel->GetState() == EKernelState::Dead)
        {
            GetWorld()->DestroyActor(Kernel);
        }        
    }
   
		// UE_LOG(LogCoreManager, Display, TEXT("Core Manager Subsystem Tick - After Cleanup"));
		
    CoreData.Kernels.RemoveAll([](const TObjectPtr<AKernel> Kernel) -> bool  
    {
        return Kernel->IsPendingKillPending();
    });

		// UE_LOG(LogCoreManager, Display, TEXT("Core Manager Subsystem Tick - After RemoveAll"));

    for (auto GrowableActors : CoreData.GrowableActorsByLayer)
    {
        GrowableActors.RemoveAll([](const TObjectPtr<AGrowable> GrowableActor) -> bool
        {
            return GrowableActor->IsPendingKillPending();
        });
    }

		// UE_LOG(LogCoreManager, Display, TEXT("Core Manager Subsystem Tick - After RemoveAll in GrowableActorsByLayer"));
    
    if (bUpdateSowingWidget)
    {
        UpdateSowingWidget(DeltaTime);
        //DEBUG_DrawSowingWidget();
    }

		//UE_LOG(LogCoreManager, Display, TEXT("Core Manager Subsystem Tick - After UpdateSowingWidget"));
    
#if 0
    // Visualize layer map trace.
    int32 NumRays = MapDimX * MapDimY;
    for (int32 i = 0; i < NumRays; i++)
    {
        DrawDebugSphere(GetWorld(), Starts[i], 10.0f, 10, FColor::Yellow);
        DrawDebugSphere(GetWorld(), Ends[i], 10.0f, 10, FColor::Yellow);
    }
#endif 
    
    // Update the placement map texture.
    if (bPlacementMapUpdated)
    {
			UE_LOG( LogCoreManager, Display,  TEXT("Updating placement map texture.") );

        FColor* Texels = StaticCast<FColor*>(PlacementMapTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

				if( Texels )
				{
					// Unwrap 2D array into 1D.
					for (int32 I = 0; I < PlacementMap2D.Num(); ++I)
					{
							for (int32 J = 0; J < PlacementMap2D[I].Num(); ++J)
							{
									Texels[J * PlacementMap2D[I].Num() + I] = PlacementMap2D[I][J] ? FColor::White : FColor::Black;
							}
					}
				}
				else
				{
					UE_LOG(LogCoreManager, Warning, TEXT("Failed to lock the texture mipmap data."));
				}

        
        PlacementMapTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
        PlacementMapTexture->UpdateResource();
 
        bPlacementMapUpdated = false;
    }
}

void UCoreManagerSubSystem::Reset()
{
	UE_LOG( LogCoreManager, Display, TEXT("Resetting Core Manager Subsystem") );

	int layer = 0;
    for (auto& GrowableActors : CoreData.GrowableActorsByLayer)
    {
			UE_LOG( LogCoreManager, Display, TEXT("Resetting layer %d"), layer++ );
        for (auto& GrowableActor : GrowableActors)
        {
            if (GrowableActor)
            {
							UE_LOG( LogCoreManager, Display, TEXT("Resetting GrowableActor %s"), *GrowableActor->GetName() );
							GrowableActor->Destroy();
            }
						else
						{
							UE_LOG( LogCoreManager, Display, TEXT("GrowableActor is null") );
						}
        }

				GrowableActors.Empty();
    }

    for (auto& Kernel : CoreData.Kernels)
		{
			Kernel->Destroy();
		}

		CoreData.Kernels.Empty();
    AMyMaruCharacter* maruPtr = Cast<AMyMaruCharacter>( UGameplayStatics::GetPlayerPawn(GetWorld(),0));
    if (maruPtr)
    {
        AMyMaruCharacter& maru = *maruPtr;
        maru.Inventory->GetBank()->Initialize();
    }
    OnRestCore.Broadcast();
}

void UCoreManagerSubSystem::DebugCreateRandomEvents( const FDateTime date )
{
	UE_LOG( LogCoreManager, Display, TEXT("DebugCreateRandomEvents") );
	// TODO MH: This is a placeholder for the actual event creation logic.
	// This function should be implemented to create random events based on the provided date.
	// For now, we will just log the date.
	UE_LOG( LogCoreManager, Display, TEXT("Creating random events for date: %s"), *date.ToString() );

	// TODO MH: Add logic to create random events here.
	
}

constexpr float kSowingWidgetGroundOffset = 30.0f;

// This is a footgun. The 2.25 is copied from the static mesh in BP_SowingWidget!
constexpr float kSowingWidgetXYScale = 2.25f;

void UCoreManagerSubSystem::UpdateSowingWidget(float dt)
{
    SowingWidgetWhiskers.Empty();

		// UE_LOG( LogCoreManager, Display, TEXT("UpdateSowingWidget"));

    FBox BoundingBox = CoreData.Landscape->GetComponentsBoundingBox().ExpandBy(1.2f);

		if( !SowingGrowableConfig )
		{
			UE_LOG( LogCoreManager, Error, TEXT("SowingGrowableConfig is not set!"));
			return;
		}

    FVector Radius(SowingGrowableConfig->GetInnerRadius(), 0, 0);

    SowingWidgetNormal   = FVector::ZeroVector;

		// TODO MH Refactor this to be better with floating point
    for (float Angle = 0; Angle < (2.0f * UE_PI); Angle += (2.0 * PI) / StaticCast<float>(SowingWidgetWhiskerCount))
    {
        FQuat Rotation(FVector(0, 0, 1.0f), Angle);

        // Layer inner radius whiskers to capture occupied state.
        FVector SowingWidgetWhiskerLocation = SowingWidgetLocation + Rotation.RotateVector(Radius);
        
        FVector Start= SowingWidgetWhiskerLocation + FVector(0.0f, 0.0f, BoundingBox.Max.Z);
        FVector End = SowingWidgetWhiskerLocation + FVector(0.0f, 0.0f, BoundingBox.Min.Z);
    
				// UE_LOG( LogCoreManager, Display, TEXT("SowingWidgetWhiskerLocation: %s"), *SowingWidgetWhiskerLocation.ToString() );


        Trace(Start, End, [this](const FVector& Location, const FVector& Normal,int32 I, int32 J) -> bool
        {
					if( I < 0 || I >= MapDimX || J < 0 || J >= MapDimY )
					{
						UE_LOG( LogCoreManager, Display, TEXT("Invalid map indices: I=%d, J=%d"), I, J );
						return false;
					}

					const auto mapRaw = PlacementMap2D[I][J];

            bool bOccupied = StaticCast<bool>( mapRaw );    
            uint8 ThisLayer = LayerMap2D[I][J];            
            if (ThisLayer == SowingLayer)
            {
                SowingWidgetWhiskers.Add(FSowingWidgetWhisker(Location, bOccupied));
            }
            return true;
        });

				// UE_LOG( LogCoreManager, Display, TEXT("Whisker Traces done") );



        // Wider whiskers to generate a smooth normal for the widget.
        SowingWidgetWhiskerLocation = SowingWidgetLocation + Rotation.RotateVector(Radius * kSowingWidgetXYScale);

        Start = SowingWidgetWhiskerLocation + FVector(0.0f, 0.0f, BoundingBox.Max.Z);
        End = SowingWidgetWhiskerLocation + FVector(0.0f, 0.0f, BoundingBox.Min.Z);
        
        Trace(Start, End, [this](const FVector& Location, const FVector& Normal,int32 I, int32 J) -> bool
        {
            SowingWidgetNormal += Normal;

            return true;
        });        
    }

    // Calculate the degree to which the widget is clear.
    SowingWidgetOccupiedPercentage = 0;
    for (const auto& Whisker : SowingWidgetWhiskers)
        SowingWidgetOccupiedPercentage += Whisker.bOccupied ? 0.0f : FMath::Square(1.0f / SowingWidgetWhiskers.Num());
    
    // Average normal.
    SowingWidgetNormal /= SowingWidgetWhiskerCount;

    // Offset location from ground.
    SowingWidgetLocation += SowingWidgetNormal * kSowingWidgetGroundOffset;
}

void UCoreManagerSubSystem::DEBUG_DrawSowingWidget()
{
    bool bAnyOccupied = false;
    
    constexpr float kSowingWhiskerRadius = 15.0f;
    FVector kSowingWidgetLocation = FVector::ZeroVector;
    
    for (const auto& Whisker : SowingWidgetWhiskers)
    {
        kSowingWidgetLocation = Whisker.Location + FVector(0, 0, kSowingWhiskerRadius);

        FColor Color = Whisker.bOccupied ? FColor::Red : FColor::Green;
        DrawDebugSphere(GetWorld(), kSowingWidgetLocation, kSowingWhiskerRadius, 10, Color);
    }
    
    DrawDebugSphere(GetWorld(), SowingWidgetLocation, SowingGrowableConfig->GetInnerRadius(), 20, FColor::White);

    DrawDebugLine(GetWorld(), SowingWidgetLocation, SowingWidgetLocation + SowingWidgetNormal * 1000.0f, FColor::Red);
}

UStaticMesh* UCoreManagerSubSystem::GetGrowableStaticMesh() const
{
    return SowingGrowableConfig ? SowingGrowableConfig->StaticMesh : nullptr;
}

float UCoreManagerSubSystem::GetGrowableBlockingRadius() const
{
    return SowingGrowableConfig ? SowingGrowableConfig->BlockingRadius : 50.f;
}

void UCoreManagerSubSystem::GetCurrentCoreDataGrowables(TArray<TObjectPtr<AGrowable>>& allGrowables) const
{
    for (auto& growableActors : CoreData.GrowableActorsByLayer)
    {
        for (auto& growableActor : growableActors)
        {
            allGrowables.Emplace(*growableActor);
        }
    }
}

void UCoreManagerSubSystem::RemoveGrowableFromCoreData(TObjectPtr<AGrowable> growable)
{
    for (auto& growableActors : CoreData.GrowableActorsByLayer)
    {
        if (!growableActors.IsEmpty() && growableActors.Contains(growable))
        {
            growableActors.Remove(growable);
            return;
        }
    }
}

bool UCoreManagerSubSystem::TryRemoveLastGrowables()
{
    TArray<TObjectPtr<AGrowable>> allGrowables;
    GetCurrentCoreDataGrowables(allGrowables);
    if (allGrowables.IsEmpty())
    {
        return false;
    }
    int32 lastGrowableIndex = allGrowables.Num() - 1;
    RemoveGrowableFromCoreData(allGrowables[lastGrowableIndex]);
    allGrowables[lastGrowableIndex]->Destroy();
    return true;
}

void UCoreManagerSubSystem::SetSowingAlreadyOccupied(bool newValue)
{
    bSowingAlreadyOccupied = newValue;
}

void UCoreManagerSubSystem::InitializeFromConfig(TSoftObjectPtr<ALandscape> InLandscape, UCoreConfig* NewCoreConfig)
{
    check(InLandscape);
    CoreData.Landscape = InLandscape;
    
    check(NewCoreConfig);
    CoreConfig = NewCoreConfig;

    CoreConfig->Initialize(CoreData.Landscape->GetTransform().GetScale3D().X);
    ValidateConfig(CoreConfig);
    
    CoreData.GrowableActorsByLayer.AddDefaulted(CoreConfig->Layers.Num());
    
    // Generate a hires texture where each texel is the dominant layer into. This is a big optimization for selecting and visualizing layers.     
    CreateMaps(CoreConfig->MapResolutionScale);
}

LandscapeSampler::LandscapeSampler(TArray<uint8>& InTarget, TObjectPtr<ULandscapeComponent> InLandscapeComponent) : Target(InTarget), LandscapeComponent(InLandscapeComponent)
{
    Stride =  (LandscapeComponent->SubsectionSizeQuads + 1) * LandscapeComponent->NumSubsections;
}

float LandscapeSampler::Sample(FVector Coordinates)
{
    int32 X1 = FMath::FloorToInt32(Coordinates.X);
    int32 Y1 = FMath::FloorToInt32(Coordinates.Y);
    int32 X2 = FMath::CeilToInt32(Coordinates.X);
    int32 Y2 = FMath::CeilToInt32(Coordinates.Y);

    // Min is to prevent the sampling of the final column from overflowing
    int32 IdxX1 = FMath::Min<int32>(((X1 / LandscapeComponent->SubsectionSizeQuads) * (LandscapeComponent->SubsectionSizeQuads + 1)) + (X1 % LandscapeComponent->SubsectionSizeQuads), Stride - 1);
    int32 IdxY1 = FMath::Min<int32>(((Y1 / LandscapeComponent->SubsectionSizeQuads) * (LandscapeComponent->SubsectionSizeQuads + 1)) + (Y1 % LandscapeComponent->SubsectionSizeQuads), Stride - 1);
    int32 IdxX2 = FMath::Min<int32>(((X2 / LandscapeComponent->SubsectionSizeQuads) * (LandscapeComponent->SubsectionSizeQuads + 1)) + (X2 % LandscapeComponent->SubsectionSizeQuads), Stride - 1);
    int32 IdxY2 = FMath::Min<int32>(((Y2 / LandscapeComponent->SubsectionSizeQuads) * (LandscapeComponent->SubsectionSizeQuads + 1)) + (Y2 % LandscapeComponent->SubsectionSizeQuads), Stride - 1);

        
    // Need to convert to [0-1] for bilinearly interpolating weight.
    float Sample11 = (float)(Target[IdxX1 + Stride * IdxY1]) / 255.0f;
    float Sample21 = (float)(Target[IdxX2 + Stride * IdxY1]) / 255.0f;
    float Sample12 = (float)(Target[IdxX1 + Stride * IdxY2]) / 255.0f;
    float Sample22 = (float)(Target[IdxX2 + Stride * IdxY2]) / 255.0f;

    float LerpX = FMath::Fractional(static_cast<float>(Coordinates.X));
    float LerpY = FMath::Fractional(static_cast<float>(Coordinates.Y));

    // Bilinear interpolate
    return FMath::Lerp(
        FMath::Lerp(Sample11, Sample21, LerpX),
        FMath::Lerp(Sample12, Sample22, LerpX),
        LerpY);
}

void UCoreManagerSubSystem::StartScrubbingCores()
{
	ScrubbingCores = true;
}

void UCoreManagerSubSystem::StopScrubbingCores()
{
	ScrubbingCores = false;
}



void UCoreManagerSubSystem::CreateMaps(int32 InMapResolutionScale)
{
    TMap<FString, TArray<uint8>> LayerCache;
    TArray<FString> LayerNames;

    // Order of Layer configs will determine layer ids.
    for (const FLayerConfig& LayerConfig : CoreConfig->Layers)
    {
        LayerNames.Add(LayerConfig.Name);
    }
    
    // Traces landscape weight maps to determine the dominant layer at a given intersection point.
    auto GetDominantLayer = [this, &LayerCache, &LayerNames](FVector Start, FVector End) -> int32
    {
        uint8 DominantLayer = kNoLayer;
        
        FCollisionQueryParams QueryParams;
        FCollisionResponseParams ResponseParams(ECR_Overlap);
        TArray<FHitResult> HitResults;
        GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECollisionChannel::ECC_WorldStatic, QueryParams, ResponseParams);

        for (const FHitResult& Hit : HitResults)
        {
            if (ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.GetComponent()))
            {
                if (const TObjectPtr<ULandscapeComponent> HitLandscape = HitLandscapeCollision->GetRenderComponent())
                {
                    float MaxWeight = 0.0f;
                    for (FWeightmapLayerAllocationInfo WeightmapLayerAllocation : HitLandscape->GetCurrentRuntimeWeightmapLayerAllocations())
                    {
                        FString Key = HitLandscape->GetName() + WeightmapLayerAllocation.LayerInfo->LayerName.ToString();
                        TArray<uint8>* Texels = LayerCache.Find(Key);
                        if (!Texels)
                        {
                            UE_LOG(LogCoreManager, Display, TEXT("    %s"), *Key);
                            Texels = &LayerCache.Add(Key);

                            const TArray<UTexture2D*> WeightmapTextures = GetWorld()->GetFeatureLevel() == ERHIFeatureLevel::ES3_1 ? HitLandscape->MobileWeightmapTextures : HitLandscape->GetWeightmapTextures();
                            FTexture2DMipMap& TopMip = WeightmapTextures[WeightmapLayerAllocation.WeightmapTextureIndex]->GetPlatformData()->Mips[0];
                            
                            uint8* PackedTexels = (uint8*) TopMip.BulkData.Lock(LOCK_READ_ONLY);
                            {
                                // This layer's weights are packed into a specific RGBA channel. We'll offset the array start based on the channel then cache it for fast retrieval later. 
                                int32 ChannelOffsets[4] = { (int32)STRUCT_OFFSET(FColor, R), (int32)STRUCT_OFFSET(FColor, G), (int32)STRUCT_OFFSET(FColor, B), (int32)STRUCT_OFFSET(FColor, A) };
                                const uint8* SrcTextureData = (const uint8*) PackedTexels + ChannelOffsets[WeightmapLayerAllocation.WeightmapTextureChannel];
                
                                int32 Stride = sizeof(FColor);

                                // Copy channel texels into cache.
                                int32 NumTexels = FMath::Square((HitLandscape->SubsectionSizeQuads + 1) * HitLandscape->NumSubsections);
                                Texels->Reserve(NumTexels);
                                for (int32 i = 0; i < NumTexels; ++i)
                                {
                                    Texels->Add(SrcTextureData[i * Stride]);
                                }
                            }                
                            TopMip.BulkData.Unlock();                            
                        }
                        
                        // Sample the weightmap at the hit location.
                        FVector LayerLocation = HitLandscape->GetComponentToWorld().InverseTransformPosition(Hit.Location);

                        LandscapeSampler Sampler(*Texels, HitLandscape);
                        float Sample = Sampler.Sample(LayerLocation);

                        if (Sample > MaxWeight)
                        {
                            MaxWeight = Sample;
                            TArray<FString>::SizeType DominantLayerIndex;
                            if (LayerNames.Find(WeightmapLayerAllocation.LayerInfo->LayerName.ToString(), DominantLayerIndex))
                            {
                                DominantLayer = DominantLayerIndex;
                            }
                            else
                            {
                                ensureMsgf(false, TEXT("Weightmap layer %s does not have a corresponding LayerConfig! Please add one."), *WeightmapLayerAllocation.LayerInfo->LayerName.ToString());                            
                            }
                        }
                    }
                }
            }
        }
        
        return DominantLayer;
    };

    FBox BoundingBox = CoreData.Landscape->GetComponentsBoundingBox();
    FVector LandscapeSizeWS = BoundingBox.GetSize();

    // Need the number of landscape components in X and Y. To do this, we'll get divide the world-space landscape dimensions by those of a single component (they're all the same). 
    TObjectPtr<ULandscapeComponent> TestLandscapeComponent = CoreData.Landscape->GetComponentByClass<ULandscapeComponent>();
    FVector LandscapeComponentSize = TestLandscapeComponent->GetLocalBounds().BoxExtent * 2.0f;
    FVector LandscapeComponentScale = TestLandscapeComponent->GetComponentTransform().GetScale3D();
    FVector LandscapeComponentSizeWS = LandscapeComponentScale * LandscapeComponentSize;  
    FVector Dims = LandscapeSizeWS / LandscapeComponentSizeWS * CoreData.Landscape->SubsectionSizeQuads * CoreData.Landscape->NumSubsections; 

    // Save these for later use.
    MapResolutionScale = InMapResolutionScale;
    MapDimX = FMath::FloorToInt32(Dims.X) * MapResolutionScale;
    MapDimY = FMath::FloorToInt32(Dims.Y) * MapResolutionScale;

    UE_LOG(LogCoreManager, Display, TEXT("Landscape map dimensions: %d x %d"), MapDimX, MapDimY);

    auto CreateMap2D = [](TArray<TArray<uint8>>& InMap, int32 InWidth, int32 InHeight) -> void
    {
        InMap.SetNum(InWidth);

        for (int32 i = 0; i < InWidth; ++i)
        {
            InMap[i].SetNumZeroed(InHeight);
        }
    };
    
    CreateMap2D(LayerMap2D, MapDimX, MapDimY);
    CreateMap2D(PlacementMap2D, MapDimX, MapDimY);
    
    // Generate rays along a grid representing the desired resolution.
    float OO_XDim = 1.0f / MapDimX;
    float OO_YDim = 1.0f / MapDimY;
        
    for (float Y = .5f * OO_YDim; Y < 1.0f; Y += (1.0f * OO_YDim))
    {
        for (float X = .5f * OO_XDim; X < 1.0f; X += (1.0f * OO_XDim))
        {
            FVector Diagonal = BoundingBox.Max - BoundingBox.Min;
            FVector Start = BoundingBox.Min + Diagonal * FVector(X, Y, 1.2f);
            FVector End = BoundingBox.Min + Diagonal * FVector(X, Y, -1.2f);

            Starts.Add(Start);
            Ends.Add(End);
            
            int32 DominantLayer = GetDominantLayer(Start, End);
            int32 I = FMath::FloorToInt32(X * MapDimX);
            int32 J = FMath::FloorToInt32(Y * MapDimY);
            
            LayerMap2D[I][J] = DominantLayer;
        }
    }

    // Create debug.
    CreateDebugResources();
}

void UCoreManagerSubSystem::CreateDebugResources()
{
    auto CreateDebugTexture = [this](const TArray<TArray<uint8>>& Map, const TArray<FColor>& Remap, const FString& Filename, bool bMakePowerOfTwo = true) -> UTexture2D*
    {
        TArray<FColor> DebugColors;
        
        // Generate texture we can use for debug visualization.
        FImage Image(MapDimX, MapDimY, 1, ERawImageFormat::Type::BGRA8, EGammaSpace::sRGB);

        for (int32 J = 0; J < MapDimY; ++J)
        {
            for (int32 I = 0; I < MapDimX; ++I)
            {
                uint8 Texel = Map[I][J];
                
                FColor Color = Texel < Remap.Num() ? Remap[Texel] : FColor::White;
                FColor* DestPixel = StaticCast<FColor*>(Image.GetPixelPointer(I, J));
                *DestPixel = Color;
            }
        }
        
        FImageCore::ResizeImageInPlace(Image,
                                       bMakePowerOfTwo ? FMath::RoundUpToPowerOfTwo(MapDimX) : MapDimX,
                                       bMakePowerOfTwo ? FMath::RoundUpToPowerOfTwo(MapDimY) : MapDimY,
                                       ERawImageFormat::Type::BGRA8,
                                       EGammaSpace::sRGB,
                                       FImageCore::EResizeImageFilter::PointSample);

        // FString Pathname = FString(TEXT("~/Downloads")) / Filename;
        // FImageUtils::SaveImageByExtension(*Pathname, Image);
        
        TObjectPtr<UTexture2D> NewTexture = FImageUtils::CreateTexture2DFromImage(Image);
        NewTexture->Filter = TF_Nearest;
        NewTexture->UpdateResource();
        return NewTexture;
    };

    TArray<FColor> LayerMapRemap = {FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow, FColor::Magenta, FColor::Cyan};
    LayerMapTexture = CreateDebugTexture(LayerMap2D, LayerMapRemap, "LayerMap.tga");

    TArray<FColor> PlacementMapRemap = {FColor::Black, FColor::White};
    PlacementMapTexture = CreateDebugTexture(PlacementMap2D, PlacementMapRemap, "PlacementMap.tga", false);

    LandscapeStandardMaterial = Cast<UMaterialInstance>(CoreData.Landscape->GetLandscapeMaterial());
    LandscapeDebugMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Game/SoStylized/Environment/Landscape/Materials/M_LandscapeDebug.M_LandscapeDebug"));
}

bool UCoreManagerSubSystem::RasterizeCircle(int32 Width, int32 Height, int32 CenterX, int32 CenterY, int32 Radius, const TFunctionRef<bool(int32, int32)>& EvaluatePixel)
{
    int32 x = 0;
    int32 y = Radius;
    int32 DecisionOver2 = 1 - Radius;
    
    auto FillRow = [&](int32 Y, int32 X1, int32 X2) -> bool
    {
        bool bContinue = true;
        for (int32 X = FMath::Max(0, X1); bContinue && X <= FMath::Min(Width - 1, X2); X++)
        {
            bContinue = EvaluatePixel(X, Y);
        }

        return bContinue;
    };
    
    while (y >= x)
    {
        // Fill rows for symmetry points
        bool bContinue = FillRow(CenterY + y, CenterX - x, CenterX + x);
        bContinue = bContinue && FillRow(CenterY - y, CenterX - x, CenterX + x);
        bContinue = bContinue && FillRow(CenterY + x, CenterX - y, CenterX + y);
        bContinue = bContinue && FillRow(CenterY - x, CenterX - y, CenterX + y);

        if (!bContinue)
            return false;
        
        x++;
        if (DecisionOver2 <= 0)
        {
            DecisionOver2 += 2 * x + 1;
        }
        else
        {
            y--;
            DecisionOver2 += 2 * (x - y) + 1;
        }
    }

    return true;
}

bool UCoreManagerSubSystem::CheckPlacement(int32 I, int32 J, float Inner)
{
    bool bCanPlace = RasterizeCircle(MapDimX,
                             MapDimY,
                            I,
                            J,
                             Inner,
                     [this](int32 X, int32 Y) -> bool
    {
        bool bContinue = true;
        if (X >= 0 && X < MapDimX && Y >= 0 && Y < MapDimY)
        {
            bContinue = !PlacementMap2D[X][Y];
        }

        return bContinue;
    });

    return true;
}

void UCoreManagerSubSystem::PaintPlacement(int32 I, int32 J, float Outer)
{
    bool bCanPlace = RasterizeCircle(MapDimX,
                             MapDimY,
                            I,
                            J,
                             Outer,
                     [this](int32 X, int32 Y) -> bool
    {
        bool bContinue = true;
        if (X >= 0 && X < MapDimX && Y >= 0 && Y < MapDimY && LayerMap2D[X][Y] == SowingLayer)
        {
            PlacementMap2D[X][Y] = 1;
            bPlacementMapUpdated = true;
        }

        return bContinue;
    });
}

bool UCoreManagerSubSystem::Trace(FVector2D ScreenPosition, FVector cameraOffset, const TFunctionRef<void(const FVector&, const FVector&, int32, int32)>& IntersectionAction) const
{
    if (!GetWorld())
    {
        return false;
    }
    
    FVector WSPosition, WSDirection;
    if (!UGameplayStatics::DeprojectScreenToWorld(GetWorld()->GetFirstPlayerController(),
                                                  ScreenPosition,
                                                  WSPosition,
                                                  WSDirection))
    {
        UE_LOG(LogCoreManager, Warning, TEXT("Failed to convert screen-to-world!"));
        return false;
    }
    
    return Trace(WSPosition + cameraOffset, WSPosition + cameraOffset + WSDirection * 10000.f, IntersectionAction);
}

bool UCoreManagerSubSystem::Trace(const FVector& Start, const FVector& End, const TFunctionRef<void(const FVector&, const FVector&, int32, int32)>& IntersectionAction) const
{
    bool bFoundIntersection = false;
    
    FCollisionQueryParams QueryParams;
    FCollisionResponseParams ResponseParams(ECR_Overlap);
    
    TArray<FHitResult> HitResults;
    GetWorld()->LineTraceMultiByChannel(HitResults, Start, End, ECollisionChannel::ECC_WorldStatic, QueryParams, ResponseParams);
    
    for (const FHitResult& Hit : HitResults)
    {
        if (ULandscapeHeightfieldCollisionComponent* HitLandscapeCollision = Cast<ULandscapeHeightfieldCollisionComponent>(Hit.GetComponent()))
        {
            if (TObjectPtr<ULandscapeComponent> HitLandscape = HitLandscapeCollision->GetRenderComponent())
            {
                FVector4 OSLocation = CoreData.Landscape->GetTransform().InverseTransformPosition(Hit.Location);
                OSLocation *= GetMapScale(); 
                
                int32 I = FMath::Min(FMath::FloorToInt32(OSLocation.X), MapDimX - 1);
                int32 J = FMath::Min(FMath::FloorToInt32(OSLocation.Y), MapDimY - 1);

                // Perform the provided intersection action.
                IntersectionAction(Hit.Location, Hit.Normal, I, J);
                
                bFoundIntersection = true;
            }

            // 1 LandscapeComponent intersection is enough.
            break;
        }
    }
    
    return bFoundIntersection;
}

bool UCoreManagerSubSystem::BeginSowing(EHealthDataType HealthDataType, FVector2D ScreenPosition, FCheckResult& CheckResult)
{
		/// UE_LOG( LogCoreManager, Display,  TEXT("BeginSowing: HealthDataType=%d, ScreenPosition=(%f,%f)"), static_cast<int32>(HealthDataType), ScreenPosition.X, ScreenPosition.Y  );
    bool bHitSomething = Trace(ScreenPosition,FVector::ZeroVector, [this, HealthDataType, &CheckResult](const FVector& WSLocation, const FVector& WSNormal, int32 I, int32 J) -> void
    {
        SowingLayer = LayerMap2D[I][J];
        SowingType = HealthDataType;
        
        const FLayerConfig& LayerConfig = CoreConfig->Layers[SowingLayer];

        // It's possible for this to return NULL. This just means someone hasn't configured growables for this layer and type.
        SowingGrowableConfig = LayerConfig.GetRandomGrowable(SowingType);
        if (SowingGrowableConfig)
        {
            if (CheckPlacement(I, J, SowingGrowableConfig->GetInnerRadiusScaled() * GetMapScale()))
            {
                SowingWidgetState = EWidgetState::kClear;
            }
            else
            {
                SowingWidgetState = EWidgetState::kBlocked;
            }
        
            SowingWidgetLocation = WSLocation;
        }
        else
        {
            SowingWidgetState = EWidgetState::kBlocked;
        }
            
        // Format BP outputs.
        CheckResult.Location = WSLocation;
        CheckResult.Layer = SowingLayer;
        // not sure what is the occupied but i think it will be bit map 0 or 1 so if it equal to 1 then it's occupied if 0 it still empty
        CheckResult.Occupied = PlacementMap2D[I][J] == 1;
    });

    return bHitSomething;
}

/**
 * Traces a ray from the screen position into the scene. If an intersection occurs, attempts to place a kernal at the intersection point.
 *
 * @param ScreenPosition The screen position (in 2D space) where the sowing operation should occur.
 * @param SowResult A reference to the FSowResult structure that will store the result of the sowing operation.
 * @return True if the collision trace intersected with the Core, false otherwise.
 */
bool UCoreManagerSubSystem::Sow(FVector2D ScreenPosition, FVector cameraOffset, FSowResult& SowResult)
{
	/// UE_LOG( LogCoreManager, Display,  TEXT("Sow: ScreenPosition=(%f,%f)"), ScreenPosition.X, ScreenPosition.Y  );
   bool bIntersected = Trace(ScreenPosition, cameraOffset, [this, &SowResult](FVector WSLocation, const FVector& WSNormal,int32 I, int32 J) -> void
    {
        SowResult.Layer = LayerMap2D[I][J];

        // Need to handle this unfortunate case. 
        if (SowResult.Layer == kNoLayer)
            return;
        
        SowResult.bSuccess = false;
        SowResult.Layer = LayerMap2D[I][J];
        SowResult.Location = WSLocation;

        SowingWidgetLocation = WSLocation;
        SowingWidgetIndex = FIntVector2(I, J);
        
       // This can happen, unfortunately.
        if (!SowingGrowableConfig)
        {
           SowResult.bSuccess = false;
           SowingWidgetState = EWidgetState::kBlocked;
            return;
        }

       if (!bSowingAlreadyOccupied)
        {
            SowResult.bSuccess = true;
            SowingWidgetState = EWidgetState::kClear;
        }
        else
        {
            SowingWidgetState = EWidgetState::kBlocked;
        }
    });

    if (bIntersected)
    {
        bUpdateSowingWidget = true;
    }
    else
    {
        // If we didn't hit the landscape, we are blocked.
        SowingWidgetState = EWidgetState::kBlocked;
        bUpdateSowingWidget = false;
    }
    
    return bIntersected;
}

void UCoreManagerSubSystem::EndSowing()
{
	UE_LOG( LogCoreManager, Display, TEXT("Ending sowing.") );
    if (SowingWidgetState == EWidgetState::kClear)
    {
        check(SowingType <= EHealthDataType::SocialEngagement);

        // This means there isn't a growable associated with the given layer or currency.
        if (!SowingGrowableConfig)
        {
            bUpdateSowingWidget = false;
            return;
        }
        
        PaintPlacement(SowingWidgetIndex.X, SowingWidgetIndex.Y, SowingGrowableConfig->GetOuterRadiusScaled() * GetMapScale());

        // Add a new kernel.
        constexpr float kInnerRadiusScale = .25f;
        float X = FMath::RandRange(-SowingGrowableConfig->GetInnerRadius() * kInnerRadiusScale, SowingGrowableConfig->GetInnerRadius() * kInnerRadiusScale);
        float Y = FMath::RandRange(-SowingGrowableConfig->GetInnerRadius() * kInnerRadiusScale, SowingGrowableConfig->GetInnerRadius() * kInnerRadiusScale);
        FVector KernelSpawnLocation = SowingWidgetLocation + FVector(X, Y, 5.0f);

        FTransform Transform(KernelSpawnLocation);
        TObjectPtr<AKernel> NewKernel = StaticCast<AKernel*>(GetWorld()->SpawnActorDeferred<AKernel>(CoreConfig->KernelClass, Transform));
        NewKernel->Initialize(SowingLayer, SowingType, SowingGrowableConfig->GetBlockingRadius(),SowingGrowableConfig->Key, CoreConfig->LightAttackTime, CoreConfig->LightDecayTime);
        NewKernel->FinishSpawning(Transform);
        
        CoreData.Kernels.Add(NewKernel);
    }

    SowingGrowableConfig = nullptr;
    bUpdateSowingWidget = false;
}

void UCoreManagerSubSystem::Sprout()
{
    float SproutDelay = 0.0f;
    // Sprout all sown kernels. 
    for (TObjectPtr<AKernel> Kernel : CoreData.Kernels)
    {
        if (Kernel->GetState() == EKernelState::Sown)
        {
            Kernel->Sprout(SproutDelay);
            SproutDelay += CoreConfig->SproutDelay;
        }
    }
}

void UCoreManagerSubSystem::RegisterGrowable(TObjectPtr<AGrowable> NewGrowable, int32 LayerIndex)
{
    CoreData.GrowableActorsByLayer[LayerIndex].Add(NewGrowable);
}

void UCoreManagerSubSystem::ValidateConfig(TObjectPtr<UCoreConfig> CoreConfig)
{
    for (const FLayerConfig& LayerConfig : CoreConfig->Layers)
    {
        check(LayerConfig.Name.Len() > 0);
        
        for (int32 i = 0; i < LayerConfig.EnhancedGrowables.AsArray.Num(); ++i)
        {
            const TArray<FGrowableConfig>* GrowableConfigs = LayerConfig.EnhancedGrowables.AsArray[i];
            for (const FGrowableConfig& GrowableConfig : *GrowableConfigs)
            {
                check(GrowableConfig.GrowableClass);
            } 
        }
    }

    check(CoreConfig->KernelClass);
}

inline void UCoreManagerSubSystem::ExecuteSetSowingWidgetPointCount(const TArray<FString>& Args)
{
    uint32 PointCount = 8;
    if (Args.Num())
    {
        PointCount = FCString::Atoi(*Args[0]);
    }

    SowingWidgetWhiskerCount = PointCount;
}

void UCoreManagerSubSystem::ExecuteSetVisualization(const TArray<FString>& Args)
{
    if (Args.Num() < 1)
    {
        return;
    }

    int32 Mode = FCString::Atoi(*Args[0]);

    if (Mode < VisualizationMode::kFirst || Mode >= VisualizationMode::kLast)
    {
        UE_LOG(LogCoreManager, Error, TEXT("Invalid visualization mode: %d"), Mode);
    }
    
    SetVisualization(Mode);
}

void UCoreManagerSubSystem::GetCameraTarget(FVector& Target) const
{
    if (!CoreData.Landscape)
    {
        return;
    }
    // Caluculate center point on the surface of the landscape. 
    FBox BoundingBox = CoreData.Landscape->GetComponentsBoundingBox().ExpandBy(1.2f);

    FVector Center = BoundingBox.GetCenter();

    FVector Start(Center.X, Center.Y, BoundingBox.Max.Z);
    FVector End(Center.X, Center.Y, BoundingBox.Min.Z);

    Trace(Start, End, [&Target](const FVector4& Location, const FVector4& Normal, int32 I, int32 J) -> void
    {
        (void)Normal;
        
        Target = Location;
    });
}

void UCoreManagerSubSystem::SetVisualization(int32 Mode)
{
    auto SetActorVisibility = [this](bool bVisible) -> void
    {
        for (int32 LayerIndex = 0; LayerIndex < CoreConfig->Layers.Num(); ++LayerIndex)
        {
            for (const TObjectPtr<AActor> GrowableActor : CoreData.GrowableActorsByLayer[LayerIndex])
            {
                GrowableActor->SetActorHiddenInGame(!bVisible);
            }
        }

        for (TObjectPtr Kernel : CoreData.Kernels)
        {
            Kernel->SetActorHiddenInGame(!bVisible);
        }
    };
    
    if (Mode > VisualizationMode::kNone)
    {
#if 0
        if (Mode == VisualizationMode::kPlacement)
        {
            UE_LOG(LogCoreManager, Warning, TEXT("Placement visualization is not available in the editor."));
            return;
        }
#endif

        bVisualizationEnabled = true;
        if (!CoreData.Landscape->bUseDynamicMaterialInstance)
        {
            UE_LOG(LogCoreManager, Warning, TEXT("Landscape is not using dynamic material instances so visualization will not work. Check the \"Use Dynamic Material Instances\" box on the Landscape asset."));
            return;
        }
        
        for (TObjectPtr<ULandscapeComponent> LandscapeComponent : CoreData.Landscape->LandscapeComponents)
        {
            TObjectPtr<UMaterialInstanceDynamic> LandscapeComponentMID = LandscapeComponent->MaterialInstancesDynamic[0]; 
            check(LandscapeComponentMID);
            
            float DebugLandscapeWrap = static_cast<float>(MapResolutionScale)/ (MapDimX + 1);

            TObjectPtr<UTexture2D> VisualizationTexture = Mode == VisualizationMode::kLayers ? LayerMapTexture : PlacementMapTexture;
            LandscapeComponentMID->SetTextureParameterValue("DebugLayerMap", VisualizationTexture);
            LandscapeComponentMID->SetScalarParameterValue("DebugLandscapeWrap", DebugLandscapeWrap);
            LandscapeComponentMID->SetScalarParameterValue("DebugVTBlend", 1.0f);
            LandscapeComponentMID->SetScalarParameterValue("DebugAttributeBlend", 1.0f);
        }

        SetActorVisibility(false);
    }
    else
    {
        bVisualizationEnabled = false;
        for (TObjectPtr<ULandscapeComponent> LandscapeComponent : CoreData.Landscape->LandscapeComponents)
        {
            LandscapeComponent->MaterialInstancesDynamic[0]->SetScalarParameterValue("DebugVTBlend", 0.0f);
            LandscapeComponent->MaterialInstancesDynamic[0]->SetScalarParameterValue("DebugAttributeBlend", 0.0f);
        }
        
        SetActorVisibility(true);
    }
}

DEFINE_LOG_CATEGORY(LogCoreManager);