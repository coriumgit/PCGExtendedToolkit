﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#include "Graph/Edges/Properties/PCGExVtxPropertyAmplitude.h"

#include "PCGPin.h"
#include "Sampling/PCGExSampling.h"


#define LOCTEXT_NAMESPACE "PCGExVtxPropertyAmplitude"
#define PCGEX_NAMESPACE PCGExVtxPropertyAmplitude

bool FPCGExAmplitudeConfig::Validate(const FPCGExContext* InContext) const
{
#define PCGEX_VALIDATE_AMP_NAME(_NAME) if(bWrite##_NAME){ PCGEX_VALIDATE_NAME_C(InContext, _NAME##AttributeName) }

	PCGEX_VALIDATE_AMP_NAME(MinAmplitude)
	PCGEX_VALIDATE_AMP_NAME(MaxAmplitude)
	PCGEX_VALIDATE_AMP_NAME(AmplitudeRange)
	PCGEX_VALIDATE_AMP_NAME(AmplitudeSign)

#undef PCGEX_VALIDATE_AMP_NAME
	return true;
}

void UPCGExVtxPropertyAmplitude::CopySettingsFrom(const UPCGExOperation* Other)
{
	Super::CopySettingsFrom(Other);
	if (const UPCGExVtxPropertyAmplitude* TypedOther = Cast<UPCGExVtxPropertyAmplitude>(Other))
	{
		Config = TypedOther->Config;
	}
}

bool UPCGExVtxPropertyAmplitude::PrepareForCluster(const FPCGExContext* InContext, TSharedPtr<PCGExCluster::FCluster> InCluster, const TSharedPtr<PCGExData::FFacade>& InVtxDataFacade, const TSharedPtr<PCGExData::FFacade>& InEdgeDataFacade)
{
	if (!Super::PrepareForCluster(InContext, InCluster, InVtxDataFacade, InEdgeDataFacade)) { return false; }

	if (!Config.Validate(InContext))
	{
		bIsValidOperation = false;
		return false;
	}

	if (Config.bWriteMinAmplitude)
	{
		if (Config.MinMode == EPCGExVtxAmplitudeMode::Length)
		{
			MinAmpLengthBuffer = InVtxDataFacade->GetWritable<double>(Config.MinAmplitudeAttributeName, 0, true, PCGExData::EBufferInit::New);
		}
		else
		{
			MinAmpBuffer = InVtxDataFacade->GetWritable<FVector>(Config.MinAmplitudeAttributeName, FVector::ZeroVector, true, PCGExData::EBufferInit::New);
		}
	}

	if (Config.bWriteMaxAmplitude)
	{
		if (Config.MaxMode == EPCGExVtxAmplitudeMode::Length)
		{
			MaxAmpLengthBuffer = InVtxDataFacade->GetWritable<double>(Config.MaxAmplitudeAttributeName, 0, true, PCGExData::EBufferInit::New);
		}
		else
		{
			MaxAmpBuffer = InVtxDataFacade->GetWritable<FVector>(Config.MaxAmplitudeAttributeName, FVector::ZeroVector, true, PCGExData::EBufferInit::New);
		}
	}

	if (Config.bWriteAmplitudeRange)
	{
		if (Config.RangeMode == EPCGExVtxAmplitudeMode::Length)
		{
			AmpRangeLengthBuffer = InVtxDataFacade->GetWritable<double>(Config.AmplitudeRangeAttributeName, 0, true, PCGExData::EBufferInit::New);
		}
		else
		{
			AmpRangeBuffer = InVtxDataFacade->GetWritable<FVector>(Config.AmplitudeRangeAttributeName, FVector::ZeroVector, true, PCGExData::EBufferInit::New);
		}
	}

	if (Config.bWriteAmplitudeSign)
	{
		AmpSignBuffer = InVtxDataFacade->GetWritable<double>(Config.AmplitudeSignAttributeName, 0, true, PCGExData::EBufferInit::New);
		bUseSize = Config.SignOutputMode == EPCGExVtxAmplitudeSignOutput::Size;
	}

	return bIsValidOperation;
}

