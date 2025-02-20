﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

// Most helpers here are Copyright Epic Games, Inc. All Rights Reserved, cherry picked for 5.3

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameFramework/Actor.h"

#include "PCGContext.h"
#include "PCGExMacros.h"
#include "PCGManagedResource.h"
#include "Data/PCGSpatialData.h"
#include "Engine/AssetManager.h"
#include "Metadata/PCGMetadataAttributeTraits.h"
#include "Async/Async.h"

#include "PCGExHelpers.generated.h"

UENUM()
enum class EPCGExPointPropertyOutput : uint8
{
	None      = 0 UMETA(DisplayName = "None", Tooltip="..."),
	Density   = 1 UMETA(DisplayName = "Density", Tooltip="..."),
	Steepness = 2 UMETA(DisplayName = "Steepness", Tooltip="..."),
	ColorR    = 3 UMETA(DisplayName = "R Channel", Tooltip="..."),
	ColorG    = 4 UMETA(DisplayName = "G Channel", Tooltip="..."),
	ColorB    = 5 UMETA(DisplayName = "B Channel", Tooltip="..."),
	ColorA    = 6 UMETA(DisplayName = "A Channel", Tooltip="..."),
};

UCLASS(Hidden)
class UPCGExComponentCallback : public UObject
{
	GENERATED_BODY()

	bool bIsOnce = false;
	TFunction<void(UActorComponent* InComponent)> CallbackFn;

public:
	UFUNCTION()
	void Callback(UActorComponent* InComponent);

	virtual void BeginDestroy() override;

	template <typename T>
	void Bind(T& Delegate, TFunction<void(UActorComponent* InComponent)>&& InCallback, const bool bOnce = false)
	{
		check(!CallbackFn)
		bIsOnce = bOnce;
		CallbackFn = InCallback;
		Delegate.AddDynamic(this, &UPCGExComponentCallback::Callback);
	}
};

UCLASS(Hidden)
class UPCGExPCGComponentCallback : public UObject
{
	GENERATED_BODY()

	bool bIsOnce = false;
	TFunction<void(UPCGComponent* InComponent)> CallbackFn;

public:
	UFUNCTION()
	void Callback(UPCGComponent* InComponent);

	virtual void BeginDestroy() override;

	template <typename T>
	void Bind(T& Delegate, TFunction<void(UPCGComponent* InComponent)>&& InCallback, const bool bOnce = false)
	{
		check(!CallbackFn)
		bIsOnce = bOnce;
		CallbackFn = InCallback;
		Delegate.AddDynamic(this, &UPCGExPCGComponentCallback::Callback);
	}
};

UINTERFACE(MinimalAPI)
class UPCGExManagedObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class IPCGExManagedObjectInterface
{
	GENERATED_BODY()

public:
	virtual void Cleanup() = 0;
};

UINTERFACE(MinimalAPI)
class UPCGExManagedComponentInterface : public UInterface
{
	GENERATED_BODY()
};

class IPCGExManagedComponentInterface
{
	GENERATED_BODY()

public:
	virtual void SetManagedComponent(UPCGManagedComponent* InManagedComponent) = 0;
	virtual UPCGManagedComponent* GetManagedComponent() = 0;
};

namespace PCGExHelpers
{
	bool TryGetAttributeName(const FPCGAttributePropertyInputSelector& InSelector, const UPCGData* InData, FName& OutName);

