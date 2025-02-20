﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGEx.h"
#include "PCGExGlobalSettings.h"
#include "PCGExPaths.h"

#include "PCGExPointsProcessor.h"
#include "Data/PCGExDataForward.h"


#include "Transform/PCGExTensorsTransform.h"
#include "Transform/PCGExTransform.h"
#include "Transform/Tensors/PCGExTensor.h"
#include "Transform/Tensors/PCGExTensorFactoryProvider.h"
#include "Transform/Tensors/PCGExTensorHandler.h"

#include "PCGExExtrudeTensors.generated.h"

UENUM()
enum class EPCGExSelfIntersectionMode : uint8
{
	StopLongest  = 0 UMETA(DisplayName = "Stop Longest", Tooltip="Stop the longest path first"),
	StopShortest = 1 UMETA(DisplayName = "Stop Shortest", Tooltip="Stop the shortest path first"),
	SortingOnly  = 2 UMETA(DisplayName = "Sorting only", Tooltip="Stop the path based on sorting rules."),
};

UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Misc")
class /*PCGEXTENDEDTOOLKIT_API*/ UPCGExExtrudeTensorsSettings : public UPCGExPointsProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings
#if WITH_EDITOR
	PCGEX_NODE_INFOS(ExtrudeTensors, "Path : Extrude Tensors", "Extrude input points into paths along tensors.");
	virtual FLinearColor GetNodeTitleColor() const override { return GetDefault<UPCGExGlobalSettings>()->NodeColorTransform; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings

	//~Begin UPCGExPointsProcessorSettings
public:
	virtual FName GetMainInputPin() const override;
	virtual FName GetMainOutputPin() const override;
	//~End UPCGExPointsProcessorSettings

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	bool bTransformRotation = true;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="bTransformRotation"))
	EPCGExTensorTransformMode Rotation = EPCGExTensorTransformMode::Align;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="bTransformRotation && Rotation == EPCGExTensorTransformMode::Align"))
	EPCGExAxis AlignAxis = EPCGExAxis::Forward;

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, InlineEditConditionToggle))
	bool bUsePerPointMaxIterations = false;

	/** Per-point Max Iterations. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Per-point Iterations", EditCondition="bUsePerPointMaxIterations"))
	FName IterationsAttribute = FName("Iterations");

	/** Max Iterations. If using per-point max, this will act as a clamping mechanism. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Max Iterations", ClampMin=1))
	int32 Iterations = 1;

	/** Whether to adjust max iteration based on max value found on points. Use at your own risks! */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Use Max from Points", ClampMin=1, EditCondition="bUsePerPointMaxIterations", HideEditConditionToggle))
	bool bUseMaxFromPoints = false;

	/** Whether to give a new seed to the points. If disabled, they will inherit the original one. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable))
	bool bRefreshSeed = true;

	/** Whether the node should attempt to close loops based on angle and proximity */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Closing Loops", meta=(PCG_NotOverridable))
	bool bDetectClosedLoops = false;

	/** Range at which the first point must be located to check angle */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Closing Loops", meta=(PCG_Overridable, DisplayName=" ├─ Search Distance", EditCondition="bCloseLoops"))
	double ClosedLoopSearchDistance = 100;

	/** Angle at which the loop will be closed, if within range */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Closing Loops", meta=(PCG_Overridable, DisplayName=" └─ Search Angle", EditCondition="bCloseLoops", Units="Degrees", ClampMin=0, ClampMax=90))
	double ClosedLoopSearchAngle = 11.25;

	/** Whether to limit the length of the generated path */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_NotOverridable))
	bool bUseMaxLength = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_NotOverridable, EditCondition="bUseMaxLength", EditConditionHides))
	EPCGExInputValueType MaxLengthInput = EPCGExInputValueType::Constant;

	/** Max length Attribute */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable, DisplayName="Max Length (Attr)", EditCondition="bUseMaxLength && MaxLengthInput!=EPCGExInputValueType::Constant", EditConditionHides))
	FName MaxLengthAttribute = FName("MaxLength");

	/** Max length Constant */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable, DisplayName="Max Length", EditCondition="bUseMaxLength && MaxLengthInput==EPCGExInputValueType::Constant", EditConditionHides, ClampMin=1))
	double MaxLength = 100;


	/** Whether to limit the number of points in a generated path */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_NotOverridable))
	bool bUseMaxPointsCount = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_NotOverridable, EditCondition="bUseMaxPointsCount", EditConditionHides))
	EPCGExInputValueType MaxPointsCountInput = EPCGExInputValueType::Constant;

	/** Max length Attribute */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable, DisplayName="Max Points Count (Attr)", EditCondition="bUseMaxPointsCount && MaxPointsCountInput!=EPCGExInputValueType::Constant", EditConditionHides))
	FName MaxPointsCountAttribute = FName("MaxPointsCount");

	/** Max length Constant */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable, DisplayName="Max Points Count", EditCondition="bUseMaxPointsCount && MaxPointsCountInput==EPCGExInputValueType::Constant", EditConditionHides, ClampMin=1))
	int32 MaxPointsCount = 100;


	/** Whether to limit path length or not */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable, ClampMin=0.001))
	double FuseDistance = 0.01;

	/** How to deal with points that are stopped */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable))
	EPCGExTensorStopConditionHandling StopConditionHandling = EPCGExTensorStopConditionHandling::Exclude;

	/** Whether to stop sampling when extrusion is stopped. While path will be cut, there's a chance that the head of the search comes back into non-stopping conditions, which would start a new extrusion. With this option disabled, new paths won't be permitted to exist. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_Overridable))
	bool bAllowChildExtrusions = false;

	/** If enabled, seeds that start stopped won't be extruded at all. Otherwise, they are transformed until they eventually reach a point that's outside stopping conditions and start an extrusion. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Limits", meta=(PCG_NotOverridable))
	bool bIgnoreStoppedSeeds = false;

	/**  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Ext)", meta=(PCG_Overridable))
	bool bDoExternalPathIntersections = false;

	/** If enabled, if the origin location of the extrusion is detected as an intersection, it is not considered an intersection. This allows to have seeds perfectly located on paths used for intersections. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Ext)", meta=(PCG_Overridable, EditCondition="bDoExternalPathIntersections"))
	bool bIgnoreIntersectionOnOrigin = true;

	/** Closed loop handling for external paths.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Ext)", meta=(PCG_Overridable, EditCondition="bDoExternalPathIntersections"))
	FPCGExPathClosedLoopDetails ClosedLoop;

	/** Intersection settings  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Ext)", meta=(PCG_Overridable, EditCondition="bDoExternalPathIntersections"))
	FPCGExPathIntersectionDetails ExternalPathIntersections;

	/** Whether to test for intersection between actively extruding paths */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Self)", meta=(PCG_Overridable))
	bool bDoSelfPathIntersections = false;

	/** How to order intersection checks. Sorting is using seeds input attributes. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Self)", meta=(PCG_Overridable, EditCondition="bDoSelfPathIntersections"))
	EPCGExSelfIntersectionMode SelfIntersectionMode = EPCGExSelfIntersectionMode::SortingOnly;

	/** Controls the order in which paths extrusion will be stopped when intersecting, if shortest/longest path fails. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Self)", meta = (PCG_Overridable, EditCondition="bDoSelfPathIntersections"))
	EPCGExSortDirection SortDirection = EPCGExSortDirection::Ascending;

	/** Intersection settings for extruding path intersections */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Intersections (Self)", meta=(PCG_Overridable, EditCondition="bDoSelfPathIntersections"))
	FPCGExPathIntersectionDetails SelfPathIntersections;


	/** TBD */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding")
	FPCGExAttributeToTagDetails AttributesToPathTags;

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfChildExtrusion = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfChildExtrusion"))
	FString IsChildExtrusionTag = TEXT("ChildExtrusion");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfIsStoppedByFilters = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfIsStoppedByFilters"))
	FString IsStoppedByFiltersTag = TEXT("StoppedByFilters");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfIsStoppedByIntersection = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfIsStoppedByIntersection"))
	FString IsStoppedByIntersectionTag = TEXT("StoppedByIntersection");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfIsStoppedBySelfIntersection = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfIsStoppedBySelfIntersection"))
	FString IsStoppedBySelfIntersectionTag = TEXT("StoppedBySelfIntersection");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfIsFollowUp = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfIsFollowUp"))
	FString IsFollowUpTag = TEXT("IsFollowUp");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfClosedLoop = true;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfClosedLoop"))
	FString IsClosedLoopTag = TEXT("ClosedLoop");

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(InlineEditConditionToggle))
	bool bTagIfOpenPath = false;

	/** ... */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Settings|Tagging & Forwarding", meta=(EditCondition="bTagIfOpenPath"))
	FString IsOpenPathTag = TEXT("OpenPath");

	/** Tensor sampling settings. Note that these are applied on the flattened sample, e.g after & on top of individual tensors' mutations. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, DisplayName="Tensor Sampling Settings"))
	FPCGExTensorHandlerDetails TensorHandlerDetails;


	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Warning and Errors", meta=(PCG_NotOverridable, AdvancedDisplay))
	bool bQuietMissingTensorError = false;


	virtual bool GetSortingRules(FPCGExContext* InContext, TArray<FPCGExSortRuleConfig>& OutRules) const;

private:
	friend class FPCGExExtrudeTensorsElement;
};

struct /*PCGEXTENDEDTOOLKIT_API*/ FPCGExExtrudeTensorsContext final : FPCGExPointsProcessorContext
{
	friend class FPCGExExtrudeTensorsElement;