void UPCGExVtxPropertyAmplitude::ProcessNode(PCGExCluster::FNode& Node, const TArray<PCGExCluster::FAdjacencyData>& Adjacency)
{
	const int32 NumAdjacency = Adjacency.Num();

	FVector AverageDirection = FVector::ZeroVector;
	FVector MinAmplitude = FVector(MAX_dbl);
	FVector MaxAmplitude = FVector(MIN_dbl_neg);

	TArray<double> Sizes;
	Sizes.SetNum(NumAdjacency);
	double MaxSize = 0;

	for (int i = 0; i < NumAdjacency; i++)
	{
		const PCGExCluster::FAdjacencyData& A = Adjacency[i];

		const FVector DirAndSize = A.Direction * A.Length;

		MaxSize = FMath::Max(MaxSize, (Sizes[i] = bUseSize ? A.Length : 1));

		AverageDirection += A.Direction;
		MinAmplitude = PCGExBlend::Min(MinAmplitude, DirAndSize);
		MaxAmplitude = PCGExBlend::Max(MaxAmplitude, DirAndSize);
	}

	const FVector AmplitudeRange = MaxAmplitude - MinAmplitude;
	AverageDirection /= static_cast<double>(NumAdjacency);

	if (AmpSignBuffer)
	{
		double Sign = 0;

		if (Config.UpMode == EPCGExVtxAmplitudeUpMode::UpVector)
		{
			FVector UpVector = DirCache ? DirCache->Read(Node.PointIndex) : Config.UpConstant;

			for (int i = 0; i < NumAdjacency; i++) { Sign += FVector::DotProduct(UpVector, Adjacency[i].Direction) * (Sizes[i] / MaxSize); }

			if (Config.SignOutputMode == EPCGExVtxAmplitudeSignOutput::NormalizedSize) { Sign /= NumAdjacency; }
			else { Sign /= NumAdjacency; }
		}
		else
		{
			for (int i = 0; i < NumAdjacency; i++) { Sign += FVector::DotProduct(AverageDirection, Adjacency[i].Direction); }
			Sign /= NumAdjacency;
		}

		if (Config.SignOutputMode != EPCGExVtxAmplitudeSignOutput::Sign)
		{
			AmpSignBuffer->GetMutable(Node.PointIndex) = Config.bAbsoluteSign ? FMath::Abs(Sign) : Sign;
		}
		else
		{
			AmpSignBuffer->GetMutable(Node.PointIndex) = Config.bAbsoluteSign ? FMath::Abs(FMath::Sign(Sign)) : FMath::Sign(Sign);
		}
	}

	if (AmpRangeBuffer) { AmpRangeBuffer->GetMutable(Node.PointIndex) = Config.bAbsoluteRange ? PCGExMath::Abs(AmplitudeRange) : AmplitudeRange; }
	if (AmpRangeLengthBuffer) { AmpRangeLengthBuffer->GetMutable(Node.PointIndex) = AmplitudeRange.Length(); }

	if (MinAmpLengthBuffer) { AmpRangeLengthBuffer->GetMutable(Node.PointIndex) = MinAmplitude.Length(); }
	if (MinAmpBuffer) { MinAmpBuffer->GetMutable(Node.PointIndex) = MinAmplitude; }

	if (MaxAmpLengthBuffer) { MaxAmpLengthBuffer->GetMutable(Node.PointIndex) = MaxAmplitude.Length(); }
	if (MaxAmpBuffer) { MaxAmpBuffer->GetMutable(Node.PointIndex) = MaxAmplitude; }
}

void UPCGExVtxPropertyAmplitude::Cleanup()
{
	Config = FPCGExAmplitudeConfig{};
	DirCache.Reset();

	MinAmpLengthBuffer.Reset();
	MaxAmpLengthBuffer.Reset();
	AmpRangeLengthBuffer.Reset();

	MinAmpBuffer.Reset();
	MaxAmpBuffer.Reset();
	AmpRangeBuffer.Reset();

	AmpSignBuffer.Reset();

	Super::Cleanup();
}

#if WITH_EDITOR
FString UPCGExVtxPropertyAmplitudeSettings::GetDisplayName() const
{
	/*
	if (Config.SourceAttributes.IsEmpty()) { return TEXT(""); }
	TArray<FName> Names = Config.SourceAttributes.Array();

	if (Names.Num() == 1) { return Names[0].ToString(); }
	if (Names.Num() == 2) { return Names[0].ToString() + TEXT(" (+1 other)"); }

	return Names[0].ToString() + FString::Printf(TEXT(" (+%d others)"), (Names.Num() - 1));
	*/
	return TEXT("");
}
#endif

UPCGExVtxPropertyOperation* UPCGExVtxPropertyAmplitudeFactory::CreateOperation(FPCGExContext* InContext) const
{
	UPCGExVtxPropertyAmplitude* NewOperation = InContext->ManagedObjects->New<UPCGExVtxPropertyAmplitude>();
	PCGEX_VTX_EXTRA_CREATE
	return NewOperation;
}

TArray<FPCGPinProperties> UPCGExVtxPropertyAmplitudeSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties = Super::InputPinProperties();
	return PinProperties;
}

UPCGExFactoryData* UPCGExVtxPropertyAmplitudeSettings::CreateFactory(FPCGExContext* InContext, UPCGExFactoryData* InFactory) const
{
	UPCGExVtxPropertyAmplitudeFactory* NewFactory = InContext->ManagedObjects->New<UPCGExVtxPropertyAmplitudeFactory>();
	NewFactory->Config = Config;
	return Super::CreateFactory(InContext, NewFactory);
}


#undef LOCTEXT_NAMESPACE
#undef PCGEX_NAMESPACE