	template <typename T>
	static TObjectPtr<T> LoadBlocking_AnyThread(const TSoftObjectPtr<T>& SoftObjectPtr, const FSoftObjectPath& FallbackPath = nullptr)
	{
		// If the requested object is valid and loaded, early exit
		TObjectPtr<T> LoadedObject = SoftObjectPtr.Get();
		if (LoadedObject) { return LoadedObject; }

		// If not, make sure it's a valid path, and if not fallback to the fallback path
		FSoftObjectPath ToBeLoaded = SoftObjectPtr.ToSoftObjectPath().IsValid() ? SoftObjectPtr.ToSoftObjectPath() : FallbackPath;

		// Make sure we have a valid path at all
		if (!ToBeLoaded.IsValid()) { return nullptr; }

		// Check if the fallback path is loaded, early exit
		LoadedObject = TSoftObjectPtr<T>(ToBeLoaded).Get();
		if (LoadedObject) { return LoadedObject; }


		if (IsInGameThread())
		{
			// We're in the game thread, request synchronous load
			UAssetManager::GetStreamableManager().RequestSyncLoad(ToBeLoaded);
		}
		else
		{
			// We're not in the game thread, we need to dispatch loading to the main thread
			// and wait in the current one
			FEvent* BlockingEvent = FPlatformProcess::GetSynchEventFromPool();
			AsyncTask(
				ENamedThreads::GameThread, [BlockingEvent, ToBeLoaded]()
				{
					const TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
						ToBeLoaded, [BlockingEvent]() { BlockingEvent->Trigger(); });

					if (!Handle || !Handle->IsActive()) { BlockingEvent->Trigger(); }
				});

			BlockingEvent->Wait();
			FPlatformProcess::ReturnSynchEventToPool(BlockingEvent);
		}

		return TSoftObjectPtr<T>(ToBeLoaded).Get();
	}

	void LoadBlocking_AnyThread(const TSharedPtr<TSet<FSoftObjectPath>>& Paths);

	void CopyStructProperties(const void* SourceStruct, void* TargetStruct, const UStruct* SourceStructType, const UStruct* TargetStructType);

	bool CopyProperties(UObject* Target, const UObject* Source, const TSet<FString>* Exclusions = nullptr);

	void SetPointProperty(FPCGPoint& InPoint, const double InValue, const EPCGExPointPropertyOutput InProperty);

	TArray<FString> GetStringArrayFromCommaSeparatedList(const FString& InCommaSeparatedString);

	TArray<UFunction*> FindUserFunctions(const TSubclassOf<AActor>& ActorClass, const TArray<FName>& FunctionNames, const TArray<const UFunction*>& FunctionPrototypes, const FPCGContext* InContext);
}

/** Holds function prototypes used to match against actor function signatures. */
UCLASS(MinimalAPI)
class UPCGExFunctionPrototypes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static UFunction* GetPrototypeWithNoParams() { return FindObject<UFunction>(StaticClass(), TEXT("PrototypeWithNoParams")); }
	static UFunction* GetPrototypeWithPointAndMetadata() { return FindObject<UFunction>(StaticClass(), TEXT("PrototypeWithPointAndMetadata")); }

private:
	UFUNCTION()
	void PrototypeWithNoParams()
	{
	}

	UFUNCTION()
	void PrototypeWithPointAndMetadata(FPCGPoint Point, const UPCGMetadata* Metadata)
	{
	}
};

namespace PCGEx
{
	class FWorkPermit final : public TSharedFromThis<FWorkPermit>
	{
	public:
		FWorkPermit() = default;
		~FWorkPermit() = default;
	};

	class FIntTracker final : public TSharedFromThis<FIntTracker>
	{
		FRWLock Lock;
		bool bTriggered = false;
		int32 PendingCount = 0;
		int32 CompletedCount = 0;

		TFunction<void()> StartFn = nullptr;
		TFunction<void()> ThresholdFn = nullptr;

	public:
		explicit FIntTracker(TFunction<void()>&& InThresholdFn)
		{
			ThresholdFn = InThresholdFn;
		}

		explicit FIntTracker(TFunction<void()>&& InStartFn, TFunction<void()>&& InThresholdFn)
		{
			StartFn = InStartFn;
			ThresholdFn = InThresholdFn;
		}

		~FIntTracker() = default;

		void IncrementPending(const int32 Count = 1);
		void IncrementCompleted(const int32 Count = 1);

		void Trigger();
		void SafetyTrigger();

		void Reset();
		void Reset(const int32 InMax);

	protected:
		void TriggerInternal();
	};

	class FUniqueNameGenerator final : public TSharedFromThis<FUniqueNameGenerator>
	{
		int32 Idx = 0;

	public:
		FUniqueNameGenerator() = default;
		~FUniqueNameGenerator() = default;