	TArray<TObjectPtr<const UPCGExTensorFactoryData>> TensorFactories;
	TArray<TObjectPtr<const UPCGExFilterFactoryData>> StopFilterFactories;

	FPCGExPathClosedLoopDetails ClosedLoop;
	FPCGExPathIntersectionDetails ExternalPathIntersections;
	FPCGExPathIntersectionDetails SelfPathIntersections;

	double ClosedLoopSquaredDistance = 0;
	double ClosedLoopSearchDot = 0;

	TArray<TSharedPtr<PCGExData::FFacade>> PathsFacades;
	TArray<TSharedPtr<PCGExPaths::FPath>> ExternalPaths;
};

class /*PCGEXTENDEDTOOLKIT_API*/ FPCGExExtrudeTensorsElement final : public FPCGExPointsProcessorElement
{
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGExContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

namespace PCGExExtrudeTensors
{
	enum class EExtrusionFlags : uint32
	{
		None           = 0,
		Bounded        = 1 << 0,
		ClosedLoop     = 1 << 1,
		AllowsChildren = 1 << 2,
		CollisionCheck = 1 << 3,
	};

	constexpr bool Supports(const EExtrusionFlags Flags, EExtrusionFlags Flag) { return (static_cast<uint32>(Flags) & static_cast<uint32>(Flag)) != 0; }

