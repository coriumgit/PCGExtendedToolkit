﻿// Copyright Timothé Lapetite 2024
// Released under the MIT license https://opensource.org/license/MIT/


#include "Graph/Pathfinding/Heuristics/PCGExHeuristicDistance.h"

void UPCGExHeuristicDistance::PrepareForData(PCGExCluster::FCluster* InCluster)
{
	MaxDistSquared = FVector::DistSquared(InCluster->Bounds.Min, InCluster->Bounds.Max);
	Super::PrepareForData(InCluster);
}

double UPCGExHeuristicDistance::GetGlobalScore(
	const PCGExCluster::FNode& From,
	const PCGExCluster::FNode& Seed,
	const PCGExCluster::FNode& Goal) const
{
	return (FVector::DistSquared(From.Position, Goal.Position) / MaxDistSquared) * ReferenceWeight;
}

UPCGExHeuristicOperation* UPCGHeuristicsFactoryShortestDistance::CreateOperation() const
{
	UPCGExHeuristicDistance* NewOperation = NewObject<UPCGExHeuristicDistance>();
	return NewOperation;
}

UPCGExParamFactoryBase* UPCGExHeuristicsShortestDistanceProviderSettings::CreateFactory(FPCGContext* InContext, UPCGExParamFactoryBase* InFactory) const
{
	UPCGHeuristicsFactoryShortestDistance* NewHeuristics = NewObject<UPCGHeuristicsFactoryShortestDistance>();
	NewHeuristics->WeightFactor = Descriptor.WeightFactor;
	return Super::CreateFactory(InContext, NewHeuristics);
}