		FName Get(const FString& BaseName);
		FName Get(const FName& BaseName);
	};

	struct /*PCGEXTENDEDTOOLKIT_API*/ FManagedObjects
	{
		mutable FRWLock ManagedObjectLock;
		mutable FRWLock DuplicatedObjectLock;

		FPCGContext* Context = nullptr;
		TWeakPtr<FWorkPermit> WorkPermit;
		TSet<UObject*> ManagedObjects;

		bool IsFlushing() const { return bIsFlushing.load(std::memory_order_acquire); }

		explicit FManagedObjects(FPCGContext* InContext, const TSharedPtr<FWorkPermit>& InLifeline);

		~FManagedObjects();

		bool IsAvailable() const;

		void Flush();

		void Add(UObject* InObject);
		bool Remove(UObject* InObject);
		void Remove(const TArray<FPCGTaggedData>& InTaggedData);

		template <class T, typename... Args>
		T* New(Args&&... InArgs)
		{
			check(WorkPermit.IsValid())
			if (IsFlushing()) { UE_LOG(LogTemp, Error, TEXT("Attempting to create a managed object while flushing!")) }

			T* Object = nullptr;
			if (!IsInGameThread())
			{
				{
					FGCScopeGuard Scope;
					Object = NewObject<T>(std::forward<Args>(InArgs)...);
				}
				check(Object);
			}
			else
			{
				Object = NewObject<T>(std::forward<Args>(InArgs)...);
			}

			Add(Object);
			return Object;
		}

		template <class T>
		T* Duplicate(const UPCGData* InData)
		{
			check(WorkPermit.IsValid())
			check(!IsFlushing())

			T* Object = nullptr;

#if PCGEX_ENGINE_VERSION >= 505

			if (!IsInGameThread())
			{
				FWriteScopeLock WriteScopeLock(ManagedObjectLock);

				// Ensure PCG AsyncState is up to date
				bool bRestoreTo = Context->AsyncState.bIsRunningOnMainThread;
				Context->AsyncState.bIsRunningOnMainThread = false;

				// Do the duplicate (uses AnyThread that requires bIsRunningOnMainThread to be up-to-date)
				Object = Cast<T>(InData->DuplicateData(Context, true));

				Context->AsyncState.bIsRunningOnMainThread = bRestoreTo;

				check(Object);
				{
					FWriteScopeLock DupeLock(DuplicatedObjectLock);
					DuplicateObjects.Add(Object);
				}
			}
			else
			{
				FWriteScopeLock WriteScopeLock(ManagedObjectLock);
				Object = Cast<T>(InData->DuplicateData(Context, true));
				check(Object);
				{
					FWriteScopeLock DupeLock(DuplicatedObjectLock);
					DuplicateObjects.Add(Object);
				}
			}


#elif PCGEX_ENGINE_VERSION == 504
			if (!IsInGameThread())
			{
				{
					FGCScopeGuard Scope;
					FWriteScopeLock WriteScopeLock(ManagedObjectLock);
					Object = Cast<T>(InData->DuplicateData(true));
				}
				check(Object);
			}
			else
			{
				FWriteScopeLock WriteScopeLock(ManagedObjectLock);
				Object = Cast<T>(InData->DuplicateData(true));
			}

#else
			
			const UPCGSpatialData* AsSpatialData = Cast<UPCGSpatialData>(InData);
			check(AsSpatialData)
			
			if (!IsInGameThread())
			{
				{
					FGCScopeGuard Scope;
					FWriteScopeLock WriteScopeLock(ManagedObjectLock);
					Object = Cast<T>(AsSpatialData->DuplicateData(true));
				}
				check(Object);
			}
			else
			{
				FWriteScopeLock WriteScopeLock(ManagedObjectLock);
				Object = Cast<T>(AsSpatialData->DuplicateData(true));
			}
			
#endif

			Add(Object);
			return Object;
		}

		void Destroy(UObject* InObject);

	protected:
		TSet<UObject*> DuplicateObjects;
		void RecursivelyClearAsyncFlag_Unsafe(UObject* InObject) const;

