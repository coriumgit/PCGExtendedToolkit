﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExFilterFactoryProvider.h"
#include "UObject/Object.h"

#include "Data/PCGExPointFilter.h"
#include "PCGExPointsProcessor.h"

#include "Paths/PCGExPaths.h"

#include "PCGExPolygonInclusionFilter.generated.h"

USTRUCT(BlueprintType)
struct FPCGExPolygonInclusionFilterConfig
{
	GENERATED_BODY()

	FPCGExPolygonInclusionFilterConfig()
	{
	}

	/** If enabled, invert the result of the test */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	bool bInvert = false;
};

/**
 * 
 */
UCLASS(MinimalAPI, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Filter")
class UPCGExPolygonInclusionFilterFactory : public UPCGExFilterFactoryData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FPCGExPolygonInclusionFilterConfig Config;

	virtual bool SupportsDirectEvaluation() const override { return true; } // TODO Change this one we support per-point tolerance from attribute

	TSharedPtr<TArray<FBox>> Bounds;
	TSharedPtr<TArray<TSharedPtr<TArray<FVector2D>>>> Polygons;

	virtual bool Init(FPCGExContext* InContext) override;
	virtual bool WantsPreparation(FPCGExContext* InContext) override;
	virtual bool Prepare(FPCGExContext* InContext) override;

	virtual TSharedPtr<PCGExPointFilter::FFilter> CreateFilter() const override;

	virtual void BeginDestroy() override;
};

namespace PCGExPointFilter
{
	class FPolygonInclusionFilter final : public FSimpleFilter
	{
	public:
		explicit FPolygonInclusionFilter(const TObjectPtr<const UPCGExPolygonInclusionFilterFactory>& InFactory)
			: FSimpleFilter(InFactory), TypedFilterFactory(InFactory)
		{
			Bounds = TypedFilterFactory->Bounds;
			Polygons = TypedFilterFactory->Polygons;
		}

		const TObjectPtr<const UPCGExPolygonInclusionFilterFactory> TypedFilterFactory;

		TSharedPtr<TArray<FBox>> Bounds;
		TSharedPtr<TArray<TSharedPtr<TArray<FVector2D>>>> Polygons;

		virtual bool Init(FPCGExContext* InContext, const TSharedPtr<PCGExData::FFacade>& InPointDataFacade) override;
		virtual bool Test(const FPCGPoint& Point) const override;
		virtual bool Test(const int32 PointIndex) const override;

		virtual ~FPolygonInclusionFilter() override
		{
		}
	};
}

///

UCLASS(MinimalAPI, BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Filter")
class UPCGExPolygonInclusionFilterProviderSettings : public UPCGExFilterProviderSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings
#if WITH_EDITOR
	PCGEX_NODE_INFOS_CUSTOM_SUBTITLE(
		PolygonInclusionFilterFactory, "Filter : Polygon 2D Inclusion", "Creates a filter definition that checks points inclusion inside polygon. This is resolved on a flat XY plane.",
		PCGEX_FACTORY_NAME_PRIORITY)
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	//~End UPCGSettings

public:
	/** Filter Config.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, ShowOnlyInnerProperties))
	FPCGExPolygonInclusionFilterConfig Config;

	virtual UPCGExFactoryData* CreateFactory(FPCGExContext* InContext, UPCGExFactoryData* InFactory) const override;

#if WITH_EDITOR
	virtual FString GetDisplayName() const override;
#endif
};
