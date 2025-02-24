﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/


#pragma once

#include "CoreMinimal.h"

#include "PCGExDetails.h"
#include "Data/PCGExData.h"

#include "PCGExCompare.generated.h"

#define PCGEX_UNSUPPORTED_STRING_TYPES(MACRO)\
MACRO(FString)\
MACRO(FName)\
MACRO(FSoftObjectPath)\
MACRO(FSoftClassPath)

#define PCGEX_UNSUPPORTED_PATH_TYPES(MACRO)\
MACRO(FSoftObjectPath)\
MACRO(FSoftClassPath)

UENUM()
enum class EPCGExIndexMode : uint8
{
	Pick   = 0 UMETA(DisplayName = "Pick", ToolTip="Index value represent a specific pick"),
	Offset = 1 UMETA(DisplayName = "Offset", ToolTip="Index value represent an offset from current point' index"),
};

UENUM()
enum class EPCGExAngularDomain : uint8
{
	Amplitude = 0 UMETA(DisplayName = "Amplitude", Tooltip="Read the value as the result of a normalized dot product"),
	Degrees   = 1 UMETA(DisplayName = "Degrees", Tooltip="Read the value as degrees"),
};

UENUM(BlueprintType)
enum class EPCGExComparison : uint8
{
	StrictlyEqual    = 0 UMETA(DisplayName = " == ", Tooltip="Operand A Strictly Equal to Operand B"),
	StrictlyNotEqual = 1 UMETA(DisplayName = " != ", Tooltip="Operand A Strictly Not Equal to Operand B"),
	EqualOrGreater   = 2 UMETA(DisplayName = " >= ", Tooltip="Operand A Equal or Greater to Operand B"),
	EqualOrSmaller   = 3 UMETA(DisplayName = " <= ", Tooltip="Operand A Equal or Smaller to Operand B"),
	StrictlyGreater  = 4 UMETA(DisplayName = " > ", Tooltip="Operand A Strictly Greater to Operand B"),
	StrictlySmaller  = 5 UMETA(DisplayName = " < ", Tooltip="Operand A Strictly Smaller to Operand B"),
	NearlyEqual      = 6 UMETA(DisplayName = " ~= ", Tooltip="Operand A Nearly Equal to Operand B"),
	NearlyNotEqual   = 7 UMETA(DisplayName = " !~= ", Tooltip="Operand A Nearly Not Equal to Operand B"),
};

UENUM()
enum class EPCGExEquality : uint8
{
	Equal    = 0 UMETA(DisplayName = " == ", Tooltip="Operand A Equal to Operand B"),
	NotEqual = 1 UMETA(DisplayName = " != ", Tooltip="Operand A Not Equal to Operand B"),
};

UENUM(BlueprintType)
enum class EPCGExStringComparison : uint8
{
	StrictlyEqual         = 0 UMETA(DisplayName = " == ", Tooltip="Operand A Strictly Equal to Operand B"),
	StrictlyNotEqual      = 1 UMETA(DisplayName = " != ", Tooltip="Operand A Strictly Not Equal to Operand B"),
	LengthStrictlyEqual   = 2 UMETA(DisplayName = " == (Length) ", Tooltip="Operand A Strictly Equal to Operand B"),
	LengthStrictlyUnequal = 3 UMETA(DisplayName = " != (Length) ", Tooltip="Operand A Strictly Not Equal to Operand B"),
	LengthEqualOrGreater  = 4 UMETA(DisplayName = " >= (Length)", Tooltip="Operand A Equal or Greater to Operand B"),
	LengthEqualOrSmaller  = 5 UMETA(DisplayName = " <= (Length)", Tooltip="Operand A Equal or Smaller to Operand B"),
	StrictlyGreater       = 6 UMETA(DisplayName = " > (Length)", Tooltip="Operand A Strictly Greater to Operand B"),
	StrictlySmaller       = 7 UMETA(DisplayName = " < (Length)", Tooltip="Operand A Strictly Smaller to Operand B"),
	LocaleStrictlyGreater = 8 UMETA(DisplayName = " > (Locale)", Tooltip="Operand A Locale Strictly Greater to Operand B Locale"),
	LocaleStrictlySmaller = 9 UMETA(DisplayName = " < (Locale)", Tooltip="Operand A Locale Strictly Smaller to Operand B Locale"),
	Contains              = 10 UMETA(DisplayName = " Contains ", Tooltip="Operand A contains Operand B"),
	StartsWith            = 11 UMETA(DisplayName = " Starts With ", Tooltip="Operand A starts with Operand B"),
	EndsWith              = 12 UMETA(DisplayName = " Ends With ", Tooltip="Operand A ends with Operand B"),
};

UENUM()
enum class EPCGExBitflagComparison : uint8
{
	MatchPartial   = 0 UMETA(DisplayName = "Match (any)", Tooltip="Value & Mask != 0 (At least some flags in the mask are set)"),
	MatchFull      = 1 UMETA(DisplayName = "Match (all)", Tooltip="Value & Mask == Mask (All the flags in the mask are set)"),
	MatchStrict    = 2 UMETA(DisplayName = "Match (strict)", Tooltip="Value == Mask (Flags strictly equals mask)"),
	NoMatchPartial = 3 UMETA(DisplayName = "No match (any)", Tooltip="Value & Mask == 0 (Flags does not contains any from mask)"),
	NoMatchFull    = 4 UMETA(DisplayName = "No match (all)", Tooltip="Value & Mask != Mask (Flags does not contains the mask)"),
};

