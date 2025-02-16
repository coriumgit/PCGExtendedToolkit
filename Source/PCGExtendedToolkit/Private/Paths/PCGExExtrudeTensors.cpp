﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#include "Paths/PCGExExtrudeTensors.h"

#include "Data/PCGExData.h"


#define LOCTEXT_NAMESPACE "PCGExExtrudeTensorsElement"
#define PCGEX_NAMESPACE ExtrudeTensors

TArray<FPCGPinProperties> UPCGExExtrudeTensorsSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::InputPinProperties();
	PCGEX_PIN_FACTORIES(PCGExTensor::SourceTensorsLabel, "Tensors", Required, {})
	PCGEX_PIN_FACTORIES(PCGExPointFilter::SourceStopConditionLabel, "Extruded points will be tested against those filters. If a filter returns true, the extrusion point is considered 'out-of-bounds'.", Normal, {})

	if (bDoExternalPathIntersections) { PCGEX_PIN_POINTS(PCGExPaths::SourcePathsLabel, "Paths that will be checked for intersections while extruding.", Normal, {}) }
	else { PCGEX_PIN_POINTS(PCGExPaths::SourcePathsLabel, "(This is only there to preserve connections, enable it in the settings.)", Advanced, {}) }

	if (bDoSelfPathIntersections) { PCGEX_PIN_FACTORIES(PCGExSorting::SourceSortingRules, "Plug sorting rules here. Order is defined by each rule' priority value, in ascending order.", Normal, {}) }
	else { PCGEX_PIN_FACTORIES(PCGExSorting::SourceSortingRules, "(This is only there to preserve connections, enable it in the settings.)", Advanced, {}) }

	return PinProperties;
}

bool UPCGExExtrudeTensorsSettings::GetSortingRules(FPCGExContext* InContext, TArray<FPCGExSortRuleConfig>& OutRules) const
{
	OutRules.Append(PCGExSorting::GetSortingRules(InContext, PCGExSorting::SourceSortingRules));
	return !OutRules.IsEmpty();
}

PCGEX_INITIALIZE_ELEMENT(ExtrudeTensors)

FName UPCGExExtrudeTensorsSettings::GetMainInputPin() const { return PCGExGraph::SourceSeedsLabel; }
FName UPCGExExtrudeTensorsSettings::GetMainOutputPin() const { return PCGExPaths::OutputPathsLabel; }

bool FPCGExExtrudeTensorsElement::Boot(FPCGExContext* InContext) const
{
	if (!FPCGExPointsProcessorElement::Boot(InContext)) { return false; }

	PCGEX_CONTEXT_AND_SETTINGS(ExtrudeTensors)

	PCGEX_FWD(ClosedLoop)
	Context->ClosedLoop.Init();

	PCGEX_FWD(ExternalPathIntersections)
	Context->ExternalPathIntersections.Init();

	PCGEX_FWD(SelfPathIntersections)
	Context->SelfPathIntersections.Init();

	if (!PCGExFactories::GetInputFactories(InContext, PCGExTensor::SourceTensorsLabel, Context->TensorFactories, {PCGExFactories::EType::Tensor}, true)) { return false; }

	GetInputFactories(Context, PCGExPointFilter::SourceStopConditionLabel, Context->StopFilterFactories, PCGExFactories::PointFilters, false);
	PCGExPointFilter::PruneForDirectEvaluation(Context, Context->StopFilterFactories);

	if (Context->TensorFactories.IsEmpty())
	{
		if (!Settings->bQuietMissingTensorError) { PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Missing tensors.")); }
		return false;
	}

	Context->ClosedLoopSquaredDistance = FMath::Square(Settings->ClosedLoopSearchDistance);
	Context->ClosedLoopSearchDot = PCGExMath::DegreesToDot(Settings->ClosedLoopSearchAngle);

	return true;
}

bool FPCGExExtrudeTensorsElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGExExtrudeTensorsElement::Execute);

	PCGEX_CONTEXT_AND_SETTINGS(ExtrudeTensors)
	PCGEX_EXECUTION_CHECK
	PCGEX_ON_INITIAL_EXECUTION
	{
		Context->AddConsumableAttributeName(Settings->IterationsAttribute);

		if (!Context->StartBatchProcessingPoints<PCGExExtrudeTensors::FBatch>(
			[&](const TSharedPtr<PCGExData::FPointIO>& Entry) { return true; },
			[&](const TSharedPtr<PCGExExtrudeTensors::FBatch>& NewBatch)
			{
				NewBatch->bPrefetchData = true;
			}))
		{
			return Context->CancelExecution(TEXT("Could not find any paths to subdivide."));
		}
	}

	PCGEX_POINTS_BATCH_PROCESSING(PCGEx::State_Done)

	Context->MainPoints->StageOutputs();

	return Context->TryComplete();
}