	class FProcessor;

	class FExtrusion : public TSharedFromThis<FExtrusion>
	{
	protected:
		TArray<FPCGPoint>& ExtrudedPoints;
		TArray<FBox> SegmentBounds;
		double DistToLastSum = 0;
		FPCGPoint Origin;

	public:
		FBox Bounds = FBox(ForceInit);

		bool bIsExtruding = false;
		bool bIsComplete = false;
		bool bIsStopped = false;
		bool bIsClosedLoop = false;
		bool bHitStopFilters = false;
		bool bHitIntersection = false;
		bool bHitSelfIntersection = false;

		bool bIsProbe = false;
		bool bIsChildExtrusion = false;
		bool bIsFollowUp = false;
		bool bAdvancedOnly = false;

		virtual ~FExtrusion() = default;
		FProcessor* Processor = nullptr;
		const FPCGExExtrudeTensorsContext* Context = nullptr;
		const UPCGExExtrudeTensorsSettings* Settings = nullptr;
		TSharedPtr<TArray<TSharedPtr<PCGExPaths::FPath>>> StaticPaths;
		TSharedPtr<PCGExTensor::FTensorsHandler> TensorsHandler;
		TSharedPtr<PCGExPointFilter::FManager> StopFilters;

		FVector LastInsertion = FVector::ZeroVector;
		FVector ExtrusionDirection = FVector::ZeroVector;
		FTransform Head = FTransform::Identity;
		FBox ActiveExtrusionBounds = FBox(ForceInit);

