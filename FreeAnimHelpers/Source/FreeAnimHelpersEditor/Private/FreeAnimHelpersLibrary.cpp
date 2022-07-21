// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "FreeAnimHelpersLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"

FTransform UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& BoneName)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	return GetBoneRefPositionInComponentSpaceByIndex(AnimationSequence, RefSkeleton.FindBoneIndex(BoneName));
}

FTransform UFreeAnimHelpersLibrary::GetSocketRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& SocketName)
{
	const USkeletalMeshSocket* Socket = AnimationSequence->GetSkeleton()->FindSocket(SocketName);
	if (!Socket)
	{
		return FTransform::Identity;
	}

	FTransform BoneTr = GetBoneRefPositionInComponentSpace(AnimationSequence, Socket->BoneName);
	return FTransform(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale) * BoneTr;
}

FTransform UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpaceByIndex(const UAnimSequence* AnimationSequence, int32 BoneIndex)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetPreviewMesh()
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 TransformIndex = BoneIndex;
	FTransform tr_bone = FTransform::Identity;

	// stop on root
	while (TransformIndex != INDEX_NONE)
	{
		tr_bone *= RefPoseSpaceBaseTMs[TransformIndex];
		TransformIndex = RefSkeleton.GetParentIndex(TransformIndex);
	}
	tr_bone.NormalizeRotation();

	return tr_bone;
}

FTransform UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time)
{
	return GetBonePositionAtTimeInCS_ToParent(AnimationSequence, BoneName, Time, FTransform(), INDEX_NONE);
}

FTransform UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS_ToParent(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time, const FTransform& ParentBonePos, const int32 ParentBoneIndex)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetPreviewMesh()
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 TransformIndex = RefSkeleton.FindBoneIndex(BoneName);
	FTransform tr_bone = FTransform::Identity;

	// @TODO: optimize by using GetBonePosesForTime and set of bones
	while (TransformIndex != INDEX_NONE && TransformIndex != ParentBoneIndex)
	{
		FTransform ParentBoneTr;
		UAnimationBlueprintLibrary::GetBonePoseForTime(AnimationSequence, RefSkeleton.GetBoneName(TransformIndex), Time, false, ParentBoneTr);

		EBoneTranslationRetargetingMode::Type RetargetType = Skeleton->GetBoneTranslationRetargetingMode(TransformIndex);
		if (RetargetType == EBoneTranslationRetargetingMode::Type::Skeleton)
		{
			ParentBoneTr.SetTranslationAndScale3D(RefPoseSpaceBaseTMs[TransformIndex].GetTranslation(), RefPoseSpaceBaseTMs[TransformIndex].GetScale3D());
		}

		tr_bone *= ParentBoneTr;
		TransformIndex = RefSkeleton.GetParentIndex(TransformIndex);
	}

	if (ParentBoneIndex != INDEX_NONE && TransformIndex == ParentBoneIndex)
	{
		tr_bone *= ParentBonePos;
	}

	tr_bone.NormalizeRotation();

	return tr_bone;
}

/* Find axis of rotator the closest to being parallel to the specified vectors. Returns +1.f in Multiplier if co-directed and -1.f otherwise */
EAxis::Type UFreeAnimHelpersLibrary::FindCoDirection(const FRotator& BoneRotator, const FVector& Direction, float& ResultMultiplier)
{
	EAxis::Type RetAxis;
	FVector dir = Direction;
	dir.Normalize();

	const FVector v1 = FRotationMatrix(BoneRotator).GetScaledAxis(EAxis::X);
	const FVector v2 = FRotationMatrix(BoneRotator).GetScaledAxis(EAxis::Y);
	const FVector v3 = FRotationMatrix(BoneRotator).GetScaledAxis(EAxis::Z);

	const float dp1 = FVector::DotProduct(v1, dir);
	const float dp2 = FVector::DotProduct(v2, dir);
	const float dp3 = FVector::DotProduct(v3, dir);

	if (FMath::Abs(dp1) > FMath::Abs(dp2) && FMath::Abs(dp1) > FMath::Abs(dp3)) {
		RetAxis = EAxis::X;
		ResultMultiplier = dp1 > 0.f ? 1.f : -1.f;
	}
	else if (FMath::Abs(dp2) > FMath::Abs(dp1) && FMath::Abs(dp2) > FMath::Abs(dp3)) {
		RetAxis = EAxis::Y;
		ResultMultiplier = dp2 > 0.f ? 1.f : -1.f;
	}
	else {
		RetAxis = EAxis::Z;
		ResultMultiplier = dp3 > 0.f ? 1.f : -1.f;
	}

	return RetAxis;
}

const FFloatCurve* UFreeAnimHelpersLibrary::GetFloatCurve(const UAnimSequence* AnimationSequence, const FName& CurveName, FAnimationCurveIdentifier& OutCurveId)
{
	// Get Container name
	const FName ContainerName = UAnimationBlueprintLibrary::RetrieveContainerNameForCurve(AnimationSequence, CurveName);
	if (ContainerName == NAME_None) return nullptr;
	// Read Curve ID
	const FSmartName CurveSmartName = UAnimationBlueprintLibrary::RetrieveSmartNameForCurve(AnimationSequence, CurveName, ContainerName);
	OutCurveId = FAnimationCurveIdentifier(CurveSmartName, ERawCurveTrackTypes::RCT_Float);
	// Get Curve Data
	return static_cast<const FFloatCurve*>(AnimationSequence->GetDataModel()->FindCurve(OutCurveId));
}