namespace PCGExExtrudeTensors
{
	FExtrusion::FExtrusion(const int32 InSeedIndex, const TSharedRef<PCGExData::FFacade>& InFacade, const int32 InMaxIterations) :
		ExtrudedPoints(InFacade->GetOut()->GetMutablePoints()), SeedIndex(InSeedIndex), RemainingIterations(InMaxIterations), PointDataFacade(InFacade)
	{
		ExtrudedPoints.Reserve(InMaxIterations);
		Origin = InFacade->Source->GetInPoint(SeedIndex);
		ExtrudedPoints.Add(Origin);
		SetHead(Origin.Transform);
	}

	void FExtrusion::SetHead(const FTransform& InHead)
	{
		LastInsertion = InHead.GetLocation();
		Head = InHead;
		ExtrudedPoints.Last().Transform = Head;
		Metrics = PCGExPaths::FPathMetrics(LastInsertion);
		Bounds = FBox(ForceInit);
		Bounds += (Metrics.Last + FVector::OneVector * 10);
		Bounds += (Metrics.Last + FVector::OneVector * -10);
	}

	void FExtrusion::Complete()
	{
		if (bIsComplete || bIsStopped) { return; }

		bIsComplete = true;

		ExtrudedPoints.Shrink();
		if (ExtrudedPoints.Num() <= 1)
		{
			PointDataFacade->Source->InitializeOutput(PCGExData::EIOInit::None);
			PointDataFacade->Source->Disable();
			return;
		}

		if (!bIsClosedLoop) { if (Settings->bTagIfOpenPath) { PointDataFacade->Source->Tags->AddRaw(Settings->IsOpenPathTag); } }
		else { if (Settings->bTagIfClosedLoop) { PointDataFacade->Source->Tags->AddRaw(Settings->IsClosedLoopTag); } }

		if (Settings->bTagIfIsStoppedByFilters && bHitStopFilters) { PointDataFacade->Source->Tags->AddRaw(Settings->IsStoppedByFiltersTag); }
		if (bHitIntersection)
		{
			// TODO : Grab data from intersection
			if (Settings->bTagIfIsStoppedByIntersection) { PointDataFacade->Source->Tags->AddRaw(Settings->IsStoppedByIntersectionTag); }
		}
		if (Settings->bTagIfChildExtrusion && bIsChildExtrusion) { PointDataFacade->Source->Tags->AddRaw(Settings->IsChildExtrusionTag); }
		if (Settings->bTagIfIsFollowUp && bIsFollowUp) { PointDataFacade->Source->Tags->AddRaw(Settings->IsFollowUpTag); }

		PointDataFacade->Source->GetOutKeys(true);
	}

	void FExtrusion::Cutoff(const FVector& InCutOff)
	{
		ExtrudedPoints.Last().Transform.SetLocation(InCutOff);
		Complete();
		bHitIntersection = true;
		bHitSelfIntersection = true;
		bIsStopped = true;
	}

	bool FExtrusion::GetHeadEdge(FVector& OutA, FVector& OutB) const
	{
		if (ExtrudedPoints.IsEmpty()) { return false; }
		OutA = ExtrudedPoints.Last(1).Transform.GetLocation();
		OutB = ExtrudedPoints.Last().Transform.GetLocation();
		return true; //FMath::IsNearlyZero(FVector::DistSquared(OutA, OutB));
	}