UENUM()
enum class EPCGExComparisonDataType : uint8
{
	Numeric = 0 UMETA(DisplayName = "Numeric", Tooltip="Compare numeric values"),
	String  = 1 UMETA(DisplayName = "String", Tooltip="Compare string values"),
};

namespace PCGExCompare
{
	FString ToString(const EPCGExComparison Comparison);
	FString ToString(const EPCGExBitflagComparison Comparison);
	FString ToString(const EPCGExStringComparison Comparison);
	FString ToString(const EPCGExStringMatchMode MatchMode);

	bool Compare(const EPCGExStringComparison Method, const FString& A, const FString& B);

#pragma region Numeric comparisons ops

	template <typename T>
	FORCEINLINE static bool StrictlyEqual(const T& A, const T& B) { return A == B; }

	template <typename T>
	FORCEINLINE static bool StrictlyNotEqual(const T& A, const T& B) { return A != B; }

	template <typename T>
	FORCEINLINE static bool EqualOrGreater(const T& A, const T& B)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return A == B || A;
		}
		else if constexpr (
			std::is_same_v<T, FVector2D> ||
			std::is_same_v<T, FVector>)
		{
			return A.SquaredLength() >= B.SquaredLength();
		}
		else if constexpr (std::is_same_v<T, FVector4>)
		{
			return EqualOrGreater(FVector(A), FVector(B));
		}
		else if constexpr (
			std::is_same_v<T, FQuat> ||
			std::is_same_v<T, FRotator>)
		{
			return EqualOrGreater(FVector(A.Euler()), FVector(B.Euler()));
		}
		else if constexpr (std::is_same_v<T, FTransform>)
		{
			return (
				EqualOrGreater(A.GetLocation(), B.GetLocation()) &&
				EqualOrGreater(A.GetRotation(), B.GetRotation()) &&
				EqualOrGreater(A.GetScale3D(), B.GetScale3D()));
		}
		else if constexpr (std::is_same_v<T, FString>)
		{
			return A.Len() >= B.Len();
		}
		else if constexpr (
			std::is_same_v<T, FName> ||
			std::is_same_v<T, FSoftObjectPath> ||
			std::is_same_v<T, FSoftClassPath>)
		{
			return EqualOrGreater(A.ToString(), B.ToString());
		}
		else
		{
			return A >= B;
		}
	}

	template <typename T>
	FORCEINLINE static bool EqualOrSmaller(const T& A, const T& B)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return A == B || !A;
		}
		else if constexpr (
			std::is_same_v<T, FVector2D> ||
			std::is_same_v<T, FVector>)
		{
			return A.SquaredLength() <= B.SquaredLength();
		}
		else if constexpr (std::is_same_v<T, FVector4>)
		{
			return EqualOrSmaller(FVector(A), FVector(B));
		}
		else if constexpr (
			std::is_same_v<T, FQuat> ||
			std::is_same_v<T, FRotator>)
		{
			return EqualOrSmaller(FVector(A.Euler()), FVector(B.Euler()));
		}
		else if constexpr (std::is_same_v<T, FTransform>)
		{
			return (
				EqualOrSmaller(A.GetLocation(), B.GetLocation()) &&
				EqualOrSmaller(A.GetRotation(), B.GetRotation()) &&
				EqualOrSmaller(A.GetScale3D(), B.GetScale3D()));
		}
		else if constexpr (std::is_same_v<T, FString>)
		{
			return A.Len() <= B.Len();
		}
		else if constexpr (
			std::is_same_v<T, FName> ||
			std::is_same_v<T, FSoftObjectPath> ||
			std::is_same_v<T, FSoftClassPath>)
		{
			return EqualOrSmaller(A.ToString(), B.ToString());
		}
		else
		{
			return A <= B;
		}
	}

	template <typename T>
	FORCEINLINE static bool StrictlyGreater(const T& A, const T& B)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return A && !B;
		}
		else if constexpr (
			std::is_same_v<T, FVector2D> ||
			std::is_same_v<T, FVector>)
		{
			return A.SquaredLength() > B.SquaredLength();
		}
		else if constexpr (std::is_same_v<T, FVector4>)
		{
			return StrictlyGreater(FVector(A), FVector(B));
		}
		else if constexpr (
			std::is_same_v<T, FQuat> ||
			std::is_same_v<T, FRotator>)
		{
			return StrictlyGreater(FVector(A.Euler()), FVector(B.Euler()));
		}
		else if constexpr (std::is_same_v<T, FTransform>)
		{
			return (
				StrictlyGreater(A.GetLocation(), B.GetLocation()) &&
				StrictlyGreater(A.GetRotation(), B.GetRotation()) &&
				StrictlyGreater(A.GetScale3D(), B.GetScale3D()));
		}
		else if constexpr (std::is_same_v<T, FString>)
		{
			return A.Len() > B.Len();
		}
		else if constexpr (
			std::is_same_v<T, FName> ||
			std::is_same_v<T, FSoftObjectPath> ||
			std::is_same_v<T, FSoftClassPath>)
		{
			return StrictlyGreater(A.ToString(), B.ToString());
		}
		else
		{
			return A > B;
		}
	}

	template <typename T>
	FORCEINLINE static bool StrictlySmaller(const T& A, const T& B)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return !A && B;
		}
		else if constexpr (
			std::is_same_v<T, FVector2D> ||
			std::is_same_v<T, FVector>)
		{
			return A.SquaredLength() < B.SquaredLength();
		}
		else if constexpr (std::is_same_v<T, FVector4>)
		{
			return StrictlySmaller(FVector(A), FVector(B));
		}
		else if constexpr (
			std::is_same_v<T, FQuat> ||
			std::is_same_v<T, FRotator>)
		{
			return StrictlySmaller(FVector(A.Euler()), FVector(B.Euler()));
		}
		else if constexpr (std::is_same_v<T, FTransform>)
		{
			return (
				StrictlySmaller(A.GetLocation(), B.GetLocation()) &&
				StrictlySmaller(A.GetRotation(), B.GetRotation()) &&
				StrictlySmaller(A.GetScale3D(), B.GetScale3D()));
		}
		else if constexpr (std::is_same_v<T, FString>)
		{
			return A.Len() < B.Len();
		}
		else if constexpr (
			std::is_same_v<T, FName> ||
			std::is_same_v<T, FSoftObjectPath> ||
			std::is_same_v<T, FSoftClassPath>)
		{
			return StrictlySmaller(A.ToString(), B.ToString());
		}
		else
		{
			return A < B;
		}
	}

	template <typename T>
	FORCEINLINE static bool NearlyEqual(const T& A, const T& B, const double Tolerance = DBL_COMPARE_TOLERANCE)
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			return A == B;
		}
		else if constexpr (std::is_same_v<T, FVector2D>)
		{
			return
				NearlyEqual(A.X, B.X, Tolerance) &&
				NearlyEqual(A.Y, B.Y, Tolerance);
		}
		else if constexpr (std::is_same_v<T, FVector2D>)
		{
			return
				NearlyEqual(A.X, B.X, Tolerance) &&
				NearlyEqual(A.Y, B.Y, Tolerance) &&
				NearlyEqual(A.Z, B.Z, Tolerance);
		}
		else if constexpr (std::is_same_v<T, FVector4>)
		{
			return
				NearlyEqual(A.X, B.X, Tolerance) &&
				NearlyEqual(A.Y, B.Y, Tolerance) &&
				NearlyEqual(A.Z, B.Z, Tolerance) &&
				NearlyEqual(A.W, B.W, Tolerance);
		}
		else if constexpr (
			std::is_same_v<T, FQuat> ||
			std::is_same_v<T, FRotator>)
		{
			return NearlyEqual(FVector(A.Euler()), FVector(B.Euler()), Tolerance);
		}
		else if constexpr (std::is_same_v<T, FTransform>)
		{
			return
				NearlyEqual(A.GetLocation(), B.GetLocation(), Tolerance) &&
				NearlyEqual(A.GetRotation(), B.GetRotation(), Tolerance) &&
				NearlyEqual(A.GetScale3D(), B.GetScale3D(), Tolerance);
		}
		else if constexpr (std::is_same_v<T, FString>)
		{
			return FMath::IsNearlyEqual(A.Len(), B.Len(), Tolerance);
		}
		else if constexpr (
			std::is_same_v<T, FName> ||
			std::is_same_v<T, FSoftObjectPath> ||
			std::is_same_v<T, FSoftClassPath>)
		{
			return NearlyEqual(A.ToString(), B.ToString(), Tolerance);
		}
		else
		{
			return FMath::IsNearlyEqual(A, B, Tolerance);
		}
	}

	template <typename T>
	FORCEINLINE static bool NearlyNotEqual(const T& A, const T& B, const double Tolerance = DBL_COMPARE_TOLERANCE) { return !NearlyEqual(A, B, Tolerance); }

