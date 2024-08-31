﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExGlobalSettings.h"

#include "PCGExPointsProcessor.h"
#include "AssetSelectors/PCGExMeshCollection.h"
#include "Geometry/PCGExFitting.h"
#include "PCGExAssetStaging.generated.h"

UCLASS(MinimalAPI, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Misc")
class /*PCGEXTENDEDTOOLKIT_API*/ UPCGExAssetStagingSettings : public UPCGExPointsProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings
#if WITH_EDITOR
	PCGEX_NODE_INFOS_CUSTOM_SUBTITLE(
		AssetStaging, "Asset Staging", "Data staging from PCGEx Asset Collections.",
		FName(TEXT("[ ") + ( CollectionSource == EPCGExCollectionSource::Asset ? AssetCollection.GetAssetName() : TEXT("Attribute Set to Collection")) + TEXT(" ]")));
	virtual FLinearColor GetNodeTitleColor() const override { return GetDefault<UPCGExGlobalSettings>()->NodeColorMiscAdd; }
#endif

protected:
	virtual FPCGElementPtr CreateElement() const override;
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	//~End UPCGSettings

	//~Begin UPCGExPointsProcessorSettings
public:
	virtual PCGExData::EInit GetMainOutputInitMode() const override;
	//~End UPCGExPointsProcessorSettings

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExCollectionSource CollectionSource = EPCGExCollectionSource::Asset;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="CollectionSource == EPCGExCollectionSource::Asset", EditConditionHides))
	TSoftObjectPtr<UPCGExAssetCollection> AssetCollection;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="CollectionSource == EPCGExCollectionSource::AttributeSet", EditConditionHides))
	FPCGExAssetAttributeSetDetails AttributeSetDetails;

	/** Distribution details */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Distribution", meta=(PCG_Overridable, ShowOnlyInnerProperties))
	FPCGExAssetDistributionDetails DistributionSettings;

	/** The name of the attribute to write asset path to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Distribution", meta=(PCG_Overridable))
	FName AssetPathAttributeName = "AssetPath";

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	FPCGExScaleToFitDetails ScaleToFit;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	FPCGExJustificationDetails Justification;

	///** If enabled, filter output based on whether a staging has been applied or not (empty entry). \n NOT IMPLEMENTED YET */
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Distribution", meta=(PCG_Overridable))
	//bool bOmitInvalidStagedPoints = false;

	/** Update point scale so staged asset fits within its bounds */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Staged Properties", meta=(PCG_Overridable))
	EPCGExWeightOutputMode WeightToAttribute = EPCGExWeightOutputMode::NoOutput;

	/** The name of the attribute to write asset weight to.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Staged Properties", meta=(PCG_Overridable, EditCondition="WeightToAttribute!=EPCGExWeightOutputMode::NoOutput && WeightToAttribute!=EPCGExWeightOutputMode::NormalizedToDensity && WeightToAttribute!=EPCGExWeightOutputMode::NormalizedInvertedToDensity"))
	FName WeightAttributeName = "AssetWeight";
};

struct /*PCGEXTENDEDTOOLKIT_API*/ FPCGExAssetStagingContext final : public FPCGExPointsProcessorContext
{
	friend class FPCGExAssetStagingElement;

	virtual ~FPCGExAssetStagingContext() override;

	TObjectPtr<UPCGExAssetCollection> MainCollection;
};

class /*PCGEXTENDEDTOOLKIT_API*/ FPCGExAssetStagingElement final : public FPCGExPointsProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGExContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

namespace PCGExAssetStaging
{
	class FProcessor final : public PCGExPointsMT::FPointsProcessor
	{
		int32 NumPoints = 0;
		int32 MaxIndex = 0;
		double MaxInputIndex = 0;

		bool bOutputWeight = false;
		bool bOneMinusWeight = false;
		bool bNormalizedWeight = false;

		const UPCGExAssetStagingSettings* LocalSettings = nullptr;
		const FPCGExAssetStagingContext* LocalTypedContext = nullptr;

		FPCGExJustificationDetails Justification;
		FPCGExAssetDistributionDetails Details;

		PCGExData::FCache<int32>* IndexGetter = nullptr;
		PCGEx::TFAttributeWriter<int32>* WeightWriter = nullptr;
		PCGEx::TFAttributeWriter<double>* NormalizedWeightWriter = nullptr;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION > 3
		PCGEx::TFAttributeWriter<FSoftObjectPath>* PathWriter = nullptr;
#else
		PCGEx::TFAttributeWriter<FString>* PathWriter = nullptr;
#endif

	public:
		explicit FProcessor(PCGExData::FPointIO* InPoints):
			FPointsProcessor(InPoints)
		{
		}

		virtual ~FProcessor() override
		{
		}

		virtual bool Process(PCGExMT::FTaskManager* AsyncManager) override;
		virtual void PrepareSingleLoopScopeForPoints(const uint32 StartIndex, const int32 Count) override;
		virtual void ProcessSinglePoint(const int32 Index, FPCGPoint& Point, const int32 LoopIdx, const int32 Count) override;
		virtual void CompleteWork() override;
	};
}