	void FExtrusion::StartNewExtrusion()
	{
		if (RemainingIterations > 1)
		{
			// We re-entered bounds from a previously completed path.
			if (const TSharedPtr<FExtrusion> ChildExtrusion = Processor->InitExtrusionFromExtrusion(SharedThis(this)))
			{
				ChildExtrusion->bIsChildExtrusion = true;
				ChildExtrusion->bIsFollowUp = bIsComplete;
			}
		}
	}

	bool FExtrusion::OnAdvanced(const bool bStop)
	{
		RemainingIterations--;

		if (bStop || RemainingIterations <= 0)
		{
			Complete();
			bIsStopped = true;
		}

		return !bIsStopped;
	}

	void FExtrusion::Insert(const FPCGPoint& InPoint)
	{
		FPCGPoint& NewPoint = ExtrudedPoints.Add_GetRef(InPoint);
		LastInsertion = NewPoint.Transform.GetLocation();
		if (Settings->bRefreshSeed) { NewPoint.Seed = PCGExRandom::ComputeSeed(NewPoint, FVector(Origin.Seed)); }
	}

	FProcessor::~FProcessor()
	{
	}

	void FProcessor::RegisterBuffersDependencies(PCGExData::FFacadePreloader& FacadePreloader)
	{
		TPointsProcessor<FPCGExExtrudeTensorsContext, UPCGExExtrudeTensorsSettings>::RegisterBuffersDependencies(FacadePreloader);

		TArray<FPCGExSortRuleConfig> RuleConfigs;
		if (Settings->GetSortingRules(ExecutionContext, RuleConfigs) && !RuleConfigs.IsEmpty())
		{
			Sorter = MakeShared<PCGExSorting::PointSorter<true>>(Context, PointDataFacade, RuleConfigs);
			Sorter->SortDirection = Settings->SortDirection;
			Sorter->RegisterBuffersDependencies(FacadePreloader);
		}
	}

