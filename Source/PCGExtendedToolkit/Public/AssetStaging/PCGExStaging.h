﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "Collections/PCGExAssetCollection.h"

namespace PCGExStaging
{
	const FName SourceCollectionMapLabel = TEXT("Map");
	const FName OutputCollectionMapLabel = TEXT("Map");

	const FName Tag_CollectionPath = FName(PCGEx::PCGExPrefix + TEXT("Collection/Path"));
	const FName Tag_CollectionIdx = FName(PCGEx::PCGExPrefix + TEXT("Collection/Idx"));
	const FName Tag_EntryIdx = FName(PCGEx::PCGExPrefix + TEXT("CollectionEntry"));

	class PCGEXTENDEDTOOLKIT_API FPickPacker : public TSharedFromThis<FPickPacker>
	{
		FPCGExContext* Context = nullptr;

		TArray<const UPCGExAssetCollection*> AssetCollections;
		TMap<const UPCGExAssetCollection*, uint32> CollectionMap;
		mutable FRWLock AssetCollectionsLock;

		uint16 BaseHash = 0;

	public:
		FPickPacker(FPCGExContext* InContext)
			: Context(InContext)
		{
			BaseHash = static_cast<uint16>(InContext->GetInputSettings<UPCGSettings>()->UID);
		}

		uint64 GetPickIdx(const UPCGExAssetCollection* InCollection, const int32 InIndex)
		{
			// TODO : Pack index pick + material pick here

			{
				FReadScopeLock ReadScopeLock(AssetCollectionsLock);
				if (const uint32* ColIdxPtr = CollectionMap.Find(InCollection)) { return PCGEx::H64(*ColIdxPtr, InIndex); }
			}

			{
				FWriteScopeLock WriteScopeLock(AssetCollectionsLock);
				if (const uint32* ColIdxPtr = CollectionMap.Find(InCollection)) { return PCGEx::H64(*ColIdxPtr, InIndex); }

				uint32 ColIndex = PCGEx::H32(BaseHash, AssetCollections.Add(InCollection));
				CollectionMap.Add(InCollection, ColIndex);
				return PCGEx::H64(ColIndex, InIndex);
			}
		}

		void PackToDataset(const UPCGParamData* InAttributeSet)
		{
			FPCGMetadataAttribute<int32>* CollectionIdx = InAttributeSet->Metadata->FindOrCreateAttribute<int32>(Tag_CollectionIdx, 0, false, true, true);

#if PCGEX_ENGINE_VERSION > 503
			FPCGMetadataAttribute<FSoftObjectPath>* CollectionPath = InAttributeSet->Metadata->FindOrCreateAttribute<FSoftObjectPath>(Tag_CollectionPath, FSoftObjectPath(), false, true, true);
#else
			FPCGMetadataAttribute<FString>* CollectionPath = InAttributeSet->Metadata->FindOrCreateAttribute<FString>(Tag_CollectionPath, TEXT(""), false, true, true);
#endif

			for (const TPair<const UPCGExAssetCollection*, uint32>& Pair : CollectionMap)
			{
				const int64 Key = InAttributeSet->Metadata->AddEntry();
				CollectionIdx->SetValue(Key, Pair.Value);

#if PCGEX_ENGINE_VERSION > 503
				CollectionPath->SetValue(Key, FSoftObjectPath(Pair.Key));
#else
				CollectionPath->SetValue(Key, FSoftObjectPath(Pair.Key).ToString());
#endif
			}
		}
	};

	template <typename C = UPCGExAssetCollection, typename A = FPCGExAssetCollectionEntry>
	class PCGEXTENDEDTOOLKIT_API TPickUnpacker : public TSharedFromThis<TPickUnpacker<C, A>>
	{
		TMap<uint32, C*> CollectionMap;

	public:
		TMap<int64, TSharedPtr<TArray<int32>>> HashedPartitions;

		TPickUnpacker()
		{
		}