		int32 SeedIndex = -1;
		int32 RemainingIterations = 0;
		double MaxLength = MAX_dbl;
		int32 MaxPointCount = MAX_int32;

		PCGExPaths::FPathMetrics Metrics;

		TSharedRef<PCGExData::FFacade> PointDataFacade;

		TSharedPtr<PCGEx::FIndexedItemOctree> EdgeOctree;
		const TArray<TSharedPtr<FExtrusion>>* Extrusions = nullptr;

		FExtrusion(const int32 InSeedIndex, const TSharedRef<PCGExData::FFacade>& InFacade, const int32 InMaxIterations);

		const TArray<FPCGPoint>& GetExtrudedPoints() const { return ExtrudedPoints; }

		const FBox& GetHeadEdge(FVector& OutA, FVector& OutB) const;
		void SetHead(const FTransform& InHead);

		virtual bool Advance() = 0;
		void Complete();
		void CutOff(const FVector& InCutOff);
		void Shorten(const FVector& InCutOff);

		bool FindClosestSolidIntersection(
			const FBox& InBox, const FVector& AB,
			const FVector& A1, const FVector& B1, FVector& OutIntersection,
			bool& OutIsLastSegment) const;

		void Cleanup();

	protected:
		bool OnAdvanced(const bool bStop);
		virtual bool Extrude(const PCGExTensor::FTensorSample& Sample, FPCGPoint& InPoint) = 0;
		void StartNewExtrusion();
		void Insert(const FPCGPoint& InPoint);
	};

	template <EExtrusionFlags InternalFlags>
	class TExtrusion : public FExtrusion
	{
	public:
		TExtrusion(const int32 InSeedIndex, const TSharedRef<PCGExData::FFacade>& InFacade, const int32 InMaxIterations):
			FExtrusion(InSeedIndex, InFacade, InMaxIterations)
		{
		}

		virtual bool Advance() override
		{
			if (bIsStopped) { return false; }

			if (!bAdvancedOnly) { ActiveExtrusionBounds = FBox(ForceInit); }

			bAdvancedOnly = true;

			const FVector PreviousHeadLocation = Head.GetLocation();
			bool bSuccess = false;
			const PCGExTensor::FTensorSample Sample = TensorsHandler->Sample(SeedIndex, Head, bSuccess);

			if (!bSuccess) { return OnAdvanced(true); }

			ExtrusionDirection = Sample.DirectionAndSize.GetSafeNormal();

			// Apply sample to head

			if (Settings->bTransformRotation)
			{
				if (Settings->Rotation == EPCGExTensorTransformMode::Absolute)
				{
					Head.SetRotation(Sample.Rotation);
				}
				else if (Settings->Rotation == EPCGExTensorTransformMode::Relative)
				{
					Head.SetRotation(Head.GetRotation() * Sample.Rotation);
				}
				else if (Settings->Rotation == EPCGExTensorTransformMode::Align)
				{
					Head.SetRotation(PCGExMath::MakeDirection(Settings->AlignAxis, ExtrusionDirection * -1, Head.GetRotation().GetUpVector()));
				}
			}

			const FVector HeadLocation = PreviousHeadLocation + Sample.DirectionAndSize;
			Head.SetLocation(HeadLocation);
			Bounds += HeadLocation;
			ActiveExtrusionBounds += (HeadLocation + FVector::OneVector * 1);
			ActiveExtrusionBounds += (HeadLocation + FVector::OneVector * -1);

			if constexpr (Supports(InternalFlags, EExtrusionFlags::ClosedLoop))
			{
				if (const FVector Tail = Origin.Transform.GetLocation();
					FVector::DistSquared(Metrics.Last, Tail) <= Context->ClosedLoopSquaredDistance &&
					FVector::DotProduct(ExtrusionDirection, (Tail - PreviousHeadLocation).GetSafeNormal()) > Context->ClosedLoopSearchDot)
				{
					bIsClosedLoop = true;
					return OnAdvanced(true);
				}
			}

			FPCGPoint HeadPoint = ExtrudedPoints.Last();
			HeadPoint.Transform = Head;

			if constexpr (Supports(InternalFlags, EExtrusionFlags::Bounded))
			{
				if (StopFilters->Test(HeadPoint))
				{
					if (bIsExtruding && !bIsComplete)
					{
						bHitStopFilters = true;
						if (Settings->StopConditionHandling == EPCGExTensorStopConditionHandling::Include) { Insert(HeadPoint); }

						Complete();

						if constexpr (!Supports(InternalFlags, EExtrusionFlags::AllowsChildren))
						{
							return OnAdvanced(true);
						}
					}

					return OnAdvanced(false);
				}

				if (bIsComplete)
				{
					if constexpr (Supports(InternalFlags, EExtrusionFlags::AllowsChildren))
					{
						StartNewExtrusion();
					}
					return OnAdvanced(true);
				}

				if (!bIsExtruding)
				{
					// Start writing path
					bIsExtruding = true;
					if (bIsProbe)
					{
						SetHead(Head);
						return OnAdvanced(false);
					}
				}
			}

			return OnAdvanced(!Extrude(Sample, HeadPoint));
		}