	bool FProcessor::Process(const TSharedPtr<PCGExMT::FTaskManager>& InAsyncManager)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PCGExExtrudeTensors::Process);

		if (!FPointsProcessor::Process(InAsyncManager)) { return false; }

		if (Sorter && !Sorter->Init()) { Sorter.Reset(); }

		StaticPaths = MakeShared<TArray<TSharedPtr<PCGExPaths::FPath>>>();

		if (!Context->StopFilterFactories.IsEmpty())
		{
			StopFilters = MakeShared<PCGExPointFilter::FManager>(PointDataFacade);
			if (!StopFilters->Init(Context, Context->StopFilterFactories)) { StopFilters.Reset(); }
		}

		TensorsHandler = MakeShared<PCGExTensor::FTensorsHandler>(Settings->TensorHandlerDetails);
		if (!TensorsHandler->Init(Context, Context->TensorFactories, PointDataFacade)) { return false; }

		AttributesToPathTags = Settings->AttributesToPathTags;
		if (!AttributesToPathTags.Init(Context, PointDataFacade)) { return false; }

		if (Settings->bUsePerPointMaxIterations)
		{
			PerPointIterations = PointDataFacade->GetBroadcaster<int32>(Settings->IterationsAttribute, true);
			if (!PerPointIterations)
			{
				PCGE_LOG_C(Warning, GraphAndLog, Context, FTEXT("Iteration attribute is missing on some inputs, they will be ignored."));
				return false;
			}

			if (Settings->bUseMaxFromPoints) { RemainingIterations = FMath::Max(RemainingIterations, PerPointIterations->Max); }
		}
		else
		{
			RemainingIterations = Settings->Iterations;
		}

		if (Settings->bUseMaxLength && Settings->MaxLengthInput == EPCGExInputValueType::Attribute)
		{
			PerPointMaxLength = PointDataFacade->GetBroadcaster<double>(Settings->MaxLengthAttribute);
			if (!PerPointMaxLength)
			{
				PCGE_LOG_C(Warning, GraphAndLog, Context, FTEXT("Max length attribute is missing on some inputs, they will be ignored."));
				return false;
			}
		}

		if (Settings->bUseMaxPointsCount && Settings->MaxPointsCountInput == EPCGExInputValueType::Attribute)
		{
			PerPointMaxPoints = PointDataFacade->GetBroadcaster<int32>(Settings->MaxPointsCountAttribute);
			if (!PerPointMaxPoints)
			{
				PCGE_LOG_C(Warning, GraphAndLog, Context, FTEXT("Max point count attribute is missing on some inputs, they will be ignored."));
				return false;
			}
		}

		const int32 NumPoints = PointDataFacade->GetNum();
		PCGEx::InitArray(ExtrusionQueue, NumPoints);
		PointFilterCache.Init(true, NumPoints);

		Context->MainPoints->IncreaseReserve(NumPoints);

		StartParallelLoopForPoints(PCGExData::ESource::In);

		return true;
	}

	void FProcessor::InitExtrusionFromSeed(const int32 InSeedIndex)
	{
		const int32 Iterations = PerPointIterations ? PerPointIterations->Read(InSeedIndex) : Settings->Iterations;
		if (Iterations < 1) { return; }

		bool bIsStopped = false;
		if (StopFilters)
		{
			bIsStopped = StopFilters->Test(PointDataFacade->Source->GetInPoint(InSeedIndex));
			if (Settings->bIgnoreStoppedSeeds && bIsStopped) { return; }
		}

		const TSharedPtr<FExtrusion> NewExtrusion = CreateExtrusionTemplate(InSeedIndex, Iterations);
		if (!NewExtrusion) { return; }

		NewExtrusion->bIsProbe = bIsStopped;
		if (Settings->bUseMaxLength) { NewExtrusion->MaxLength = PerPointMaxLength ? PerPointMaxLength->Read(InSeedIndex) : Settings->MaxLength; }
		if (Settings->bUseMaxPointsCount) { NewExtrusion->MaxPointCount = PerPointMaxPoints ? PerPointMaxPoints->Read(InSeedIndex) : Settings->MaxPointsCount; }

		ExtrusionQueue[InSeedIndex] = NewExtrusion;
	}

	TSharedPtr<FExtrusion> FProcessor::InitExtrusionFromExtrusion(const TSharedRef<FExtrusion>& InExtrusion)
	{
		if (!Settings->bAllowChildExtrusions) { return nullptr; }

		const TSharedPtr<FExtrusion> NewExtrusion = CreateExtrusionTemplate(InExtrusion->SeedIndex, InExtrusion->RemainingIterations);
		if (!NewExtrusion) { return nullptr; }

		NewExtrusion->SetHead(InExtrusion->Head);

		{
			FWriteScopeLock WriteScopeLock(NewExtrusionLock);
			NewExtrusions.Add(NewExtrusion);
		}

		return NewExtrusion;
	}

	void FProcessor::PrepareLoopScopesForRanges(const TArray<PCGExMT::FScope>& Loops)
	{
		CompletedExtrusions = MakeShared<PCGExMT::TScopedArray<TSharedPtr<FExtrusion>>>(Loops);
	}

	void FProcessor::PrepareSingleLoopScopeForPoints(const PCGExMT::FScope& Scope)
	{
		PointDataFacade->Fetch(Scope);
		//FilterScope(Scope);
	}

	void FProcessor::ProcessSinglePoint(const int32 Index, FPCGPoint& Point, const PCGExMT::FScope& Scope)
	{
		//if (!PointFilterCache[Index]) { return; }
		InitExtrusionFromSeed(Index);
	}

	void FProcessor::OnPointsProcessingComplete()
	{
		if (UpdateExtrusionQueue()) { StartParallelLoopForRange(ExtrusionQueue.Num(), 32); }
	}

	void FProcessor::ProcessSingleRangeIteration(const int32 Iteration, const PCGExMT::FScope& Scope)
	{
		if (!ExtrusionQueue[Iteration]->Advance())
		{
			ExtrusionQueue[Iteration]->Complete();
			CompletedExtrusions->Get(Scope)->Add(ExtrusionQueue[Iteration]);
		}
	}

	void FProcessor::OnRangeProcessingComplete()
	{
		RemainingIterations--;

		if (Settings->bDoSelfPathIntersections)
		{
			// TODO : Sort extrusion queue using sorter?
			// TODO : Test last edge of all ongoing extrusions

			FVector A = FVector::ZeroVector;
			FVector B = FVector::ZeroVector;

			FVector A1 = FVector::ZeroVector;
			FVector B1 = FVector::ZeroVector;

			FVector C = FVector::ZeroVector;
			double BestDistance = MAX_dbl;

			if (Sorter)
			{
				ExtrusionQueue.Sort(
					[S = Sorter](const TSharedPtr<FExtrusion>& EA, const TSharedPtr<FExtrusion>& EB)
					{
						return S->Sort(EA->SeedIndex, EB->SeedIndex);
					});
			}

			for (int i = 0; i < ExtrusionQueue.Num(); i++)
			{
				TSharedPtr<FExtrusion> E = ExtrusionQueue[i];

				if (E->bAdvancedOnly || !E->GetHeadEdge(A1, B1)) { continue; }

				FBox EdgeBox = FBox(ForceInit);
				EdgeBox += A1;
				EdgeBox += B1;
				EdgeBox = EdgeBox.ExpandBy(Settings->SelfPathIntersections.ToleranceSquared);

				const FVector Dir = (B1 - A1).GetSafeNormal();
				bool bIsStopped = false;

				for (int j = 0; j < ExtrusionQueue.Num(); j++)
				{
					if (i == j) { continue; }

					TSharedPtr<FExtrusion> OE = ExtrusionQueue[j];

					if (!OE->bIsExtruding) { continue; }

					const TArray<FPCGPoint>& EPts = OE->GetExtrudedPoints();
					const int32 SegmentNum = EPts.Num();

					if (SegmentNum < 1) { continue; }
					if (!OE->Bounds.Intersect(EdgeBox)) { continue; }

					FVector B2 = EPts[0].Transform.GetLocation();

					for (int k = 1; k < SegmentNum; k++)
					{
						const FVector A2 = EPts[k].Transform.GetLocation();
						ON_SCOPE_EXIT { B2 = A2; };

						FBox OEBox = FBox(ForceInit);
						OEBox += A2;
						OEBox += B2;
						OEBox = OEBox.ExpandBy(Settings->SelfPathIntersections.ToleranceSquared);

						if (!EdgeBox.Intersect(OEBox)) { continue; }

						if (Settings->SelfPathIntersections.bUseMinAngle || Settings->SelfPathIntersections.bUseMaxAngle)
						{
							const FVector ODir = (B2 - A2).GetSafeNormal();
							if (!Settings->SelfPathIntersections.CheckDot(FMath::Abs(FVector::DotProduct(ODir, Dir)))) { continue; }
						}

						FMath::SegmentDistToSegment(A1, B1, A2, B2, A, B);

						if (k == SegmentNum - 1 && OE->bHitSelfIntersection)
						{
							// Ignore already cut-off
							if (B == B2 || B == A2) { continue; }
						}

						if (FVector::DistSquared(A, B) >= Settings->SelfPathIntersections.ToleranceSquared) { continue; }

						if (!bIsStopped)
						{
							bIsStopped = true;
							C = B;
							BestDistance = FVector::DistSquared(A1, C);
						}
						else if (const double Dist = FVector::DistSquared(A1, B); Dist < BestDistance)
						{
							C = B;
							BestDistance = Dist;
						}
					}
				}

				if (bIsStopped)
				{
					ExtrusionQueue[i]->Cutoff(C);
					CompletedExtrusions->Values[0]->Add(E);
				}
			}
		}

		// TODO : If detecting collisions is enabled, start detection loop here
		// Test only with last edge of each extrusion against all others extrusions including itself
		if (UpdateExtrusionQueue()) { StartParallelLoopForRange(ExtrusionQueue.Num(), 32); }
	}

	bool FProcessor::UpdateExtrusionQueue()
	{
		if (RemainingIterations <= 0) { return false; }

		int32 WriteIndex = 0;
		for (int i = 0; i < ExtrusionQueue.Num(); i++)
		{
			if (!ExtrusionQueue[i]->bIsComplete) { ExtrusionQueue[WriteIndex++] = ExtrusionQueue[i]; }
		}

		ExtrusionQueue.SetNum(WriteIndex);

		if (!NewExtrusions.IsEmpty())
		{
			ExtrusionQueue.Reserve(ExtrusionQueue.Num() + NewExtrusions.Num());
			ExtrusionQueue.Append(NewExtrusions);
			NewExtrusions.Reset();
		}

		if (ExtrusionQueue.IsEmpty()) { return false; }

		// Convert completed paths to static collision constraints
		if (Settings->bDoSelfPathIntersections && CompletedExtrusions)
		{
			CompletedExtrusions->ForEach(
				[&](TArray<TSharedPtr<FExtrusion>>& Completed)
				{
					StaticPaths.Get()->Reserve(StaticPaths.Get()->Num() + Completed.Num());
					for (const TSharedPtr<FExtrusion>& E : Completed)
					{
						TSharedPtr<PCGExPaths::FPath> StaticPath = PCGExPaths::MakePath(E->GetExtrudedPoints(), Settings->ExternalPathIntersections.ToleranceSquared, false);
						StaticPath->BuildEdgeOctree();
						StaticPaths.Get()->Add(StaticPath);
					}
				});

			CompletedExtrusions.Reset();
		}

		return true;
	}

	void FProcessor::CompleteWork()
	{
		for (const TSharedPtr<FExtrusion>& E : ExtrusionQueue) { if (E) { E->Complete(); } }
		CompletedExtrusions.Reset();
		ExtrusionQueue.Empty();
		StaticPaths->Empty();
	}

	TSharedPtr<FExtrusion> FProcessor::CreateExtrusionTemplate(const int32 InSeedIndex, const int32 InMaxIterations)
	{
		const TSharedPtr<PCGExData::FPointIO> NewIO = Context->MainPoints->Emplace_GetRef(PointDataFacade->Source->GetIn(), PCGExData::EIOInit::None);
		if (!NewIO) { return nullptr; }

		PCGEX_MAKE_SHARED(Facade, PCGExData::FFacade, NewIO.ToSharedRef());
		if (!Facade->Source->InitializeOutput(PCGExData::EIOInit::New)) { return nullptr; }

		TSharedPtr<FExtrusion> NewExtrusion = nullptr;

#define PCGEX_NEW_EXTRUSION(_FLAGS) NewExtrusion = MakeShared<TExtrusion<_FLAGS>>(InSeedIndex, Facade.ToSharedRef(), InMaxIterations);
#define PCGEX_1_FLAGS_CASE(_A) case EExtrusionFlags::_A : PCGEX_NEW_EXTRUSION(EExtrusionFlags::_A) break;
#define PCGEX_2_FLAGS(_A, _B) static_cast<EExtrusionFlags>(static_cast<uint32>(EExtrusionFlags::_A) | static_cast<uint32>(EExtrusionFlags::_B))
#define PCGEX_2_FLAGS_CASE(_A, _B) case PCGEX_2_FLAGS(_A, _B) : PCGEX_NEW_EXTRUSION(PCGEX_2_FLAGS(_A, _B)) break;
#define PCGEX_3_FLAGS(_A, _B, _C) static_cast<EExtrusionFlags>(static_cast<uint32>(EExtrusionFlags::_A) | static_cast<uint32>(EExtrusionFlags::_B) | static_cast<uint32>(EExtrusionFlags::_C))
#define PCGEX_3_FLAGS_CASE(_A, _B, _C) case PCGEX_3_FLAGS(_A, _B, _C) : PCGEX_NEW_EXTRUSION(PCGEX_3_FLAGS(_A, _B, _C)) break;
#define PCGEX_4_FLAGS(_A, _B, _C, _D) static_cast<EExtrusionFlags>(static_cast<uint32>(EExtrusionFlags::_A) | static_cast<uint32>(EExtrusionFlags::_B) | static_cast<uint32>(EExtrusionFlags::_C) | static_cast<uint32>(EExtrusionFlags::_D))
#define PCGEX_4_FLAGS_CASE(_A, _B, _C, _D) case PCGEX_4_FLAGS(_A, _B, _C, _D) : PCGEX_NEW_EXTRUSION(PCGEX_4_FLAGS(_A, _B, _C, _D)) break;

		switch (ComputeFlags())
		{
		PCGEX_1_FLAGS_CASE(None)
		PCGEX_1_FLAGS_CASE(Bounded)
		PCGEX_1_FLAGS_CASE(AllowsChildren)
		PCGEX_1_FLAGS_CASE(ClosedLoop)
		PCGEX_1_FLAGS_CASE(CollisionCheck)
		PCGEX_2_FLAGS_CASE(Bounded, AllowsChildren)
		PCGEX_2_FLAGS_CASE(Bounded, ClosedLoop)
		PCGEX_2_FLAGS_CASE(AllowsChildren, ClosedLoop)
		PCGEX_2_FLAGS_CASE(Bounded, CollisionCheck)
		PCGEX_2_FLAGS_CASE(ClosedLoop, CollisionCheck)
		PCGEX_2_FLAGS_CASE(AllowsChildren, CollisionCheck)
		PCGEX_3_FLAGS_CASE(Bounded, AllowsChildren, ClosedLoop)
		PCGEX_3_FLAGS_CASE(CollisionCheck, AllowsChildren, ClosedLoop)
		PCGEX_3_FLAGS_CASE(Bounded, CollisionCheck, ClosedLoop)
		PCGEX_3_FLAGS_CASE(Bounded, AllowsChildren, CollisionCheck)
		PCGEX_4_FLAGS_CASE(Bounded, AllowsChildren, ClosedLoop, CollisionCheck)
		default:
			checkNoEntry(); // You missed flags dummy
			break;
		}

#undef PCGEX_NEW_EXTRUSION
#undef PCGEX_1_FLAGS_CASE
#undef PCGEX_2_FLAGS
#undef PCGEX_2_FLAGS_CASE
#undef PCGEX_3_FLAGS
#undef PCGEX_3_FLAGS_CASE
#undef PCGEX_4_FLAGS
#undef PCGEX_4_FLAGS_CASE

		if (Settings->bUseMaxLength) { NewExtrusion->MaxLength = PerPointMaxLength ? PerPointMaxLength->Read(InSeedIndex) : Settings->MaxLength; }
		if (Settings->bUseMaxPointsCount) { NewExtrusion->MaxPointCount = PerPointMaxPoints ? PerPointMaxPoints->Read(InSeedIndex) : Settings->MaxPointsCount; }

		NewExtrusion->PointDataFacade->Source->IOIndex = BatchIndex * 1000000 + InSeedIndex;
		AttributesToPathTags.Tag(InSeedIndex, Facade->Source);

		NewExtrusion->Processor = this;
		NewExtrusion->Context = Context;
		NewExtrusion->Settings = Settings;
		NewExtrusion->TensorsHandler = TensorsHandler;
		NewExtrusion->StopFilters = StopFilters;
		NewExtrusion->StaticPaths = StaticPaths;

		return NewExtrusion;
	}

	FBatch::FBatch(FPCGExContext* InContext, const TArray<TWeakPtr<PCGExData::FPointIO>>& InPointsCollection)
		: TBatch<FProcessor>(InContext, InPointsCollection)
	{
	}

	void FBatch::Process(TSharedPtr<PCGExMT::FTaskManager> InAsyncManager)
	{
		AsyncManager = InAsyncManager;

		PCGEX_TYPED_CONTEXT_AND_SETTINGS(ExtrudeTensors)

		if (Settings->bDoExternalPathIntersections)
		{
			if (TryGetFacades(Context, PCGExPaths::SourcePathsLabel, Context->PathsFacades, false, true))
			{
				Context->ExternalPaths.Reserve(Context->PathsFacades.Num());
				for (const TSharedPtr<PCGExData::FFacade>& Facade : Context->PathsFacades)
				{
					TSharedPtr<PCGExPaths::FPath> Path = PCGExPaths::MakePath(
						Facade->GetIn()->GetPoints(),
						Settings->ExternalPathIntersections.ToleranceSquared,
						Context->ClosedLoop.IsClosedLoop(Facade->Source));

					Context->ExternalPaths.Add(Path);
					Path->BuildEdgeOctree();
				}
			}
		}

		OnPathsPrepared();
	}

	void FBatch::OnPathsPrepared()
	{
		TBatch<FProcessor>::Process(AsyncManager);
	}
}


#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
