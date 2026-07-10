// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreConfig.h"

void UCoreConfig::Initialize(float LandscapeScale)
{
	// Calculate growable radii. 
	for (FLayerConfig& LayerConfig : Layers)
	{
		for (int32 i = 0; i < LayerConfig.EnhancedGrowables.AsArray.Num(); ++i)
		{
			for (FGrowableConfig& GrowableConfig : *LayerConfig.EnhancedGrowables.AsArray[i])
			{
				// Calculate bounds. 
				GrowableConfig.CalculateBounds(LandscapeScale);

				// Get the name of the static mesh associated with the growable. The mesh name can be specified in the
				// config or in the growable class.
				FString MeshName = GrowableConfig.StaticMesh->GetName();

				// Make a lookup key formatted as "<layer config>_<health type>_<mesh name>". This is an attempt to
				// support save compability as meshes are added. 
				FString Key = LayerConfig.Name + TEXT("_") +
					          UEnum::GetValueAsString(StaticCast<EHealthDataType>(i)) + TEXT("_") + 
							  MeshName;

				GrowableConfig.Key = FName(*Key);

				UE_LOG(LogCoreManager, Display, TEXT("GrowableConfig: [%s]"), *Key);

				GrowablesLookup.Add(GrowableConfig.Key, &GrowableConfig);
			}                
		}
	}
	
}
