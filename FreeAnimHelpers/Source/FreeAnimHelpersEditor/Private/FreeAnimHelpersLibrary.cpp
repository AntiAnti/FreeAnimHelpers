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
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
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

FTransform UFreeAnimHelpersLibrary::GetBonePositionAtTimeInComponentSpace(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetPreviewMesh()
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 TransformIndex = RefSkeleton.FindBoneIndex(BoneName);
	FTransform tr_bone = FTransform::Identity;

	// @TODO: optimize by using GetBonePosesForTime and set of bones
	while (TransformIndex != INDEX_NONE)
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
	tr_bone.NormalizeRotation();

	return tr_bone;
}