	private:
		std::atomic<bool> bIsFlushing{false};
	};

	FVector GetPointsCentroid(const TArray<FPCGPoint>& InPoints);

#pragma region Metadata Type

	constexpr static int32 GetMetadataSize(const EPCGMetadataTypes InType)
	{
		switch (InType)
		{
		case EPCGMetadataTypes::Float:
		case EPCGMetadataTypes::Double:
		case EPCGMetadataTypes::Integer32:
		case EPCGMetadataTypes::Integer64:
			return 1;
		case EPCGMetadataTypes::Vector2:
			return 2;
		case EPCGMetadataTypes::Vector:
		case EPCGMetadataTypes::Rotator:
			return 3;
		case EPCGMetadataTypes::Vector4:
		case EPCGMetadataTypes::Quaternion:
			return 4;
		default:
		case EPCGMetadataTypes::Transform:
		case EPCGMetadataTypes::String:
		case EPCGMetadataTypes::Boolean:
		case EPCGMetadataTypes::Name:
		case EPCGMetadataTypes::Unknown:
			return -1;
		}
	}
	
	template <typename T>
	constexpr static EPCGMetadataTypes GetMetadataType()
	{
		if constexpr (std::is_same_v<T, bool>) { return EPCGMetadataTypes::Boolean; }
		else if constexpr (std::is_same_v<T, int32>) { return EPCGMetadataTypes::Integer32; }
		else if constexpr (std::is_same_v<T, int64>) { return EPCGMetadataTypes::Integer64; }
		else if constexpr (std::is_same_v<T, float>) { return EPCGMetadataTypes::Float; }
		else if constexpr (std::is_same_v<T, double>) { return EPCGMetadataTypes::Double; }
		else if constexpr (std::is_same_v<T, FVector2D>) { return EPCGMetadataTypes::Vector2; }
		else if constexpr (std::is_same_v<T, FVector>) { return EPCGMetadataTypes::Vector; }
		else if constexpr (std::is_same_v<T, FVector4>) { return EPCGMetadataTypes::Vector4; }
		else if constexpr (std::is_same_v<T, FQuat>) { return EPCGMetadataTypes::Quaternion; }
		else if constexpr (std::is_same_v<T, FRotator>) { return EPCGMetadataTypes::Rotator; }
		else if constexpr (std::is_same_v<T, FTransform>) { return EPCGMetadataTypes::Transform; }
		else if constexpr (std::is_same_v<T, FString>) { return EPCGMetadataTypes::String; }
		else if constexpr (std::is_same_v<T, FName>) { return EPCGMetadataTypes::Name; }
#if PCGEX_ENGINE_VERSION > 503
		else if constexpr (std::is_same_v<T, FSoftClassPath>) { return EPCGMetadataTypes::SoftClassPath; }
		else if constexpr (std::is_same_v<T, FSoftObjectPath>) { return EPCGMetadataTypes::SoftObjectPath; }
#endif
		else { return EPCGMetadataTypes::Unknown; }
	}

	constexpr static EPCGMetadataTypes GetPropertyType(const EPCGPointProperties Property)
	{
		switch (Property)
		{
		case EPCGPointProperties::Density:
			return EPCGMetadataTypes::Float;
		case EPCGPointProperties::BoundsMin:
			return EPCGMetadataTypes::Vector;
		case EPCGPointProperties::BoundsMax:
			return EPCGMetadataTypes::Vector;
		case EPCGPointProperties::Extents:
			return EPCGMetadataTypes::Vector;
		case EPCGPointProperties::Color:
			return EPCGMetadataTypes::Vector4;
		case EPCGPointProperties::Position:
			return EPCGMetadataTypes::Vector;
		case EPCGPointProperties::Rotation:
			return EPCGMetadataTypes::Rotator;
		case EPCGPointProperties::Scale:
			return EPCGMetadataTypes::Vector;
		case EPCGPointProperties::Transform:
			return EPCGMetadataTypes::Transform;
		case EPCGPointProperties::Steepness:
			return EPCGMetadataTypes::Float;
		case EPCGPointProperties::LocalCenter:
			return EPCGMetadataTypes::Vector;
		case EPCGPointProperties::Seed:
			return EPCGMetadataTypes::Integer32;
		default:
			return EPCGMetadataTypes::Unknown;
		}
	}

