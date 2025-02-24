﻿// Copyright 2025 Timothé Lapetite and contributors
// Released under the MIT license https://opensource.org/license/MIT/

#pragma once

#include "CoreMinimal.h"
#include "PCGEx.h"
#include "PCGExMT.h"
#include "PCGExFitting.h"
#include "Curve/CurveUtil.h"
#include "Data/PCGExData.h"


#include "PCGExGeo.generated.h"

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExGeo2DProjectionDetails
{
	GENERATED_BODY()

	FPCGExGeo2DProjectionDetails()
	{
	}

	explicit FPCGExGeo2DProjectionDetails(const FPCGExGeo2DProjectionDetails& Other)
		: bSupportLocalNormal(Other.bLocalProjectionNormal),
		  ProjectionNormal(Other.ProjectionNormal),
		  bLocalProjectionNormal(Other.bLocalProjectionNormal),
		  LocalNormal(Other.LocalNormal)
	{
	}

	explicit FPCGExGeo2DProjectionDetails(const bool InSupportLocalNormal)
		: bSupportLocalNormal(InSupportLocalNormal)
	{
	}

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, HideInDetailPanel, Hidden, EditConditionHides, EditCondition="false"))
	bool bSupportLocalNormal = true;

	/** Normal vector of the 2D projection plane. Defaults to Up for XY projection. Used as fallback when using invalid local normal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, ShowOnlyInnerProperties))
	FVector ProjectionNormal = FVector::UpVector;

	/** */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition="bSupportLocalNormal", EditConditionHides))
	bool bLocalProjectionNormal = false;

	/** Local attribute to fetch projection normal from */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_NotOverridable, EditCondition="bSupportLocalNormal && bLocalProjectionNormal", EditConditionHides))
	FPCGAttributePropertyInputSelector LocalNormal;

	TSharedPtr<PCGExData::TBuffer<FVector>> NormalGetter;
	FQuat ProjectionQuat = FQuat::Identity;
	FQuat ProjectionInverseQuat = FQuat::Identity;

	bool Init(const FPCGContext* InContext, const TSharedPtr<PCGExData::FFacade>& PointDataFacade);

	~FPCGExGeo2DProjectionDetails()
	{
	}

	FQuat GetQuat(const int32 PointIndex) const;

	FVector Project(const FVector& InPosition, const int32 PointIndex) const;
	FVector Project(const FVector& InPosition) const;
	FVector ProjectFlat(const FVector& InPosition) const;
	FVector ProjectFlat(const FVector& InPosition, const int32 PointIndex) const;
	FTransform ProjectFlat(const FTransform& InTransform) const;
	FTransform ProjectFlat(const FTransform& InTransform, const int32 PointIndex) const;

	template <typename T>
	void ProjectFlat(const TSharedPtr<PCGExData::FFacade>& InFacade, TArray<T>& OutPositions) const
	{
		const TArray<FPCGPoint>& InPoints = InFacade->Source->GetInOut()->GetPoints();
		const int32 NumVectors = InPoints.Num();
		PCGEx::InitArray(OutPositions, NumVectors);
		for (int i = 0; i < NumVectors; i++) { OutPositions[i] = T(ProjectFlat(InPoints[i].Transform.GetLocation(), i)); }
	}

	template <typename T>
	void ProjectFlat(const TSharedPtr<PCGExData::FFacade>& InFacade, TArray<T>& OutPositions, const PCGExMT::FScope& Scope) const
	{
		const TArray<FPCGPoint>& InPoints = InFacade->Source->GetInOut()->GetPoints();
		const int32 NumVectors = InPoints.Num();
		if (OutPositions.Num() < NumVectors) { PCGEx::InitArray(OutPositions, NumVectors); }

		for (int i = Scope.Start; i < Scope.End; i++) { OutPositions[i] = T(ProjectFlat(InPoints[i].Transform.GetLocation(), i)); }
	}

	void Project(const TArray<FVector>& InPositions, TArray<FVector>& OutPositions) const;
	void Project(const TArrayView<FVector>& InPositions, TArray<FVector>& OutPositions) const;
	void Project(const TArray<FVector>& InPositions, TArray<FVector2D>& OutPositions) const;
	void Project(const TArrayView<FVector>& InPositions, TArray<FVector2D>& OutPositions) const;
	void Project(const TArray<FPCGPoint>& InPoints, TArray<FVector>& OutPositions) const;
};

