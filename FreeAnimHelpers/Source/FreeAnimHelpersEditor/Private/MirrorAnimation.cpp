// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "MirrorAnimation.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "FreeAnimHelpersLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetMathLibrary.h"

UMirrorAnimation::UMirrorAnimation()
{
}

void UMirrorAnimation::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	int32 BonesNum = RefSkeleton.GetRawRefBoneNames().Num();
	TArray<FName> BoneNames;
	TArray<FTransform> BoneTransforms;
	TArray<FTransform> BoneTransformsNew;
	TArray<FTransform> BoneTransformsNewCS;

	FVector RootScaleMul;
	if (MirrorAxis == EFAHRegularAxis::X)
		RootScaleMul = FVector(-1.f, 1.f, 1.f);
	else if (MirrorAxis == EFAHRegularAxis::Y)
		RootScaleMul = FVector(1.f, -1.f, 1.f);
	else if (MirrorAxis == EFAHRegularAxis::Z)
		RootScaleMul = FVector(1.f, 1.f, -1.f);
	FTransform MirrorTr;
	MirrorTr.SetScale3D(RootScaleMul);

	// List all bones
	BoneNames.SetNum(BonesNum);
	BoneTransforms.SetNum(BonesNum);
	BoneTransformsNew.SetNum(BonesNum);
	BoneTransformsNewCS.SetNum(BonesNum);
	for (int32 BoneIndex = 0; BoneIndex < BonesNum; BoneIndex++)
	{
		BoneNames[BoneIndex] = RefSkeleton.GetBoneName(BoneIndex);
	}

	// Tracks to save data
	TMap<FName, FRawAnimSequenceTrack> OutTracks;

	// Initialize containers
	for (int32 BoneIndex = 0; BoneIndex < BonesNum; BoneIndex++)
	{
		FRawAnimSequenceTrack& Track = OutTracks.Add(BoneNames[BoneIndex]);
		Track.PosKeys.SetNumUninitialized(KeysNum);
		Track.RotKeys.SetNumUninitialized(KeysNum);
		Track.ScaleKeys.SetNumUninitialized(KeysNum);
	}

	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);

		for (int32 BoneIndex = 0; BoneIndex < BonesNum; BoneIndex++)
		{
			const auto& BoneName = BoneNames[BoneIndex];
			int32 ParentBoneIndex = RefSkeleton.GetParentIndex(BoneIndex);

			FTransform PoseLS;
			UFreeAnimHelpersLibrary::GetBonePoseForTime(AnimationSequence, BoneName, Time, false, PoseLS, AnimationSequence->GetPreviewMesh());

			if (BoneIndex == 0)
				BoneTransforms[BoneIndex] = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, BoneName, Time);
			else
				BoneTransforms[BoneIndex] = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS_ToParent(AnimationSequence, BoneName, Time, BoneTransforms[ParentBoneIndex], ParentBoneIndex);

			if (BoneIndex == 0)
			{
				BoneTransformsNewCS[BoneIndex] = FTransform::Identity;
				BoneTransformsNew[BoneIndex] = FTransform::Identity;
			}
			else
			{
				FTransform MirroredTrCS = BoneTransforms[BoneIndex];
				MirroredTrCS.Mirror((EAxis::Type)MirrorAxis, (EAxis::Type::None));
				//FTransform MirroredTrCS = BoneTransforms[BoneIndex] * MirrorTr;

				// need to cleanup scale
				FTransform CleanTrCS = FTransform(MirroredTrCS.GetTranslation());
				//CleanTrCS.SetScale3D(MirroredTrCS.GetScale3D().GetAbs());
				CleanTrCS.SetRotation(UKismetMathLibrary::MakeRotFromXY(-MirroredTrCS.GetRotation().GetAxisX(), MirroredTrCS.GetRotation().GetAxisY()).Quaternion());
				/*
				CleanTrCS.SetTranslation(MirroredTrCS.GetTranslation());
				if (MirrorAxis == EFAHRegularAxis::X)
					CleanTrCS.SetRotation(UKismetMathLibrary::MakeRotFromYZ(MirroredTrCS.GetRotation().GetAxisY(), MirroredTrCS.GetRotation().GetAxisZ()).Quaternion());
				else if (MirrorAxis == EFAHRegularAxis::Y)
					CleanTrCS.SetRotation(UKismetMathLibrary::MakeRotFromXZ(MirroredTrCS.GetRotation().GetAxisX(), MirroredTrCS.GetRotation().GetAxisZ()).Quaternion());
				else if (MirrorAxis == EFAHRegularAxis::Z)
					CleanTrCS.SetRotation(UKismetMathLibrary::MakeRotFromXY(MirroredTrCS.GetRotation().GetAxisX(), MirroredTrCS.GetRotation().GetAxisY()).Quaternion());
				CleanTrCS.SetScale3D(MirroredTrCS.GetScale3D().GetAbs());
				*/

				BoneTransformsNewCS[BoneIndex] = CleanTrCS;
				if (BoneIndex == 0)
					BoneTransformsNew[BoneIndex] = CleanTrCS;
				else
					BoneTransformsNew[BoneIndex] = CleanTrCS.GetRelativeTransform(BoneTransformsNewCS[ParentBoneIndex]);
			}

			OutTracks[BoneName].PosKeys[FrameIndex] = (FVector3f)BoneTransformsNew[BoneIndex].GetTranslation();
			OutTracks[BoneName].RotKeys[FrameIndex] = (FQuat4f)BoneTransformsNew[BoneIndex].GetRotation();
			OutTracks[BoneName].ScaleKeys[FrameIndex] = (FVector3f)BoneTransformsNew[BoneIndex].GetScale3D();
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

FRotator UMirrorAnimation::AddLocalRotation(const FRotator& AdditionRot, const FRotator& BaseRot)
{
	return UKismetMathLibrary::ComposeRotators(BaseRot.GetInverse(), AdditionRot.GetInverse()).GetInverse();
}