	constexpr bool DummyBoolean = bool{};
	constexpr int32 DummyInteger32 = int32{};
	constexpr int64 DummyInteger64 = int64{};
	constexpr float DummyFloat = float{};
	constexpr double DummyDouble = double{};
	const FVector2D DummyVector2 = FVector2D::ZeroVector;
	const FVector DummyVector = FVector::ZeroVector;
	const FVector4 DummyVector4 = FVector4::Zero();
	const FQuat DummyQuaternion = FQuat::Identity;
	const FRotator DummyRotator = FRotator::ZeroRotator;
	const FTransform DummyTransform = FTransform::Identity;
	const FString DummyString = TEXT("");
	const FName DummyName = NAME_None;
	const FSoftClassPath DummySoftClassPath = FSoftClassPath{};
	const FSoftObjectPath DummySoftObjectPath = FSoftObjectPath{};

	template <typename T, typename Func>
	static void ExecuteWithRightType(Func&& Callback)
	{
		if constexpr (std::is_same_v<T, bool>) { Callback(DummyBoolean); }
		else if constexpr (std::is_same_v<T, int32>) { Callback(DummyInteger32); }
		else if constexpr (std::is_same_v<T, int64>) { Callback(DummyInteger64); }
		else if constexpr (std::is_same_v<T, float>) { Callback(DummyFloat); }
		else if constexpr (std::is_same_v<T, double>) { Callback(DummyDouble); }
		else if constexpr (std::is_same_v<T, FVector2D>) { Callback(DummyVector2); }
		else if constexpr (std::is_same_v<T, FVector>) { Callback(DummyVector); }
		else if constexpr (std::is_same_v<T, FVector4>) { Callback(DummyVector4); }
		else if constexpr (std::is_same_v<T, FQuat>) { Callback(DummyQuaternion); }
		else if constexpr (std::is_same_v<T, FRotator>) { Callback(DummyRotator); }
		else if constexpr (std::is_same_v<T, FTransform>) { Callback(DummyTransform); }
		else if constexpr (std::is_same_v<T, FString>) { Callback(DummyString); }
		else if constexpr (std::is_same_v<T, FName>) { Callback(DummyName); }
#if PCGEX_ENGINE_VERSION > 503
		else if constexpr (std::is_same_v<T, FSoftClassPath>) { Callback(DummySoftClassPath); }
		else if constexpr (std::is_same_v<T, FSoftObjectPath>) { Callback(DummySoftObjectPath); }
#endif
		else { static_assert("Unsupported type"); }
	}

	template <typename Func>
	static void ExecuteWithRightType(const EPCGMetadataTypes Type, Func&& Callback)
	{
#define PCGEX_EXECUTE_WITH_TYPE(_TYPE, _ID, ...) case EPCGMetadataTypes::_ID : ExecuteWithRightType<_TYPE>(Callback); break;

		switch (Type)
		{
		PCGEX_FOREACH_SUPPORTEDTYPES(PCGEX_EXECUTE_WITH_TYPE)
		default: ;
		}

#undef PCGEX_EXECUTE_WITH_TYPE
	}

	template <typename Func>
	static void ExecuteWithRightType(const int16 Type, Func&& Callback)
	{
		ExecuteWithRightType(static_cast<EPCGMetadataTypes>(Type), Callback);
	}

#pragma endregion

#pragma region Array

	template <typename T>
	static void InitArray(TArray<T>& InArray, const int32 Num)
	{
		if constexpr (std::is_trivially_copyable_v<T>) { InArray.SetNumUninitialized(Num); }
		else { InArray.SetNum(Num); }
	}

	template <typename T>
	static void InitArray(TSharedPtr<TArray<T>>& InArray, const int32 Num)
	{
		if (!InArray) { InArray = MakeShared<TArray<T>>(); }
		if constexpr (std::is_trivially_copyable_v<T>) { InArray->SetNumUninitialized(Num); }
		else { InArray->SetNum(Num); }
	}