USTRUCT(BlueprintType)
struct PCGEXTENDEDTOOLKIT_API FPCGExLloydSettings
{
	GENERATED_BODY()

	FPCGExLloydSettings()
	{
	}

	/** The number of Lloyd iteration to apply prior to generating the graph. Very expensive!*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bLloydRelax"))
	int32 Iterations = 3;

	/** The influence of each iteration. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Settings, meta = (PCG_Overridable, EditCondition="bLloydRelax", ClampMin=0, ClampMax=1))
	double Influence = 1;

	FORCEINLINE bool IsValid() const { return Iterations > 0 && Influence > 0; }
};

UENUM()
enum class EPCGExCellCenter : uint8
{
	Balanced     = 0 UMETA(DisplayName = "Balanced", ToolTip="Pick centroid if circumcenter is out of bounds, otherwise uses circumcenter."),
	Circumcenter = 1 UMETA(DisplayName = "Canon (Circumcenter)", ToolTip="Uses Delaunay cells' circumcenter."),
	Centroid     = 2 UMETA(DisplayName = "Centroid", ToolTip="Uses Delaunay cells' averaged vertice positions.")
};

namespace PCGExGeo
{
	PCGEX_CTX_STATE(State_ExtractingMesh)

	bool IsWinded(const EPCGExWinding Winding, const bool bIsInputClockwise);
	bool IsWinded(const EPCGExWindingMutation Winding, const bool bIsInputClockwise);

	struct PCGEXTENDEDTOOLKIT_API FPolygonInfos
	{
		double Area = 0;
		bool bIsClockwise = false;
		double Perimeter = 0;
		double Compactness = 0;

		explicit FPolygonInfos(const TArray<FVector2D>& InPolygon);

		bool IsWinded(const EPCGExWinding Winding) const;
	};

	template <typename T>
	FORCEINLINE double Det(const T& A, const T& B) { return A.X * B.Y - A.Y * B.X; }

	FORCEINLINE static double S_U(
		const FVector& A, const FVector& B, const FVector& C, const FVector& D,
		const FVector& E, const FVector& F, const FVector& G, const FVector& H)
	{
		return (A.Z - B.Z) * (C.X * D.Y - D.X * C.Y) - (E.Z - F.Z) * (G.X * H.Y - H.X * G.Y);
	};

	FORCEINLINE static double S_D(
		const int FirstComponent, const int SecondComponent,
		FVector A, FVector B, FVector C)
	{
		return
			A[FirstComponent] * (B[SecondComponent] - C[SecondComponent]) +
			B[FirstComponent] * (C[SecondComponent] - A[SecondComponent]) +
			C[FirstComponent] * (A[SecondComponent] - B[SecondComponent]);
	};

	FORCEINLINE static double S_E(
		const int FirstComponent, const int SecondComponent,
		const FVector& A, const FVector& B, const FVector& C, const FVector& D,
		const double RA, const double RB, const double RC, const double RD, const double UVW)
	{
		return (RA * S_D(FirstComponent, SecondComponent, B, C, D) - RB * S_D(FirstComponent, SecondComponent, C, D, A) +
			RC * S_D(FirstComponent, SecondComponent, D, A, B) - RD * S_D(FirstComponent, SecondComponent, A, B, C)) / UVW;
	};

	static double S_SQ(const FVector& P) { return P.X * P.X + P.Y * P.Y + P.Z * P.Z; };

	bool FindSphereFrom4Points(const FVector& A, const FVector& B, const FVector& C, const FVector& D, FSphere& OutSphere);
	bool FindSphereFrom4Points(const TArrayView<FVector>& Positions, const int32 (&Vtx)[4], FSphere& OutSphere);
	void GetCircumcenter(const TArrayView<FVector>& Positions, const int32 (&Vtx)[3], FVector& OutCircumcenter);

	void GetCentroid(const TArrayView<FVector>& Positions, const int32 (&Vtx)[4], FVector& OutCentroid);
	void GetCentroid(const TArrayView<FVector>& Positions, const int32 (&Vtx)[3], FVector& OutCentroid);
	void GetLongestEdge(const TArrayView<FVector>& Positions, const int32 (&Vtx)[3], uint64& Edge);
	void GetLongestEdge(const TArrayView<FVector>& Positions, const int32 (&Vtx)[4], uint64& Edge);
	void PointsToPositions(const TArray<FPCGPoint>& Points, TArray<FVector>& OutPositions);
	FVector GetBarycentricCoordinates(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);
	bool IsPointInTriangle(const FVector& P, const FVector& A, const FVector& B, const FVector& C);

	template <typename T>
	static double AngleCCW(const T& A, const T& B)
	{
		double Angle = FMath::Atan2((A[0] * B[1] - A[1] * B[0]), (A[0] * B[0] + A[1] * B[1]));
		return Angle < 0 ? Angle = TWO_PI + Angle : Angle;
	}

	/**
		 *	 Leave <---.Apex-----> Arrive (Direction)
		 *		   . '   |    '  .  
		 *		A----Anchor---------B
		 */
	struct PCGEXTENDEDTOOLKIT_API FApex
	{
		FApex()
		{
		}