#pragma endregion

	template <typename T>
	FORCEINLINE static bool Compare(const EPCGExComparison Method, const T& A, const T& B, const double Tolerance = DBL_COMPARE_TOLERANCE)
	{
		switch (Method)
		{
		case EPCGExComparison::StrictlyEqual:
			return StrictlyEqual(A, B);
		case EPCGExComparison::StrictlyNotEqual:
			return StrictlyNotEqual(A, B);
		case EPCGExComparison::EqualOrGreater:
			return EqualOrGreater(A, B);
		case EPCGExComparison::EqualOrSmaller:
			return EqualOrSmaller(A, B);
		case EPCGExComparison::StrictlyGreater:
			return StrictlyGreater(A, B);
		case EPCGExComparison::StrictlySmaller:
			return StrictlySmaller(A, B);
		case EPCGExComparison::NearlyEqual:
			return NearlyEqual(A, B, Tolerance);
		case EPCGExComparison::NearlyNotEqual:
			return NearlyNotEqual(A, B, Tolerance);
		default:
			return false;
		}
	}

	bool Compare(const EPCGExComparison Method, const TSharedPtr<PCGExData::FTagValue>& A, const double B, const double Tolerance = DBL_COMPARE_TOLERANCE);
	bool Compare(const EPCGExStringComparison Method, const TSharedPtr<PCGExData::FTagValue>& A, const FString B);
	bool Compare(const EPCGExBitflagComparison Method, const int64& Flags, const int64& Mask);

	bool HasMatchingTags(const TSharedPtr<PCGExData::FTags>& InTags, const FString& Query, const EPCGExStringMatchMode MatchMode);
	bool GetMatchingValueTags(const TSharedPtr<PCGExData::FTags>& InTags, const FString& Query, const EPCGExStringMatchMode MatchMode, TArray<TSharedPtr<PCGExData::FTagValue>>& OutValues);
}

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExComparisonDetails
{
	GENERATED_BODY()

	FPCGExComparisonDetails()
	{
	}

	FPCGExComparisonDetails(const FPCGExComparisonDetails& Other):
		Comparison(Other.Comparison),
		Tolerance(Other.Tolerance)
	{
	}

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FPCGAttributePropertyInputSelector OperandA;

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	FPCGAttributePropertyInputSelector OperandB;

	/** Comparison method. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	EPCGExComparison Comparison = EPCGExComparison::StrictlyEqual;

	/** Comparison Tolerance. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="Comparison==EPCGExComparison::NearlyEqual||Comparison==EPCGExComparison::NearlyNotEqual", EditConditionHides, ClampMin=0.001))
	double Tolerance = DBL_COMPARE_TOLERANCE;
};

UENUM()
enum class EPCGExDirectionCheckMode : uint8
{
	Dot  = 0 UMETA(DisplayName = "Dot (Precise)", Tooltip="Extensive comparison using Dot product"),
	Hash = 1 UMETA(DisplayName = "Hash (Fast)", Tooltip="Simplified check using hash comparison with a destructive tolerance"),
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExVectorHashComparisonDetails
{
	GENERATED_BODY()

	FPCGExVectorHashComparisonDetails()
	{
	}

	/** Type of Tolerance value source */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExInputValueType HashToleranceInput = EPCGExInputValueType::Constant;

	/** Tolerance value use for comparison */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Hash Tolerance (Attr)", EditCondition="HashToleranceInput!=EPCGExInputValueType::Constant", EditConditionHides))
	FPCGAttributePropertyInputSelector HashToleranceAttribute;

	/** Tolerance value use for comparison */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Hash Tolerance", EditCondition="HashToleranceInput==EPCGExInputValueType::Constant", EditConditionHides, ClampMin=0.00001))
	double HashToleranceConstant = 0.001;

	FVector CWTolerance = FVector::ZeroVector;

	bool bUseLocalTolerance = false;
	TSharedPtr<PCGExData::TBuffer<double>> LocalOperand;

	bool Init(const FPCGContext* InContext, const TSharedRef<PCGExData::FFacade>& InPrimaryDataFacade);
	FVector GetCWTolerance(const int32 PointIndex) const;
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExDotComparisonDetails
{
	GENERATED_BODY()

	FPCGExDotComparisonDetails()
	{
	}

	/** Comparison */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable))
	EPCGExComparison Comparison = EPCGExComparison::EqualOrGreater;

	/** Value domain. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable))
	EPCGExAngularDomain Domain = EPCGExAngularDomain::Amplitude;

	/** If enabled, the dot product will be made absolute before testing. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable))
	bool bUnsignedComparison = false;

	/** Type of Dot value source */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, DisplayName="Threshold Input"))
	EPCGExInputValueType ThresholdInput = EPCGExInputValueType::Constant;

	/** Dot value use for comparison */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Threshold (Amplitude or Degrees)", EditCondition="ThresholdInput!=EPCGExInputValueType::Constant", EditConditionHides))
	FPCGAttributePropertyInputSelector ThresholdAttribute;

	/** Dot value use for comparison (In raw -1/1 range) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Amplitude", EditCondition="ThresholdInput==EPCGExInputValueType::Constant && Domain==EPCGExAngularDomain::Amplitude", EditConditionHides, ClampMin=-1, ClampMax=1))
	double DotConstant = 1;

	/** Tolerance for dot comparison. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName=" └─ Tolerance", EditCondition="(Comparison==EPCGExComparison::NearlyEqual || Comparison==EPCGExComparison::NearlyNotEqual) && Domain==EPCGExAngularDomain::Amplitude", EditConditionHides, ClampMin=0, ClampMax=1))
	double DotTolerance = 0.1;

	/** Dot value use for comparison (In degrees) */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Degrees", EditCondition="ThresholdInput==EPCGExInputValueType::Constant && Domain==EPCGExAngularDomain::Degrees", EditConditionHides, ClampMin=0, ClampMax=180, Units="Degrees"))
	double DegreesConstant = 0;

	/** Tolerance for dot comparison. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName=" └─ Tolerance", EditCondition="(Comparison==EPCGExComparison::NearlyEqual || Comparison==EPCGExComparison::NearlyNotEqual) && Domain==EPCGExAngularDomain::Degrees", EditConditionHides, ClampMin=0, ClampMax=180, Units="Degrees"))
	double DegreesTolerance = 0.1;

	bool bUseAttribute = false;
	double ComparisonTolerance = 0;
	double ComparisonThreshold = 0;
	TSharedPtr<PCGExData::TBuffer<double>> ThresholdGetter;

	bool Init(const FPCGContext* InContext, const TSharedRef<PCGExData::FFacade>& InPrimaryDataCache);

	double GetComparisonThreshold(const int32 PointIndex) const;

	bool Test(const double A, const double B) const;
	bool Test(const double A, const int32 Index) const;
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExAttributeToTagComparisonDetails
{
	GENERATED_BODY()

	FPCGExAttributeToTagComparisonDetails()
	{
	}

	/** Type of Tag Name value */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable))
	EPCGExInputValueType TagNameInput = EPCGExInputValueType::Constant;

	/** Attribute to read tag name value from. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Tag Name (Attr)", EditCondition="TagNameInput!=EPCGExInputValueType::Constant", EditConditionHides, HideEditConditionToggle))
	FName TagNameAttribute = FName("Tag");

	/** Constant tag name value. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, DisplayName="Tag Name", EditCondition="TagNameInput==EPCGExInputValueType::Constant", EditConditionHides, HideEditConditionToggle))
	FString TagName = TEXT("Tag");

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, DisplayName="Match"))
	EPCGExStringMatchMode NameMatch = EPCGExStringMatchMode::Equals;

	/** Whether to do a tag value match or not. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable))
	bool bDoValueMatch = false;

	/** Expected value type, this is a strict check. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bDoValueMatch"))
	EPCGExComparisonDataType ValueType = EPCGExComparisonDataType::Numeric;

	/** Attribute to read tag name value from. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="bDoValueMatch"))
	FPCGAttributePropertyInputSelector ValueAttribute;

	/** Comparison */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, DisplayName="Comparison", EditCondition="bDoValueMatch && ValueType==EPCGExComparisonDataType::Numeric", EditConditionHides))
	EPCGExComparison NumericComparison = EPCGExComparison::NearlyEqual;

	/** Rounding mode for relative measures */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, EditCondition="bDoValueMatch && ValueType==EPCGExComparisonDataType::Numeric && (NumericComparison==EPCGExComparison::NearlyEqual || NumericComparison==EPCGExComparison::NearlyNotEqual)", EditConditionHides))
	double Tolerance = DBL_COMPARE_TOLERANCE;

	/** Comparison */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, DisplayName="Comparison", EditCondition="bDoValueMatch && ValueType==EPCGExComparisonDataType::String", EditConditionHides))
	EPCGExStringComparison StringComparison = EPCGExStringComparison::Contains;

	TSharedPtr<PCGEx::TAttributeBroadcaster<FString>> TagNameGetter;
	TSharedPtr<PCGEx::TAttributeBroadcaster<double>> NumericValueGetter;
	TSharedPtr<PCGEx::TAttributeBroadcaster<FString>> StringValueGetter;

	bool Init(const FPCGContext* InContext, const TSharedRef<PCGExData::FFacade>& InSourceDataFacade);

	bool Matches(const TSharedPtr<PCGExData::FTags>& InTags, const int32 SourceIndex, const FPCGPoint& SourcePoint) const;
	bool Matches(const TSharedPtr<PCGExData::FTags>& InTags, const PCGExData::FPointRef& SourcePointRef) const;
};

