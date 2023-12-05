﻿// Copyright Timothé Lapetite 2023
// Released under the MIT license https://opensource.org/license/MIT/

#include "Misc/PCGExWriteIndex.h"

#define LOCTEXT_NAMESPACE "PCGExWriteIndexElement"

PCGExPointIO::EInit UPCGExWriteIndexSettings::GetPointOutputInitMode() const { return PCGExPointIO::EInit::DuplicateInput; }

FPCGElementPtr UPCGExWriteIndexSettings::CreateElement() const { return MakeShared<FPCGExWriteIndexElement>(); }

FPCGContext* FPCGExWriteIndexElement::Initialize(const FPCGDataCollection& InputData, TWeakObjectPtr<UPCGComponent> SourceComponent, const UPCGNode* Node)
{
	FPCGExWriteIndexContext* Context = new FPCGExWriteIndexContext();
	InitializeContext(Context, InputData, SourceComponent, Node);
	return Context;
}

bool FPCGExWriteIndexElement::Validate(FPCGContext* InContext) const
{
	if (!FPCGExPointsProcessorElementBase::Validate(InContext)) { return false; }

	FPCGExWriteIndexContext* Context = static_cast<FPCGExWriteIndexContext*>(InContext);

	const UPCGExWriteIndexSettings* Settings = Context->GetInputSettings<UPCGExWriteIndexSettings>();
	check(Settings);

	const FName OutName = Settings->OutputAttributeName;
	if (!FPCGMetadataAttributeBase::IsValidName(OutName))
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("InvalidName", "Output name is invalid."));
		return false;
	}

	Context->bOutputNormalizedIndex = Settings->bOutputNormalizedIndex;
	Context->OutName = Settings->OutputAttributeName;
	return true;
}


bool FPCGExWriteIndexElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExWriteIndexElement::Execute);

	FPCGExWriteIndexContext* Context = static_cast<FPCGExWriteIndexContext*>(InContext);

	if (Context->IsSetup())
	{
		if (!Validate(Context)) { return true; }
		Context->SetState(PCGExMT::State_ReadyForNextPoints);
	}

	if (Context->IsState(PCGExMT::State_ReadyForNextPoints))
	{
		Context->SetState(PCGExMT::State_ProcessingPoints);
	}

	if (Context->bOutputNormalizedIndex)
	{
		if (Context->IsState(PCGExMT::State_ProcessingPoints))
		{
			auto Initialize = [&](UPCGExPointIO* PointIO)
			{
				FWriteScopeLock WriteLock(Context->MapLock);
				PointIO->BuildMetadataEntries();
				FPCGMetadataAttribute<double>* IndexAttribute = PointIO->Out->Metadata->FindOrCreateAttribute<double>(Context->OutName, 0, false);
				Context->NormalizedAttributeMap.Add(PointIO, IndexAttribute);
			};

			auto ProcessPoint = [&](const int32 Index, const UPCGExPointIO* PointIO)
			{
				const FPCGPoint& Point = PointIO->GetOutPoint(Index);
				FPCGMetadataAttribute<double>* IndexAttribute = *(Context->NormalizedAttributeMap.Find(PointIO));
				IndexAttribute->SetValue(Point.MetadataEntry, static_cast<double>(Index) / static_cast<double>(PointIO->NumInPoints));
			};


			if (Context->BulkProcessMainPoints(Initialize, ProcessPoint)) { Context->Done(); }
		}
	}
	else
	{
		if (Context->IsState(PCGExMT::State_ProcessingPoints))
		{
			auto Initialize = [&](UPCGExPointIO* PointIO)
			{
				FWriteScopeLock WriteLock(Context->MapLock);
				PointIO->BuildMetadataEntries();
				FPCGMetadataAttribute<int64>* IndexAttribute = PointIO->Out->Metadata->FindOrCreateAttribute<int64>(Context->OutName, -1, false);
				Context->AttributeMap.Add(PointIO, IndexAttribute);
			};

			auto ProcessPoint = [&](const int32 Index, const UPCGExPointIO* PointIO)
			{
				const FPCGPoint& Point = PointIO->GetOutPoint(Index);
				FPCGMetadataAttribute<int64>* IndexAttribute = *(Context->AttributeMap.Find(PointIO));
				check(IndexAttribute);
				IndexAttribute->SetValue(Point.MetadataEntry, Index);
			};


			if (Context->BulkProcessMainPoints(Initialize, ProcessPoint)) { Context->Done(); }
		}
	}


	if (Context->IsDone())
	{
		Context->OutputPoints();
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
