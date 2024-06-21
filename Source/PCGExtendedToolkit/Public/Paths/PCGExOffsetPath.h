﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExPathProcessor.h"

#include "PCGExPointsProcessor.h"
#include "Geometry/PCGExGeo.h"
#include "PCGExOffsetPath.generated.h"

/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Path")
class PCGEXTENDEDTOOLKIT_API UPCGExOffsetPathSettings : public UPCGExPathProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(OffsetPath, "Path : Offset", "Offset paths points.");
#endif

protected:
	virtual FPCGElementPtr CreateElement() const override;
	//~End UPCGSettings interface

	//~Begin UObject interface
public:
	virtual void PostInitProperties() override;
#if WITH_EDITOR

public:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~End UObject interface

	//~Begin UPCGExPointsProcessorSettings interface
public:
	virtual PCGExData::EInit GetMainOutputInitMode() const override;
	//~End UPCGExPointsProcessorSettings interface

public:
	/** Consider paths to be closed -- processing will wrap between first and last points. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool bClosedPath = false;

	/** Offset type.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGExFetchType OffsetType = EPCGExFetchType::Constant;

	/** Offset size.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="OffsetType == EPCGExFetchType::Constant"))
	double OffsetConstant = 1.0;

	/** Fetch the offset size from a local attribute. The regular Size parameter then act as a scale.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="OffsetType == EPCGExFetchType::Attribute"))
	FPCGAttributePropertyInputSelector OffsetAttribute;

	/** Up Vector type.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGExFetchType UpVectorType = EPCGExFetchType::Constant;

	/** Up vector used to calculate Offset direction.*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="UpVectorType == EPCGExFetchType::Constant"))
	FVector UpVectorConstant = FVector::UpVector;

	/** Fetch the Up vector from a local point attribute. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="UpVectorType == EPCGExFetchType::Attribute"))
	FPCGAttributePropertyInputSelector UpVectorAttribute;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExOffsetPathContext final : public FPCGExPathProcessorContext
{
	friend class FPCGExOffsetPathElement;

	virtual ~FPCGExOffsetPathContext() override;
};

class PCGEXTENDEDTOOLKIT_API FPCGExOffsetPathElement final : public FPCGExPathProcessorElement
{
public:
	virtual FPCGContext* Initialize(
		const FPCGDataCollection& InputData,
		TWeakObjectPtr<UPCGComponent> SourceComponent,
		const UPCGNode* Node) override;

protected:
	virtual bool Boot(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};

namespace PCGExOffsetPath
{
	class FProcessor final : public PCGExPointsMT::FPointsProcessor
	{
		int32 NumPoints = 0;

		double OffsetConstant = 0;
		FVector UpVector = FVector::UpVector;

		TArray<FVector> Positions;
		TArray<FVector> Normals;

		PCGEx::FLocalSingleFieldGetter* OffsetGetter = nullptr;
		PCGEx::FLocalVectorGetter* UpGetter = nullptr;

	public:
		explicit FProcessor(PCGExData::FPointIO* InPoints):
			FPointsProcessor(InPoints)
		{
		}

		virtual ~FProcessor() override;

		virtual bool Process(PCGExMT::FTaskManager* AsyncManager) override;
		virtual void ProcessSinglePoint(const int32 Index, FPCGPoint& Point) override;
		virtual void ProcessSingleRangeIteration(const int32 Iteration) override;
		virtual void CompleteWork() override;

	protected:
		FVector NRM(const int32 A, const int32 B, const int32 C) const;
	};
}