UENUM()
enum class EPCGExBitOp : uint8
{
	Set = 0 UMETA(DisplayName = "=", ToolTip="(Flags = Mask) Set the bit with the specified value."),
	AND = 1 UMETA(DisplayName = "AND", ToolTip="(Flags &= Mask) Output true if boths bits == 1, otherwise false."),
	OR  = 2 UMETA(DisplayName = "OR", ToolTip="(Flags |= Mask) Output true if any of the bits == 1, otherwise false."),
	NOT = 3 UMETA(DisplayName = "NOT", ToolTip="(Flags &= ~Mask) Like AND, but inverts the masks."),
	XOR = 4 UMETA(DisplayName = "XOR", ToolTip="(Flags ^= Mask) Invert the flag bit where the mask == 1."),
};

UENUM()
enum class EPCGExBitmaskMode : uint8
{
	Direct     = 0 UMETA(DisplayName = "Direct", ToolTip="Used for easy override mostly"),
	Individual = 1 UMETA(DisplayName = "Individual", ToolTip="Use an array to manually set the bits"),
	Composite  = 2 UMETA(DisplayName = "Composite", ToolTip="Use a ton of dropdown to set the bits"),
};

#pragma region Bitmasks


/*
UENUM(BlueprintType, meta=(DisplayName="[PCGEx] Bitflag 64"))
enum class EPCGExBitflag64 : int64
{
	None    = 0,
	Flag_1  = 1ULL << 0 UMETA(DisplayName = "Alpha"),
	Flag_2  = 1ULL << 1 UMETA(DisplayName = "Beta"),
	Flag_3  = 1ULL << 2 UMETA(DisplayName = "Gamma"),
	Flag_4  = 1ULL << 3 UMETA(DisplayName = "Delta"),
	Flag_5  = 1ULL << 4 UMETA(DisplayName = "Epsilon"),
	Flag_6  = 1ULL << 5 UMETA(DisplayName = "Zeta"),
	Flag_7  = 1ULL << 6 UMETA(DisplayName = "Eta"),
	Flag_8  = 1ULL << 7 UMETA(DisplayName = "Theta"),
	Flag_9  = 1ULL << 8 UMETA(DisplayName = "Iota"),
	Flag_10 = 1ULL << 9 UMETA(DisplayName = "Kappa"),
	Flag_11 = 1ULL << 10 UMETA(DisplayName = "Lambda"),
	Flag_12 = 1ULL << 11 UMETA(DisplayName = "Mu"),
	Flag_13 = 1ULL << 12 UMETA(DisplayName = "Nu"),
	Flag_14 = 1ULL << 13 UMETA(DisplayName = "Xi"),
	Flag_15 = 1ULL << 14 UMETA(DisplayName = "Omicron"),
	Flag_16 = 1ULL << 15 UMETA(DisplayName = "Pi"),
	Flag_17 = 1ULL << 16 UMETA(DisplayName = "Rho"),
	Flag_18 = 1ULL << 17 UMETA(DisplayName = "Sigma"),
	Flag_19 = 1ULL << 18 UMETA(DisplayName = "Tau"),
	Flag_20 = 1ULL << 19 UMETA(DisplayName = "Upsilon"),
	Flag_21 = 1ULL << 20 UMETA(DisplayName = "Phi"),
	Flag_22 = 1ULL << 21 UMETA(DisplayName = "Chi"),
	Flag_23 = 1ULL << 22 UMETA(DisplayName = "Psi"),
	Flag_24 = 1ULL << 23 UMETA(DisplayName = "Omega"),
	Flag_25 = 1ULL << 24 UMETA(DisplayName = "Ares"),
	Flag_26 = 1ULL << 25 UMETA(DisplayName = "Zeus"),
	Flag_27 = 1ULL << 26 UMETA(DisplayName = "Hera"),
	Flag_28 = 1ULL << 27 UMETA(DisplayName = "Apollo"),
	Flag_29 = 1ULL << 28 UMETA(DisplayName = "Hermes"),
	Flag_30 = 1ULL << 29 UMETA(DisplayName = "Athena"),
	Flag_31 = 1ULL << 30 UMETA(DisplayName = "Artemis"),
	Flag_32 = 1ULL << 31 UMETA(DisplayName = "Demeter"),
	Flag_33 = 1ULL << 32 UMETA(DisplayName = "Dionysus"),
	Flag_34 = 1ULL << 33 UMETA(DisplayName = "Hades"),
	Flag_35 = 1ULL << 34 UMETA(DisplayName = "Hephaestus"),
	Flag_36 = 1ULL << 35 UMETA(DisplayName = "Hera"),
	Flag_37 = 1ULL << 36 UMETA(DisplayName = "Hestia"),
	Flag_38 = 1ULL << 37 UMETA(DisplayName = "Poseidon"),
	Flag_39 = 1ULL << 38 UMETA(DisplayName = "Janus"),
	Flag_40 = 1ULL << 39 UMETA(DisplayName = "Mars"),
	Flag_41 = 1ULL << 40 UMETA(DisplayName = "Venus"),
	Flag_42 = 1ULL << 41 UMETA(DisplayName = "Jupiter"),
	Flag_43 = 1ULL << 42 UMETA(DisplayName = "Saturn"),
	Flag_44 = 1ULL << 43 UMETA(DisplayName = "Neptune"),
	Flag_45 = 1ULL << 44 UMETA(DisplayName = "Pluto"),
	Flag_46 = 1ULL << 45 UMETA(DisplayName = "Vesta"),
	Flag_47 = 1ULL << 46 UMETA(DisplayName = "Mercury"),
	Flag_48 = 1ULL << 47 UMETA(DisplayName = "Sol"),
	Flag_49 = 1ULL << 48 UMETA(DisplayName = "Luna"),
	Flag_50 = 1ULL << 49 UMETA(DisplayName = "Terra"),
	Flag_51 = 1ULL << 50 UMETA(DisplayName = "Vulcan"),
	Flag_52 = 1ULL << 51 UMETA(DisplayName = "Juno"),
	Flag_53 = 1ULL << 52 UMETA(DisplayName = "Ceres"),
	Flag_54 = 1ULL << 53 UMETA(DisplayName = "Minerva"),
	Flag_55 = 1ULL << 54 UMETA(DisplayName = "Bacchus"),
	Flag_56 = 1ULL << 55 UMETA(DisplayName = "Aurora"),
	Flag_57 = 1ULL << 56 UMETA(DisplayName = "Flora"),
	Flag_58 = 1ULL << 57 UMETA(DisplayName = "Faunus"),
	Flag_59 = 1ULL << 58 UMETA(DisplayName = "Iris"),
	Flag_60 = 1ULL << 59 UMETA(DisplayName = "Mithras"),
	Flag_61 = 1ULL << 60 UMETA(DisplayName = "Fortuna"),
	Flag_62 = 1ULL << 61 UMETA(DisplayName = "Bellona"),
	Flag_63 = 1ULL << 62 UMETA(DisplayName = "Fides"),
	Flag_64 = 1ULL << 63 UMETA(DisplayName = "Pax"),
};
ENUM_CLASS_FLAGS(EPCGExBitflag64)
*/

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 0-8 Bits Range"))
enum class EPCGExBitmask8_00_08 : uint8
{
	None   = 0,
	Flag_1 = 1 << 0 UMETA(DisplayName = "(0) Alpha"),
	Flag_2 = 1 << 1 UMETA(DisplayName = "(1) Beta"),
	Flag_3 = 1 << 2 UMETA(DisplayName = "(2) Gamma"),
	Flag_4 = 1 << 3 UMETA(DisplayName = "(3) Delta"),
	Flag_5 = 1 << 4 UMETA(DisplayName = "(4) Epsilon"),
	Flag_6 = 1 << 5 UMETA(DisplayName = "(5) Zeta"),
	Flag_7 = 1 << 6 UMETA(DisplayName = "(6) Eta"),
	Flag_8 = 1 << 7 UMETA(DisplayName = "(7) Theta"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_00_08)
using EPCGExBitmask8_00_08Bitmask = TEnumAsByte<EPCGExBitmask8_00_08>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 8-16 Bits Range"))
enum class EPCGExBitmask8_08_16 : uint8
{
	None    = 0,
	Flag_9  = 1 << 0 UMETA(DisplayName = "(8) Iota"),
	Flag_10 = 1 << 1 UMETA(DisplayName = "(9) Kappa"),
	Flag_11 = 1 << 2 UMETA(DisplayName = "(10) Lambda"),
	Flag_12 = 1 << 3 UMETA(DisplayName = "(11) Mu"),
	Flag_13 = 1 << 4 UMETA(DisplayName = "(12) Nu"),
	Flag_14 = 1 << 5 UMETA(DisplayName = "(13) Xi"),
	Flag_15 = 1 << 6 UMETA(DisplayName = "(14) Omicron"),
	Flag_16 = 1 << 7 UMETA(DisplayName = "(15) Pi"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_08_16)
using EPCGExBitmask8_08_16Bitmask = TEnumAsByte<EPCGExBitmask8_08_16>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 16-24 Bits Range"))
enum class EPCGExBitmask8_16_24 : uint8
{
	None    = 0,
	Flag_17 = 1 << 0 UMETA(DisplayName = "(16) Rho"),
	Flag_18 = 1 << 1 UMETA(DisplayName = "(17) Sigma"),
	Flag_19 = 1 << 2 UMETA(DisplayName = "(18) Tau"),
	Flag_20 = 1 << 3 UMETA(DisplayName = "(19) Upsilon"),
	Flag_21 = 1 << 4 UMETA(DisplayName = "(20) Phi"),
	Flag_22 = 1 << 5 UMETA(DisplayName = "(21) Chi"),
	Flag_23 = 1 << 6 UMETA(DisplayName = "(22) Psi"),
	Flag_24 = 1 << 7 UMETA(DisplayName = "(23) Omega"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_16_24)
using EPCGExBitmask8_16_24Bitmask = TEnumAsByte<EPCGExBitmask8_16_24>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 24-32 Bits Range"))
enum class EPCGExBitmask8_24_32 : uint8
{
	None    = 0,
	Flag_25 = 1 << 0 UMETA(DisplayName = "(24) Ares"),
	Flag_26 = 1 << 1 UMETA(DisplayName = "(25) Zeus"),
	Flag_27 = 1 << 2 UMETA(DisplayName = "(26) Hera"),
	Flag_28 = 1 << 3 UMETA(DisplayName = "(27) Apollo"),
	Flag_29 = 1 << 4 UMETA(DisplayName = "(28) Hermes"),
	Flag_30 = 1 << 5 UMETA(DisplayName = "(29) Athena"),
	Flag_31 = 1 << 6 UMETA(DisplayName = "(30) Artemis"),
	Flag_32 = 1 << 7 UMETA(DisplayName = "(31) Demeter"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_24_32)
using EPCGExBitmask8_24_32Bitmask = TEnumAsByte<EPCGExBitmask8_24_32>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 32-40 Bits Range"))
enum class EPCGExBitmask8_32_40 : uint8
{
	None    = 0,
	Flag_33 = 1 << 0 UMETA(DisplayName = "(32) Dionysus"),
	Flag_34 = 1 << 1 UMETA(DisplayName = "(33) Hades"),
	Flag_35 = 1 << 2 UMETA(DisplayName = "(34) Hephaestus"),
	Flag_36 = 1 << 3 UMETA(DisplayName = "(35) Hera"),
	Flag_37 = 1 << 4 UMETA(DisplayName = "(36) Hestia"),
	Flag_38 = 1 << 5 UMETA(DisplayName = "(37) Poseidon"),
	Flag_39 = 1 << 6 UMETA(DisplayName = "(38) Janus"),
	Flag_40 = 1 << 7 UMETA(DisplayName = "(39) Mars"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_32_40)
using EPCGExBitmask8_32_40Bitmask = TEnumAsByte<EPCGExBitmask8_32_40>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 40-48 Bits Range"))
enum class EPCGExBitmask8_40_48 : uint8
{
	None    = 0,
	Flag_41 = 1 << 0 UMETA(DisplayName = "(40) Venus"),
	Flag_42 = 1 << 1 UMETA(DisplayName = "(41) Jupiter"),
	Flag_43 = 1 << 2 UMETA(DisplayName = "(42) Saturn"),
	Flag_44 = 1 << 3 UMETA(DisplayName = "(43) Neptune"),
	Flag_45 = 1 << 4 UMETA(DisplayName = "(44) Pluto"),
	Flag_46 = 1 << 5 UMETA(DisplayName = "(45) Vesta"),
	Flag_47 = 1 << 6 UMETA(DisplayName = "(46) Mercury"),
	Flag_48 = 1 << 7 UMETA(DisplayName = "(47) Sol"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_40_48)
using EPCGExBitmask8_40_48Bitmask = TEnumAsByte<EPCGExBitmask8_40_48>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 48-56 Bits Range"))
enum class EPCGExBitmask8_48_56 : uint8
{
	None    = 0,
	Flag_49 = 1 << 0 UMETA(DisplayName = "(48) Luna"),
	Flag_50 = 1 << 1 UMETA(DisplayName = "(49) Terra"),
	Flag_51 = 1 << 2 UMETA(DisplayName = "(50) Vulcan"),
	Flag_52 = 1 << 3 UMETA(DisplayName = "(51) Juno"),
	Flag_53 = 1 << 4 UMETA(DisplayName = "(52) Ceres"),
	Flag_54 = 1 << 5 UMETA(DisplayName = "(53) Minerva"),
	Flag_55 = 1 << 6 UMETA(DisplayName = "(54) Bacchus"),
	Flag_56 = 1 << 7 UMETA(DisplayName = "(55) Aurora"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_48_56)
using EPCGExBitmask8_48_56Bitmask = TEnumAsByte<EPCGExBitmask8_48_56>;

UENUM(meta=(Bitflags, UseEnumValuesAsMaskValuesInEditor="true", DisplayName="[PCGEx] Bitflag 56-64 Bits Range"))
enum class EPCGExBitmask8_56_64 : uint8
{
	None    = 0,
	Flag_57 = 1 << 0 UMETA(DisplayName = "(56) Flora"),
	Flag_58 = 1 << 1 UMETA(DisplayName = "(57) Faunus"),
	Flag_59 = 1 << 2 UMETA(DisplayName = "(58) Iris"),
	Flag_60 = 1 << 3 UMETA(DisplayName = "(59) Mithras"),
	Flag_61 = 1 << 4 UMETA(DisplayName = "(60) Fortuna"),
	Flag_62 = 1 << 5 UMETA(DisplayName = "(61) Bellona"),
	Flag_63 = 1 << 6 UMETA(DisplayName = "(62) Fides"),
	Flag_64 = 1 << 7 UMETA(DisplayName = "(63) Pax"),
};

ENUM_CLASS_FLAGS(EPCGExBitmask8_56_64)
using EPCGExBitmask8_56_64Bitmask = TEnumAsByte<EPCGExBitmask8_56_64>;

#pragma endregion

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FClampedBit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0", ClampMax = "63"))
	uint8 BitIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bValue;

