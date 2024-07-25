// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "FingersCurl.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "FreeAnimHelpersLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetMathLibrary.h"

UFingersCurl::UFingersCurl()
{
	const FRotator DefValue = FRotator(0.f, -15.f, 0.f);
	if (HandRight.Num() == 0)
	{
		HandRight.Add(TEXT("thumb_03_r"), DefValue);
		HandRight.Add(TEXT("index_03_r"), DefValue);
		HandRight.Add(TEXT("middle_03_r"), DefValue);
		HandRight.Add(TEXT("ring_03_r"), DefValue);
		HandRight.Add(TEXT("pinky_03_r"), DefValue);
	}
	if (HandLeft.Num() == 0)
	{
		HandLeft.Add(TEXT("thumb_03_l"), DefValue);
		HandLeft.Add(TEXT("index_03_l"), DefValue);
		HandLeft.Add(TEXT("middle_03_l"), DefValue);
		HandLeft.Add(TEXT("ring_03_l"), DefValue);
		HandLeft.Add(TEXT("pinky_03_l"), DefValue);
	}
}

void UFingersCurl::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	TArray<FName> BoneNames;
	TMap<FName, FRotator> FingersAddend;

	if (bApplyRightHand)
	{
		FingersAddend = HandRight;
	}
	if (bApplyLeftHand)
	{
		FingersAddend.Append(HandLeft);
	}
	if (FingersAddend.Num() == 0)
	{
		return;
	}

	TMap<FName, FRotator> SecondaryFingerBones;
	for (const auto& LastBone : FingersAddend)
	{
		int32 Bone3Index = RefSkeleton.FindBoneIndex(LastBone.Key);
		if (Bone3Index == INDEX_NONE) return;
		int32 Bone2Index = RefSkeleton.GetParentIndex(Bone3Index);
		if (Bone2Index == INDEX_NONE) return;
		int32 Bone1Index = RefSkeleton.GetParentIndex(Bone2Index);
		if (Bone1Index == INDEX_NONE) return;

		SecondaryFingerBones.Add(RefSkeleton.GetBoneName(Bone2Index), LastBone.Value);
		SecondaryFingerBones.Add(RefSkeleton.GetBoneName(Bone1Index), LastBone.Value);
	}

	FingersAddend.Append(SecondaryFingerBones);
	FingersAddend.GetKeys(BoneNames);
	TArray<FTransform> BoneTransforms;
	BoneTransforms.SetNum(BoneNames.Num());

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

		// get current transforms
		UFreeAnimHelpersLibrary::GetBonePosesForTime(AnimationSequence, BoneNames, Time, false, BoneTransforms, AnimationSequence->GetPreviewMesh());

		for (int32 i = 0; i < BoneNames.Num(); i++)
		{
			const FName& BoneName = BoneNames[i];
			const FTransform& LocalTransform = BoneTransforms[i];

			//FRotator LocalRotation = LocalTransform.Rotator();
			//UKismetMathLibrary::ComposeRotators(LocalRotation, FingersAddend[BoneName]);

			FRotator LocalRotation = AddLocalRotation(FingersAddend[BoneName], LocalTransform.Rotator());

			OutTracks[BoneName].PosKeys[FrameIndex] = (FVector3f)LocalTransform.GetTranslation();
			OutTracks[BoneName].RotKeys[FrameIndex] = (FQuat4f)LocalRotation.Quaternion();
			OutTracks[BoneName].ScaleKeys[FrameIndex] = (FVector3f)LocalTransform.GetScale3D();
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

FRotator UFingersCurl::AddLocalRotation(const FRotator& AdditionRot, const FRotator& BaseRot)
{
	return UKismetMathLibrary::ComposeRotators(BaseRot.GetInverse(), AdditionRot.GetInverse()).GetInverse();
}