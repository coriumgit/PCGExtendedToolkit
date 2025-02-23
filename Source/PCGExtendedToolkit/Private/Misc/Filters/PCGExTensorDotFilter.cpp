﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#include "Misc/Filters/PCGExTensorDotFilter.h"


#include "Transform/Tensors/PCGExTensorFactoryProvider.h"
#include "Transform/Tensors/PCGExTensorHandler.h"


#define LOCTEXT_NAMESPACE "PCGExTensorDotFilterDefinition"
#define PCGEX_NAMESPACE PCGExTensorDotFilterDefinition

bool UPCGExTensorDotFilterFactory::Init(FPCGExContext* InContext)
{
	if (!Super::Init(InContext)) { return false; }

	if (!PCGExFactories::GetInputFactories(InContext, PCGExTensor::SourceTensorsLabel, TensorFactories, {PCGExFactories::EType::Tensor}, true)) { return false; }
	if (TensorFactories.IsEmpty())
	{
		if (!bQuietMissingInputError) { PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Missing tensors.")); }
		return false;
	}

	return true;
}

TSharedPtr<PCGExPointFilter::FFilter> UPCGExTensorDotFilterFactory::CreateFilter() const
{
	return MakeShared<PCGExPointFilter::FTensorDotFilter>(this);
}

bool UPCGExTensorDotFilterFactory::RegisterConsumableAttributesWithData(FPCGExContext* InContext, const UPCGData* InData) const
{
	if (!Super::RegisterConsumableAttributesWithData(InContext, InData)) { return false; }

	FName Consumable = NAME_None;
	PCGEX_CONSUMABLE_SELECTOR(Config.OperandA, Consumable)
	PCGEX_CONSUMABLE_CONDITIONAL(Config.DotComparisonDetails.ThresholdInput == EPCGExInputValueType::Attribute, Config.DotComparisonDetails.ThresholdAttribute, Consumable)

	return true;
}

bool PCGExPointFilter::FTensorDotFilter::Init(FPCGExContext* InContext, const TSharedPtr<PCGExData::FFacade>& InPointDataFacade)
{
	if (!FFilter::Init(InContext, InPointDataFacade)) { return false; }

	TensorsHandler = MakeShared<PCGExTensor::FTensorsHandler>(TypedFilterFactory->Config.TensorHandlerDetails);
	if (!TensorsHandler->Init(InContext, TypedFilterFactory->TensorFactories, InPointDataFacade)) { return false; }

	OperandA = PointDataFacade->GetScopedBroadcaster<FVector>(TypedFilterFactory->Config.OperandA);
	if (!OperandA)
	{
		PCGEX_LOG_INVALID_SELECTOR_C(InContext, "Operand A", TypedFilterFactory->Config.OperandA)
		return false;
	}

	// TODO : Validate tensor factories

	return true;
}

bool PCGExPointFilter::FTensorDotFilter::Test(const int32 PointIndex) const
{
	const FPCGPoint& Point = PointDataFacade->Source->GetInPoint(PointIndex);

	bool bSuccess = false;
	const PCGExTensor::FTensorSample Sample = TensorsHandler->Sample(PointIndex, Point.Transform, bSuccess);

	if (!bSuccess) { return false; }

	return DotComparison.Test(
		FVector::DotProduct(
			TypedFilterFactory->Config.bTransformOperandA ? OperandA->Read(PointIndex) : Point.Transform.TransformVectorNoScale(OperandA->Read(PointIndex)),
			Sample.DirectionAndSize.GetSafeNormal()),
		DotComparison.GetComparisonThreshold(PointIndex));
}

PCGEX_CREATE_FILTER_FACTORY(TensorDot)

#if WITH_EDITOR
FString UPCGExTensorDotFilterProviderSettings::GetDisplayName() const
{
	FString DisplayName = PCGEx::GetSelectorDisplayName(Config.OperandA) + " ⋅ Tensor";
	return DisplayName;
}
#endif

#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