	FClampedBit() : BitIndex(0), bValue(false)
	{
	}

	int64 Get() const { return bValue ? (static_cast<int64>(0) | (1LL << BitIndex)) : (static_cast<int64>(0) & (~(1LL << BitIndex))); }

	bool operator==(const FClampedBit& Other) const { return BitIndex == Other.BitIndex; }
	friend uint32 GetTypeHash(const FClampedBit& Key) { return HashCombine(0, GetTypeHash(Key.BitIndex)); }
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FClampedBitOp
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0", ClampMax = "63"))
	uint8 BitIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	EPCGExBitOp Op = EPCGExBitOp::OR;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	bool bValue;

	FClampedBitOp() : BitIndex(0), bValue(true)
	{
	}

	int64 Get() const { return bValue ? (static_cast<int64>(0) | (1LL << BitIndex)) : (static_cast<int64>(0) & (~(1LL << BitIndex))); }

	bool operator==(const FClampedBitOp& Other) const { return BitIndex == Other.BitIndex; }
	friend uint32 GetTypeHash(const FClampedBitOp& Key) { return HashCombine(0, GetTypeHash(Key.BitIndex)); }
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExBitmask
{
	GENERATED_BODY()

	FPCGExBitmask()
	{
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable))
	EPCGExBitmaskMode Mode = EPCGExBitmaskMode::Individual;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="Mode==EPCGExBitmaskMode::Direct", EditConditionHides))
	int64 Bitmask = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, EditCondition="Mode==EPCGExBitmaskMode::Individual", TitleProperty="Bit # {BitIndex} = {bValue}", EditConditionHides))
	TArray<FClampedBit> Bits;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_00_08", DisplayName="0-8 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_00_08 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_08_16", DisplayName="8-16 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_08_16 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_16_24", DisplayName="16-24 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_16_24 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_24_32", DisplayName="24-32 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_24_32 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_32_40", DisplayName="32-40 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_32_40 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_40_48", DisplayName="40-48 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_40_48 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_48_56", DisplayName="48-56 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_48_56 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_56_64", DisplayName="56-64 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_56_64 = 0;