	template <typename T>
	static void InitArray(TSharedRef<TArray<T>> InArray, const int32 Num)
	{
		if constexpr (std::is_trivially_copyable_v<T>) { InArray.SetNumUninitialized(Num); }
		else { InArray.SetNum(Num); }
	}

	template <typename T>
	static void InitArray(TArray<T>* InArray, const int32 Num)
	{
		if constexpr (std::is_trivially_copyable_v<T>) { InArray->SetNumUninitialized(Num); }
		else { InArray->SetNum(Num); }
	}

	template <typename T>
	void ReorderArray(TArray<T>& InArray, const TArray<int32>& InOrder)
	{
		check(InArray.Num() == InOrder.Num())

		const int32 NumElements = InArray.Num();
		TBitArray<> Visited;
		Visited.Init(false, NumElements);

		for (int32 i = 0; i < NumElements; ++i)
		{
			if (Visited[i])
			{
				continue;
			}

			int32 Current = i;
			T Temp = MoveTemp(InArray[i]);

			while (!Visited[Current])
			{
				Visited[Current] = true;
				int32 Next = InOrder[Current];

				if (Next == i)
				{
					InArray[Current] = MoveTemp(Temp);
					break;
				}

				InArray[Current] = MoveTemp(InArray[Next]);
				Current = Next;
			}
		}
	}

	template <typename D>
	struct TOrder
	{
		int32 Index = -1;
		D Det;

		TOrder(const int32 InIndex, const D& InDet)
			: Index(InIndex), Det(InDet)
		{
		}
	};

	template <typename T>
	static void ShiftArrayToSmallest(TArray<T>& InArray)
	{
		const int32 Num = InArray.Num();
		if (Num <= 1) { return; }

		int32 MinIndex = 0;
		for (int32 i = 1; i < Num; ++i) { if (InArray[i] < InArray[MinIndex]) { MinIndex = i; } }

		if (MinIndex > 0)
		{
			TArray<T> TempArray;
			TempArray.Append(InArray.GetData() + MinIndex, Num - MinIndex);
			TempArray.Append(InArray.GetData(), MinIndex);

			FMemory::Memcpy(InArray.GetData(), TempArray.GetData(), sizeof(T) * Num);
		}
	}

	template <typename T, typename FPredicate>
	static void ShiftArrayToPredicate(TArray<T>& InArray, FPredicate&& Predicate)
	{
		const int32 Num = InArray.Num();
		if (Num <= 1) { return; }

		int32 MinIndex = 0;
		for (int32 i = 1; i < Num; ++i) { if (Predicate(InArray[i], InArray[MinIndex])) { MinIndex = i; } }

		if (MinIndex > 0)
		{
			TArray<T> TempArray;
			TempArray.Append(InArray.GetData() + MinIndex, Num - MinIndex);
			TempArray.Append(InArray.GetData(), MinIndex);

			FMemory::Memcpy(InArray.GetData(), TempArray.GetData(), sizeof(T) * Num);
		}
	}

	template <typename T, typename D>
	void ReorderArray(TArray<T>& InArray, const TArray<TOrder<D>>& InOrder)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(PCGExHelpers::ReorderArray);

		check(InArray.Num() == InOrder.Num());

		const int32 NumElements = InArray.Num();
		TBitArray<> Visited;
		Visited.Init(false, NumElements);

		for (int32 i = 0; i < NumElements; ++i)
		{
			if (Visited[i])
			{
				continue; // Skip already visited elements in a cycle.
			}

			int32 Current = i;
			T Temp = MoveTemp(InArray[i]); // Temporarily hold the current element.

			// Follow the cycle defined by the indices in InOrder.
			while (!Visited[Current])
			{
				Visited[Current] = true;

				int32 Next = InOrder[Current].Index;
				if (Next == i)
				{
					InArray[Current] = MoveTemp(Temp);
					break;
				}

				InArray[Current] = MoveTemp(InArray[Next]);
				Current = Next;
			}
		}
	}

#pragma endregion
}
