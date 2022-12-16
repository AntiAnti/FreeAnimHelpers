// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "AnimateIKBones.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "FreeAnimHelpersLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"

UAnimateIKBones::UAnimateIKBones()
{
	if (IKtoFK.Num() == 0)
	{
		IKtoFK.Add(TEXT("ik_foot_root"), TEXT("root"));
		IKtoFK.Add(TEXT("ik_foot_r"), TEXT("foot_r"));
		IKtoFK.Add(TEXT("ik_foot_l"), TEXT("foot_l"));
		IKtoFK.Add(TEXT("ik_hand_root"), TEXT("root"));
		IKtoFK.Add(TEXT("ik_hand_gun"), TEXT("hand_r"));
		IKtoFK.Add(TEXT("ik_hand_r"), TEXT("hand_r"));
		IKtoFK.Add(TEXT("ik_hand_l"), TEXT("hand_l"));
	}
}

void UAnimateIKBones::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	// Names of source bones
	TMap<FName, FTransform> HumanoidBones;
	// Bone name to parent bone name
	TMap<FName, FName> Parents;
	// Frame values
	TMap<FName, FTransform> FramePos;
	// Tracks to save data
	TMap<FName, FRawAnimSequenceTrack> OutTracks;

	// Initialize containers
	for (const auto& BonePair : IKtoFK)
	{
		if (RefSkeleton.FindBoneIndex(BonePair.Key) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Can't find bone: %s"), *BonePair.Key.ToString());
			return;
		}
		if (RefSkeleton.FindBoneIndex(BonePair.Value) == INDEX_NONE)
		{
			UE_LOG(LogTemp, Warning, TEXT("Can't find bone: %s"), *BonePair.Value.ToString());
			return;
		}

		FRawAnimSequenceTrack& Track = OutTracks.Add(BonePair.Key);
		Track.PosKeys.SetNumUninitialized(KeysNum);
		Track.RotKeys.SetNumUninitialized(KeysNum);
		Track.ScaleKeys.SetNumUninitialized(KeysNum);

		if (!Parents.Contains(BonePair.Key))
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(BonePair.Key));
			Parents.Add(BonePair.Key, (ParentIndex == INDEX_NONE) ? BonePair.Key : RefSkeleton.GetBoneName(ParentIndex));
		}
		if (!Parents.Contains(BonePair.Value))
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(BonePair.Value));
			Parents.Add(BonePair.Value, (ParentIndex == INDEX_NONE) ? BonePair.Value : RefSkeleton.GetBoneName(ParentIndex));
		}

		if (IKtoFK.Contains(BonePair.Value))
		{
			// Use runtime transforms of IK bones
			FramePos.Add(BonePair.Value);
		}
		else
		{
			// Doesn't add IK bones
			HumanoidBones.Add(BonePair.Value);
		}
	}

	for (const auto& ChildToParent : Parents)
	{
		if (!HumanoidBones.Contains(ChildToParent.Value) && !FramePos.Contains(ChildToParent.Value))
		{
			FramePos.Add(ChildToParent.Value);
		}
	}

	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);

		for (auto& BonePair : HumanoidBones)
		{
			BonePair.Value = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, BonePair.Key, Time);
		}

		for (const auto& BonePair : IKtoFK)
		{
			const FName& IKBone = BonePair.Key;
			const FName& FKBone = BonePair.Value;

			const FTransform SourcePos = FramePos.Contains(FKBone)
				? FramePos[FKBone]
				: HumanoidBones[FKBone];
			const FName ParentBoneName = Parents[IKBone];
			FTransform ParentPos = FTransform::Identity;
			if (ParentBoneName != IKBone)
			{
				ParentPos = FramePos.Contains(ParentBoneName)
					? FramePos[ParentBoneName]
					: HumanoidBones[ParentBoneName];
			}

			FTransform RelativeTr = SourcePos.GetRelativeTransform(ParentPos);
			// Save to track
			OutTracks[IKBone].PosKeys[FrameIndex] = (FVector3f)RelativeTr.GetTranslation();
			OutTracks[IKBone].RotKeys[FrameIndex] = (FQuat4f)RelativeTr.GetRotation();
			OutTracks[IKBone].ScaleKeys[FrameIndex] = (FVector3f)RelativeTr.GetScale3D();

			if (FTransform* PositionToSave = FramePos.Find(IKBone))
			{
				*PositionToSave = SourcePos;
			}
		}
	}

	// Save new keys in DataModel
	IAnimationDataController& Controller = AnimationSequence->GetController();
	for (const auto& Track : OutTracks)
	{
		const FName& BoneName = Track.Key;
		Controller.RemoveBoneTrack(BoneName);
		Controller.AddBoneTrack(BoneName);
		Controller.SetBoneTrackKeys(BoneName, Track.Value.PosKeys, Track.Value.RotKeys, Track.Value.ScaleKeys);
	}
}