	int64 Get() const;
	void DoOperation(const EPCGExBitOp Op, int64& Flags) const;
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExBitmaskWithOperation
{
	GENERATED_BODY()

	FPCGExBitmaskWithOperation()
	{
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable))
	EPCGExBitmaskMode Mode = EPCGExBitmaskMode::Direct;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_Overridable, EditCondition="Mode==EPCGExBitmaskMode::Direct", EditConditionHides))
	int64 Bitmask = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, EditCondition="Mode==EPCGExBitmaskMode::Individual", TitleProperty="Bit # {BitIndex} = {bValue}", EditConditionHides))
	TArray<FClampedBitOp> Bits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta=(PCG_NotOverridable, EditCondition="Mode!=EPCGExBitmaskMode::Individual", EditConditionHides))
	EPCGExBitOp Op = EPCGExBitOp::OR;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_00_08", DisplayName="0-8 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_00_08 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_08_16", DisplayName="8-16 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_08_16 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_16_24", DisplayName="16-24 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_16_24 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_24_32", DisplayName="24-32 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_24_32 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_32_40", DisplayName="32-40 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_32_40 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_40_48", DisplayName="40-48 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_40_48 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_48_56", DisplayName="48-56 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_48_56 = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta=(PCG_NotOverridable, Bitmask, BitmaskEnum="/Script/PCGExtendedToolkit.EPCGExBitmask8_56_64", DisplayName="56-64 Bits", EditCondition="Mode==EPCGExBitmaskMode::Composite", EditConditionHides))
	uint8 Range_56_64 = 0;

	int64 Get() const;
	void DoOperation(int64& Flags) const;
};

#undef PCGEX_UNSUPPORTED_STRING_TYPES
#undef PCGEX_UNSUPPORTED_PATH_TYPES