	protected:
		virtual bool Extrude(const PCGExTensor::FTensorSample& Sample, FPCGPoint& InPoint) override;
	};

	template <EExtrusionFlags InternalFlags>
	bool TExtrusion<InternalFlags>::Extrude(const PCGExTensor::FTensorSample& Sample, FPCGPoint& InPoint)
	{
		// return whether we can keep extruding or not
		bIsExtruding = true;

		double DistToLast = 0;
		const double Length = Metrics.Add(Metrics.Last + Sample.DirectionAndSize, DistToLast);
		DistToLastSum += DistToLast;

		if (DistToLastSum < Settings->FuseDistance) { return true; }
		DistToLastSum = 0;

		if (Length > MaxLength)
		{
			// Adjust position to match max length
			const FVector LastValidPos = ExtrudedPoints.Last().Transform.GetLocation();
			InPoint.Transform.SetLocation(LastValidPos + ((Metrics.Last - LastValidPos).GetSafeNormal() * (Length - MaxLength)));
		}

		if constexpr (Supports(InternalFlags, EExtrusionFlags::CollisionCheck))
		{
			int32 PathIndex = -1;
			int32 SegmentIndex = -1;
			FVector Intersection = FVector::ZeroVector;

			bIsExtruding = true;

			const FVector StartPt = ExtrudedPoints.Last().Transform.GetLocation();

			if (FindClosestIntersection(
				Context->ExternalPaths, Context->ExternalPathIntersections,
				StartPt, InPoint.Transform.GetLocation(),
				PathIndex, SegmentIndex, Intersection))
			{
				bHitIntersection = true;


				if (FMath::IsNearlyZero(FVector::DistSquared(Intersection, StartPt)))
				{
					if (!Settings->bIgnoreIntersectionOnOrigin || (Settings->bIgnoreIntersectionOnOrigin && ExtrudedPoints.Num() > 1))
					{
						return OnAdvanced(true);
					}
				}
				else
				{
					InPoint.Transform.SetLocation(Intersection);
					Insert(InPoint);
					return OnAdvanced(true);
				}
			}

			if (FindClosestIntersection(
					*StaticPaths.Get(), Context->SelfPathIntersections,
					StartPt, InPoint.Transform.GetLocation(),
					PathIndex, SegmentIndex, Intersection))
			{
				bHitIntersection = true;
				bHitSelfIntersection = true;

				if (FMath::IsNearlyZero(FVector::DistSquared(Intersection, StartPt)))
				{
					if (!Settings->bIgnoreIntersectionOnOrigin || (Settings->bIgnoreIntersectionOnOrigin && ExtrudedPoints.Num() > 1))
					{
						return OnAdvanced(true);
					}
				}
				else
				{
					InPoint.Transform.SetLocation(Intersection);
					Insert(InPoint);
					return OnAdvanced(true);
				}
			}
		}

		Insert(InPoint);

		return !(Length >= MaxLength || ExtrudedPoints.Num() >= MaxPointCount);
	}

