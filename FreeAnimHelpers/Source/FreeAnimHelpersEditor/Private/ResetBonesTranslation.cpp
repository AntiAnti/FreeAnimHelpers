// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "ResetBonesTranslation.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"

UResetBonesTranslation::UResetBonesTranslation()
	: PreviewMesh(nullptr)
{
}

void UResetBonesTranslation::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	if (!IsValid(PreviewMesh))
	{
		PreviewMesh = AnimationSequence->GetPreviewMesh();
		if (!IsValid(PreviewMesh)) return;
	}

	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = PreviewMesh->GetRefSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();
	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	TArray<FTransform> Bones;
	TArray<FVector> BoneRefTranslation;
	TArray<FName> BoneNames;
	TMap<FName, FRawAnimSequenceTrack> OutTracks;
	Bones.Reserve(RefSkeleton.GetNum());
	BoneNames.Reserve(RefSkeleton.GetNum());
	OutTracks.Reserve(RefSkeleton.GetNum());

	for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); BoneIndex++)
	{
		EBoneTranslationRetargetingMode::Type RetargetType = Skeleton->GetBoneTranslationRetargetingMode(BoneIndex);
		if (RetargetType == EBoneTranslationRetargetingMode::Type::Skeleton)
		{
			const FName BoneName = RefSkeleton.GetBoneName(BoneIndex);
			BoneNames.Add(BoneName);
			BoneRefTranslation.Add(RefPoseSpaceBaseTMs[BoneIndex].GetTranslation());
			Bones.Add(FTransform());

			auto& Track = OutTracks.Add(BoneName, FRawAnimSequenceTrack());
			Track.PosKeys.SetNum(KeysNum);
			Track.RotKeys.SetNum(KeysNum);
			Track.ScaleKeys.SetNum(KeysNum);
		}
	}

	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		UAnimationBlueprintLibrary::GetBonePosesForFrame(AnimationSequence, BoneNames, FrameIndex, false, Bones);

		for (int32 Index = 0; Index < BoneNames.Num(); Index++)
		{
			const FName BoneName = BoneNames[Index];
			OutTracks[BoneName].PosKeys[FrameIndex] = (FVector3f)BoneRefTranslation[Index];
			OutTracks[BoneName].RotKeys[FrameIndex] = (FQuat4f)Bones[Index].GetRotation();
			OutTracks[BoneName].ScaleKeys[FrameIndex] = (FVector3f)Bones[Index].GetScale3D();
		}
	}

	// Save new keys in DataModel
	IAnimationDataController& Controller = AnimationSequence->GetController();
	for (const auto& BoneName : BoneNames)
	{
		Controller.RemoveBoneTrack(BoneName);
		Controller.AddBoneTrack(BoneName);
		Controller.SetBoneTrackKeys(BoneName, OutTracks[BoneName].PosKeys, OutTracks[BoneName].RotKeys, OutTracks[BoneName].ScaleKeys);
	}
}
