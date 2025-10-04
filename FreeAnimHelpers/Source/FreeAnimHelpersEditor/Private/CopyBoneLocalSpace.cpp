// (c) Yuri N. K. 2025. All rights reserved.
// ykasczc@gmail.com

#include "CopyBoneLocalSpace.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "FreeAnimHelpersLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetMathLibrary.h"

UCopyBoneLocalSpace::UCopyBoneLocalSpace()
{
	if (Bones.IsEmpty())
	{
		Bones.Add(FBoneCopyWrapper(TEXT("thumb_03_r"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("index_03_r"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("middle_03_r"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("ring_03_r"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("pinky_03_r"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("weapon_r"), 1));

		Bones.Add(FBoneCopyWrapper(TEXT("thumb_03_l"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("index_03_l"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("middle_03_l"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("ring_03_l"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("pinky_03_l"), 3));
		Bones.Add(FBoneCopyWrapper(TEXT("weapon_l"), 1));
	}
}

void UCopyBoneLocalSpace::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	TArray<FName> BoneNames;
	TArray<FTransform> BoneTransformsSrc, BoneTransformsDst;
	TMap<FName, int32> BoneIndices;
	TMap<FName, FBoneCopyRuntimeData> BonesData;

	if (!IsValid(SourceSequence))
	{
		return;
	}

	for (const auto& NewChain : Bones)
	{
		int32 BoneIndex = RefSkeleton.FindBoneIndex(NewChain.ChainEndBoneName);
		if (BoneIndex != INDEX_NONE)
		{
			BoneIndices.Add(NewChain.ChainEndBoneName, BoneIndex);
			BonesData.Add(NewChain.ChainEndBoneName, FBoneCopyRuntimeData(NewChain));

			for (int32 i = 1; i < NewChain.ChainLength; i++)
			{
				BoneIndex = RefSkeleton.GetParentIndex(BoneIndex);
				if (BoneIndex <= 0) continue;

				const FName BName = RefSkeleton.GetBoneName(BoneIndex);

				BoneIndices.Add(BName, BoneIndex);
				BonesData.Add(BName, FBoneCopyRuntimeData(NewChain));
			}
		}
	}

	BoneIndices.GenerateKeyArray(BoneNames);
	BoneTransformsSrc.SetNum(BoneNames.Num());
	BoneTransformsDst.SetNum(BoneNames.Num());

	// Tracks to save data
	TMap<FName, FRawAnimSequenceTrack> OutTracks;

	// Initialize containers
	for (const auto& BoneName : BoneNames)
	{
		FRawAnimSequenceTrack& Track = OutTracks.Add(BoneName);
		Track.PosKeys.SetNumUninitialized(KeysNum);
		Track.RotKeys.SetNumUninitialized(KeysNum);
		Track.ScaleKeys.SetNumUninitialized(KeysNum);
	}

	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);
		float SrcTime = Time;
		float SrcPlayLength = SourceSequence->GetPlayLength();

		if (Time > SrcPlayLength)
		{
			if (bLoopSourceData)
			{
				while (SrcTime > SrcPlayLength) SrcTime -= SrcPlayLength;
			}
			else
			{
				SrcTime = SrcPlayLength;
			}
		}

		// get current transforms in source sequence
		UFreeAnimHelpersLibrary::GetBonePosesForTime(SourceSequence, BoneNames, SrcTime, false, BoneTransformsSrc, AnimationSequence->GetPreviewMesh());
		// get current transforms in target sequence
		UFreeAnimHelpersLibrary::GetBonePosesForTime(AnimationSequence, BoneNames, Time, false, BoneTransformsDst, AnimationSequence->GetPreviewMesh());

		for (int32 i = 0; i < BoneNames.Num(); i++)
		{
			const FName& BoneName = BoneNames[i];
			FTransform& LocalTransformSrc = BoneTransformsSrc[i];
			const FTransform& LocalTransformDst = BoneTransformsDst[i];

			if (UAnimationBlueprintLibrary::DoesCurveExist(SourceSequence, BoneName, ERawCurveTrackTypes::RCT_Transform))
			{
				const FTransformCurve& c = SourceSequence->GetDataModel()->GetTransformCurve(FAnimationCurveIdentifier(BoneName, ERawCurveTrackTypes::RCT_Transform));
				LocalTransformSrc = c.Evaluate(SrcTime, 1.f) * LocalTransformSrc;
			}

			OutTracks[BoneName].PosKeys[FrameIndex] = BonesData[BoneName].bCopyTranslation ? (FVector3f)LocalTransformSrc.GetTranslation() : (FVector3f)LocalTransformDst.GetTranslation();
			OutTracks[BoneName].RotKeys[FrameIndex] = BonesData[BoneName].bCopyRotation ? (FQuat4f)LocalTransformSrc.GetRotation() : (FQuat4f)LocalTransformDst.GetRotation();
			OutTracks[BoneName].ScaleKeys[FrameIndex] = (FVector3f)LocalTransformDst.GetScale3D();
		}
	}

	// Save new keys in DataModel
	IAnimationDataController& Controller = AnimationSequence->GetController();
	for (const auto& Track : OutTracks)
	{
		const FName& BoneName = Track.Key;
		Controller.RemoveBoneTrack(BoneName);
#if ENGINE_MINOR_VERSION < 2
		Controller.AddBoneTrack(BoneName);
#else
		Controller.AddBoneCurve(BoneName);
#endif
		Controller.SetBoneTrackKeys(BoneName, Track.Value.PosKeys, Track.Value.RotKeys, Track.Value.ScaleKeys);
	}
}