	class FProcessor final : public PCGExPointsMT::TPointsProcessor<FPCGExExtrudeTensorsContext, UPCGExExtrudeTensorsSettings>
	{
	protected:
		TSharedPtr<PCGExSorting::PointSorter<true>> Sorter;

		FRWLock NewExtrusionLock;
		int32 RemainingIterations = 0;

		TSharedPtr<PCGExData::TBuffer<int32>> PerPointIterations;
		TSharedPtr<PCGExData::TBuffer<int32>> PerPointMaxPoints;
		TSharedPtr<PCGExData::TBuffer<double>> PerPointMaxLength;

		TSharedPtr<PCGExPointFilter::FManager> StopFilters;
		TSharedPtr<PCGExTensor::FTensorsHandler> TensorsHandler;

		FPCGExAttributeToTagDetails AttributesToPathTags;
		TArray<TSharedPtr<FExtrusion>> ExtrusionQueue;
		TArray<TSharedPtr<FExtrusion>> NewExtrusions;

		TSharedPtr<PCGExMT::TScopedArray<TSharedPtr<FExtrusion>>> CompletedExtrusions;
		TSharedPtr<TArray<TSharedPtr<PCGExPaths::FPath>>> StaticPaths;

	public:
		explicit FProcessor(const TSharedRef<PCGExData::FFacade>& InPointDataFacade):
			TPointsProcessor(InPointDataFacade)
		{
		}

		virtual ~FProcessor() override;

		virtual bool IsTrivial() const override { return false; }

		virtual void RegisterBuffersDependencies(PCGExData::FFacadePreloader& FacadePreloader) override;

		virtual bool Process(const TSharedPtr<PCGExMT::FTaskManager>& InAsyncManager) override;

		void InitExtrusionFromSeed(const int32 InSeedIndex);
		TSharedPtr<FExtrusion> InitExtrusionFromExtrusion(const TSharedRef<FExtrusion>& InExtrusion);

		virtual void PrepareLoopScopesForRanges(const TArray<PCGExMT::FScope>& Loops) override;
		virtual void PrepareSingleLoopScopeForPoints(const PCGExMT::FScope& Scope) override;
		virtual void ProcessSinglePoint(const int32 Index, FPCGPoint& Point, const PCGExMT::FScope& Scope) override;
		virtual void OnPointsProcessingComplete() override;

		virtual void ProcessSingleRangeIteration(const int32 Iteration, const PCGExMT::FScope& Scope) override;
		virtual void OnRangeProcessingComplete() override;

		bool UpdateExtrusionQueue();

		virtual void CompleteWork() override;

	protected:
		EExtrusionFlags ComputeFlags() const
		{
			uint32 Flags = 0;

			if (Settings->bAllowChildExtrusions) { Flags |= static_cast<uint32>(EExtrusionFlags::AllowsChildren); }
			if (Settings->bDetectClosedLoops) { Flags |= static_cast<uint32>(EExtrusionFlags::ClosedLoop); }
			if (StopFilters) { Flags |= static_cast<uint32>(EExtrusionFlags::Bounded); }
			if (!Context->ExternalPaths.IsEmpty() || Settings->bDoSelfPathIntersections) { Flags |= static_cast<uint32>(EExtrusionFlags::CollisionCheck); }

			return static_cast<EExtrusionFlags>(Flags);
		}

		TSharedPtr<FExtrusion> CreateExtrusionTemplate(const int32 InSeedIndex, const int32 InMaxIterations);
	};

	class FBatch final : public PCGExPointsMT::TBatch<FProcessor>
	{
	public:
		explicit FBatch(FPCGExContext* InContext, const TArray<TWeakPtr<PCGExData::FPointIO>>& InPointsCollection);
		virtual void Process(TSharedPtr<PCGExMT::FTaskManager> InAsyncManager) override;
		void OnPathsPrepared();
	};
}