		FApex(const FVector& Start, const FVector& End, const FVector& InApex);

		FVector Direction = FVector::ZeroVector;
		FVector Anchor = FVector::ZeroVector;
		FVector TowardStart = FVector::ZeroVector;
		FVector TowardEnd = FVector::ZeroVector;
		double Alpha = 0;

		FVector GetAnchorNormal(const FVector& Location) const { return (Anchor - Location).GetSafeNormal(); }

		void Scale(const double InScale);
		void Extend(const double InSize);

		static FApex FromStartOnly(const FVector& Start, const FVector& InApex) { return FApex(Start, InApex, InApex); }
		static FApex FromEndOnly(const FVector& End, const FVector& InApex) { return FApex(InApex, End, InApex); }
	};

	struct PCGEXTENDEDTOOLKIT_API FExCenterArc
	{
		double Radius = 0;
		double Theta = 0;
		double SinTheta = 0;

		FVector Center = FVector::ZeroVector;
		FVector Normal = FVector::ZeroVector;
		FVector Hand = FVector::ZeroVector;
		FVector OtherHand = FVector::ZeroVector;

		bool bIsLine = false;

		FExCenterArc()
		{
		}


		/**
		 * ExCenter arc from 3 points.
		 * The arc center will be opposite to B
		 * @param A 
		 * @param B 
		 * @param C 
		 */
		FExCenterArc(const FVector& A, const FVector& B, const FVector& C);

		/**
		 * ExCenter arc from 2 segments.
		 * The arc center will be opposite to B
		 * @param A1 
		 * @param B1 
		 * @param A2 
		 * @param B2
		 * @param MaxLength 
		 */
		FExCenterArc(const FVector& A1, const FVector& B1, const FVector& A2, const FVector& B2, const double MaxLength = 100000);

		FORCEINLINE double GetLength() const { return Radius * Theta; }

		/**
		 * 
		 * @param Alpha 0-1 normalized range on the arc
		 * @return 
		 */
		FVector GetLocationOnArc(const double Alpha) const;
	};
}

namespace PCGExGeoTasks
{
	class FTransformPointIO final : public PCGExMT::FPCGExIndexedTask
	{
	public:
		FTransformPointIO(const int32 InTaskIndex,
		                  const TSharedPtr<PCGExData::FPointIO>& InPointIO,
		                  const TSharedPtr<PCGExData::FPointIO>& InToBeTransformedIO,
		                  FPCGExTransformDetails* InTransformDetails) :
			FPCGExIndexedTask(InTaskIndex),
			PointIO(InPointIO),
			ToBeTransformedIO(InToBeTransformedIO),
			TransformDetails(InTransformDetails)
		{
		}

		TSharedPtr<PCGExData::FPointIO> PointIO;
		TSharedPtr<PCGExData::FPointIO> ToBeTransformedIO;
		FPCGExTransformDetails* TransformDetails = nullptr;

		virtual void ExecuteTask(const TSharedPtr<PCGExMT::FTaskManager>& AsyncManager) override;
	};
}