		bool UnpackDataset(FPCGContext* InContext, const UPCGParamData* InAttributeSet)
		{
			const UPCGMetadata* Metadata = InAttributeSet->Metadata;

#if PCGEX_ENGINE_VERSION > 503
			TUniquePtr<FPCGAttributeAccessorKeysEntries> Keys = MakeUnique<FPCGAttributeAccessorKeysEntries>(Metadata);
#else
			const TSharedPtr<PCGEx::FAttributesInfos> Infos = PCGEx::FAttributesInfos::Get(Metadata);
			if (Infos->Attributes.IsEmpty())
			{
				PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Missing required attributes."));
				return false;
			}
			TUniquePtr<FPCGAttributeAccessorKeysEntries> Keys = MakeUnique<FPCGAttributeAccessorKeysEntries>(Infos->Attributes[0]); // Probably not reliable, but make 5.3 compile -_-
#endif

			const int32 NumEntries = Keys->GetNum();
			if (NumEntries == 0)
			{
				PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Attribute set is empty."));
				return false;
			}

			CollectionMap.Reserve(CollectionMap.Num() + NumEntries);

			const FPCGMetadataAttribute<int32>* CollectionIdx = InAttributeSet->Metadata->GetConstTypedAttribute<int32>(Tag_CollectionIdx);

#if PCGEX_ENGINE_VERSION > 503
			const FPCGMetadataAttribute<FSoftObjectPath>* CollectionPath = InAttributeSet->Metadata->GetConstTypedAttribute<FSoftObjectPath>(Tag_CollectionPath);
#else
			const FPCGMetadataAttribute<FString>* CollectionPath = InAttributeSet->Metadata->GetConstTypedAttribute<FString>(Tag_CollectionPath);
#endif

			if (!CollectionIdx || !CollectionPath)
			{
				PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Missing required attributes, or unsupported type."));
				return false;
			}

			for (int i = 0; i < NumEntries; i++)
			{
				int32 Idx = CollectionIdx->GetValueFromItemKey(i);

#if PCGEX_ENGINE_VERSION > 503
				C* Collection = PCGExHelpers::LoadBlocking_AnyThread<C>(TSoftObjectPtr<C>(CollectionPath->GetValueFromItemKey(i)));
#else
				C* Collection = PCGExHelpers::LoadBlocking_AnyThread<C>(nullptr, FSoftObjectPath(CollectionPath->GetValueFromItemKey(i)));
#endif

				if (!Collection)
				{
					PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Some collections could not be loaded."));
					return false;
				}

				if (CollectionMap.Contains(Idx))
				{
					if (CollectionMap[Idx] == Collection) { continue; }

					PCGE_LOG_C(Error, GraphAndLog, InContext, FTEXT("Collection Idx collision."));
					return false;
				}

				CollectionMap.Add(Idx, Collection);
			}

			return true;
		}

		void UnpackPin(FPCGContext* InContext, const FName InPinLabel)
		{
			for (TArray<FPCGTaggedData> Params = InContext->InputData.GetParamsByPin(InPinLabel);
			     const FPCGTaggedData& InTaggedData : Params)
			{
				const UPCGParamData* ParamData = Cast<UPCGParamData>(InTaggedData.Data);

				if (!ParamData) { continue; }
				const TSharedPtr<PCGEx::FAttributesInfos> Infos = PCGEx::FAttributesInfos::Get(ParamData->Metadata);

				if (!ParamData->Metadata->HasAttribute(Tag_CollectionIdx) || !ParamData->Metadata->HasAttribute(Tag_CollectionPath)) { continue; }

				UnpackDataset(InContext, ParamData);
			}
		}

		bool HasValidMapping() const { return !CollectionMap.IsEmpty(); }

		bool ResolveEntry(uint64 EntryHash, const A*& OutEntry)
		{
			const UPCGExAssetCollection* EntryHost = nullptr;

			uint32 CollectionIdx = 0;
			uint32 EntryIndex = 0;
			PCGEx::H64(EntryHash, CollectionIdx, EntryIndex);

			C** Collection = CollectionMap.Find(CollectionIdx);
			if (!Collection || !(*Collection)->IsValidIndex(EntryIndex)) { return false; }

			return (*Collection)->GetEntryAt(OutEntry, EntryIndex, EntryHost);
		}

		bool BuildPartitions(const UPCGPointData* InPointData)
		{
			const FPCGMetadataAttribute<int64>* HashAttribute = InPointData->Metadata->GetConstTypedAttribute<int64>(Tag_EntryIdx);

			if (!HashAttribute) { return false; }

			const TArray<FPCGPoint>& InPoints = InPointData->GetPoints();
			const TSharedPtr<FPCGAttributeAccessorKeysPoints> InKeys = MakeShared<FPCGAttributeAccessorKeysPoints>(InPoints);
			const TUniquePtr<FPCGAttributeAccessor<int64>> InAccessor = MakeUnique<FPCGAttributeAccessor<int64>>(HashAttribute, InPointData->Metadata);

			const int32 NumPoints = InPoints.Num();

			TArray<int64> Hashes;
			Hashes.SetNumUninitialized(NumPoints);

			if (const TArrayView<int64> InRange = MakeArrayView(Hashes.GetData(), NumPoints);
				!InAccessor->GetRange(InRange, 0, *InKeys)) { return false; }

			// Build partitions
			for (int i = 0; i < NumPoints; i++)
			{
				uint64 EntryHash = Hashes[i];
				TSharedPtr<TArray<int32>>* Indices = HashedPartitions.Find(EntryHash);

				if (!Indices)
				{
					PCGEX_MAKE_SHARED(NewIndices, TArray<int32>)
					NewIndices->Add(i);
					HashedPartitions.Add(EntryHash, NewIndices);
				}
				else
				{
					(*Indices)->Add(i);
				}
			}

			return !HashedPartitions.IsEmpty();
		}
	};
}
