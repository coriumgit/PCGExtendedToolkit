﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGExPathProcessor.h"

#include "PCGExPointsProcessor.h"
#include "PCGExSettings.h"
#include "PCGExBlendPath.generated.h"

class UPCGExSubPointsBlendOperation;
/**
 * Calculates the distance between two points (inherently a n*n operation)
 */
UCLASS(BlueprintType, ClassGroup = (Procedural), Category="PCGEx|Path")
class PCGEXTENDEDTOOLKIT_API UPCGExBlendPathSettings : public UPCGExPathProcessorSettings
{
	GENERATED_BODY()

public:
	//~Begin UPCGSettings interface
#if WITH_EDITOR
	PCGEX_NODE_INFOS(BlendPath, "Path : Blend", "Blend path individual points between its start and end points.");
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

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Settings, Instanced, meta=(PCG_Overridable, ShowOnlyInnerProperties, NoResetToDefault))
	TObjectPtr<UPCGExSubPointsBlendOperation> Blending;
};

struct PCGEXTENDEDTOOLKIT_API FPCGExBlendPathContext final : public FPCGExPathProcessorContext
{
	friend class FPCGExBlendPathElement;

	virtual ~FPCGExBlendPathContext() override;

	EPCGExSubdivideMode SubdivideMethod;
	UPCGExSubPointsBlendOperation* Blending = nullptr;
};

class PCGEXTENDEDTOOLKIT_API FPCGExBlendPathElement final : public FPCGExPathProcessorElement
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

class PCGEXTENDEDTOOLKIT_API FPCGExBlendPathTask final : public PCGExMT::FPCGExTask
{
public:
	FPCGExBlendPathTask(PCGExData::FPointIO* InPointIO) :
		PCGExMT::FPCGExTask(InPointIO)
	{
	}

	virtual bool ExecuteTask() override;
};
