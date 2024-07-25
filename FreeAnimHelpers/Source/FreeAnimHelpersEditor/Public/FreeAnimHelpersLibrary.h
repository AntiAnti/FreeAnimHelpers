// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FreeAnimHelpersLibrary.generated.h"

class UAnimSequence;
class USkeletalMesh;
class UCurveFloat;
class UCurveVector;

/**
 * Global functions for Editor module
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UFreeAnimHelpersLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/* For reference pose: get bone transform in component space by bone name */
	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static FTransform GetBoneRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& BoneName);

	/* For reference pose: get transform of socket in component space by its name */
	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static FTransform GetSocketRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& SocketName);

	/* For reference pose: get bone transform in component space by bone index */
	static FTransform GetBoneRefPositionInComponentSpaceByIndex(const UAnimSequence* AnimationSequence, int32 BoneIndex);

	/* For reference pose: get bone transform in component space by bone index, for reference skeleton */
	static FTransform GetRefSkeletonBonePositionByIndex(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);

	/* For pose in animation: get bone transform in compnent space */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Bone Transform at Animation Time"), Category = "FreeAnimHelpersLibrary")
	static FTransform GetBonePositionAtTimeInCS(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time);

	/* For pose in animation: get socket transform in compnent space */
	UFUNCTION(BlueprintCallable, meta=(DisplayName="Get Socket Transform at Animation Time"), Category = "FreeAnimHelpersLibrary")
	static FTransform GetSocketPositionAtTimeInCS(const UAnimSequence* AnimationSequence, const FName& SocketName, float Time);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static void ResetSkinndeAssetRootBoneScale(USkeletalMesh* SkeletalMesh, bool bKeepModelSize = true);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static void AddFloatCurveKey(UCurveFloat* Curve, float Time, float Value, bool bInterpCubic);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static void AddVectorCurveKey(UCurveVector* Curve, float Time, FVector Value, bool bInterpCubic);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static void ClearFloatCurve(UCurveFloat* Curve);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static void ClearVectorCurve(UCurveVector* Curve);

	/* For pose in animation: get bone transform in compnent space (relative to another bone; use for optimization) */
	static FTransform GetBonePositionAtTimeInCS_ToParent(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time, const FTransform& ParentBonePos, const int32 ParentBoneIndex);

	/* Find axis of rotator the closest to being parallel to the specified vectors. Returns +1.f in Multiplier if co-directed and -1.f otherwise */
	static EAxis::Type FindCoDirection(const FRotator& BoneRotator, const FVector& Direction, float& ResultMultiplier);

	/* Find float curve */
	static const FFloatCurve* GetFloatCurve(const UAnimSequence* AnimationSequence, const FName& CurveName, FAnimationCurveIdentifier& OutCurveId);

	static void GetBonePoseForTime(const UAnimSequenceBase* AnimationSequenceBase, const FName& BoneName, float Time, bool bExtractRootMotion, FTransform& Pose, const USkeletalMesh* PreviewMesh = nullptr);
	static void GetBonePosesForTime(const UAnimSequenceBase* AnimationSequenceBase, const TArray<FName>& BoneNames, float Time, bool bExtractRootMotion, TArray<FTransform>& Poses, const USkeletalMesh* PreviewMesh = nullptr);
};
