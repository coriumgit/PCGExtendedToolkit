﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/


#include "Paths/SubPoints/PCGExSubPointsOperation.h"

#include "Data/PCGExPointIO.h"


void UPCGExSubPointsOperation::CopySettingsFrom(const UPCGExOperation* Other)
{
	Super::CopySettingsFrom(Other);
	if (const UPCGExSubPointsOperation* TypedOther = Cast<UPCGExSubPointsOperation>(Other))
	{
		bClosedPath = TypedOther->bClosedPath;
	}
}

void UPCGExSubPointsOperation::PrepareForData(PCGExData::FFacade* InPrimaryFacade)
{
}

void UPCGExSubPointsOperation::ProcessSubPoints(
	const PCGExData::FPointRef& From,
	const PCGExData::FPointRef& To,
	const TArrayView<FPCGPoint>& SubPoints,
	const PCGExPaths::FPathMetrics& Metrics,
	const int32 StartIndex) const
{
}
