// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "Curves/CurveVector.h"
#include "DistanceCurveModifierEx.generated.h"

/** Axes to calculate the distance value from */
UENUM(BlueprintType)
enum class EFAHDistanceCurve_Axis : uint8
{
	X,
	Y,
	Z,
	XY,
	XZ,
	YZ,
	XYZ
};

/** Axes to calculate the distance value from */
UENUM(BlueprintType)
enum class EFAHDMotionRefPoint : uint8
{
	BeginAtStart,
	StopAtEnd,
	FindFromMotion
};

/** Approximate direction in skeletal mesh space */
UENUM(BlueprintType)
enum class EMATMovementDirection : uint8
{
	MD_X					UMETA(DisplayName = "X+"),
	MD_XY					UMETA(DisplayName = "X+ Y+"),
	MD_Y					UMETA(DisplayName = "Y+"),
	MD_XnY					UMETA(DisplayName = "X- Y+"),
	MD_Xn					UMETA(DisplayName = "X-"),
	MD_XnYn					UMETA(DisplayName = "X- Y-"),
	MD_Yn					UMETA(DisplayName = "Y-"),
	MD_XYn					UMETA(DisplayName = "X+ Y-"),
	MD_Z					UMETA(DisplayName = "Z (Vertical)"),
};

/** Builds traveling distance information from animation without root motion, then bakes it to a curve.
 * A negative value indicates distance remaining to a stop or pivot point.
 * A positive value indicates distance traveled from a start point or from the beginning of the clip.
 * Also can bake reversed curves (time and distance axes swapped, with distance mapped to animation length)
 */
UCLASS()
class UDistanceCurveModifierEx : public UAnimationModifier
{
	GENERATED_BODY()

public:

	/** Rate used to sample the animation. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = "1"))
	int32 SampleRate = 30;

	/** Name for the generated curve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName CurveName = TEXT("Distance");

	/** Name of pelvis/hips bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName PelvisBone = TEXT("pelvis");

	/** Name of right foot bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName FootRightBone = TEXT("foot_r");

	/** Name of left foot bone */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	FName FootLeftBone = TEXT("foot_l");

	/** Movement direction since first frame in skeletal mesh space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EMATMovementDirection InitialDirection = EMATMovementDirection::MD_Y;

	/** Root motion speed must be below this threshold to be considered stopped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (EditCondition = "ReferencePoint==EFAHDMotionRefPoint::FindFromMotion"))
	float StopSpeedThreshold = 5.0f;

	/** Axes to calculate the distance value from. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EFAHDistanceCurve_Axis Axis = EFAHDistanceCurve_Axis::XY;

	/** Root motion is considered to be stopped at the clip's end */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	EFAHDMotionRefPoint ReferencePoint = EFAHDMotionRefPoint::BeginAtStart;

	/** Bake Time(Distance) function to curve rather then Distance(Time) to save */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bOptimizedDistanceCurveFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	bool bRootMotionToCurves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion")
	bool bRootMotionToRootBone = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="bRootMotionToRootBone && !bRootMotionToCurves"), Category = "Root Motion")
	bool bRootMotionFromCurves = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion", meta=(EditCondition="bRootMotionToCurves || bRootMotionToRootBone"))
	float SmoothenAccuracy = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Root Motion", meta = (EditCondition = "bRootMotionToCurves"))
	FName RootCurveName = TEXT("RootMotion");

	virtual void OnApply_Implementation(UAnimSequence* Animation) override;
	virtual void OnRevert_Implementation(UAnimSequence* Animation) override;

private:

	/** Helper functions to calculate the magnitude of a vector only considering a specific axis or axes */
	static float CalculateMagnitude(const FVector& Vector, EFAHDistanceCurve_Axis Axis);
	static float CalculateMagnitudeSq(const FVector& Vector, EFAHDistanceCurve_Axis Axis);
	static void AddVectorCurveKey(FRuntimeVectorCurve& Curve, float Time, const FVector& Value);
	static FVector GetVectorCurveValue(const FRuntimeVectorCurve& Curve, int32 KeyIndex);
	static FVector DirectionAsVector(EMATMovementDirection Direction);

	// fill RootOffset curve
	void GenerateRootMotion(UAnimSequence* AnimationSequence);
	// find delta offset in interval
	FVector FindRootMotion(float StartTime, float DeltaTime) const;
	// find delta offset in interval
	FVector FindRootMotionInRange(float StartTime, float EndTime) const;

	// Root motion from starting point
	FRuntimeVectorCurve RootOffset;
};