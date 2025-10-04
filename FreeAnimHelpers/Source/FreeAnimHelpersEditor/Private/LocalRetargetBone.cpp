// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "LocalRetargetBone.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "FreeAnimHelpersLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetMathLibrary.h"

ULocalRetargetBone::ULocalRetargetBone()
{
	BoneNames = { TEXT("weapon_joint_r"), TEXT("weapon_joint_l") };
	SourceBoneNames = { TEXT("rhang_tag_bone"), TEXT("lhang_tag_bone") };
}

void ULocalRetargetBone::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	FString SourceAssetName = SourceAnimPath / AnimationSequence->GetName() + TEXT(".") + AnimationSequence->GetName();
	UAnimSequence* SourceAnimSequence = LoadObject<UAnimSequence>(NULL, *SourceAssetName, NULL, LOAD_None, NULL);
	if (!IsValid(SourceAnimSequence))
	{
		UE_LOG(LogTemp, Log, TEXT("Can't find source animation: %s"), *SourceAssetName);
	}
	USkeleton* SourceSkeleton = SourceAnimSequence->GetSkeleton();
	const FReferenceSkeleton& SourceRefSkeleton = Skeleton->GetReferenceSkeleton();

	const int32 RetargetBonesNum = BoneNames.Num();
	if (RetargetBonesNum != SourceBoneNames.Num())
	{
		return;
	}

	TArray<int32> BoneIndex, SourceBoneIndex;
	BoneIndex.SetNum(RetargetBonesNum);
	SourceBoneIndex.SetNum(RetargetBonesNum);
	for (int32 i = 0; i < RetargetBonesNum; i++)
	{
		BoneIndex[i] = RefSkeleton.FindBoneIndex(BoneNames[i]);
		SourceBoneIndex[i] = SourceRefSkeleton.FindBoneIndex(SourceBoneNames[i]);
	}

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

		for (int32 i = 0; i < RetargetBonesNum; i++)
		{
			const FName& TargetBoneName = BoneNames[i];
			const FName& SourceBoneName = SourceBoneNames[i];
			const bool bRightHand = TargetBoneName.ToString().EndsWith(TEXT("_r"));

			// get current local transform of source bone
			FTransform SourceTr;
			UFreeAnimHelpersLibrary::GetBonePoseForTime(SourceAnimSequence, SourceBoneName, Time, false, SourceTr, SourceAnimSequence->GetPreviewMesh());

			// convertion is hardcoded now
			const FTransform HandReorientTr = bRightHand
				? FTransform(UKismetMathLibrary::MakeRotFromXY(FVector(0.f, -1.f, 0.f), FVector(0.f, 0.f, -1.f)))
				: FTransform(UKismetMathLibrary::MakeRotFromXY(FVector(0.f, -1.f, 0.f), FVector(0.f, 0.f, 1.f)));

			FTransform GenericTargetBoneTr = SourceTr.GetRelativeTransform(HandReorientTr);
			GenericTargetBoneTr.ScaleTranslation(TranslationScale);

			const FTransform TargetHandToGenTr = bRightHand
				? FTransform(UKismetMathLibrary::MakeRotFromXY(FVector(-1.f, 0.f, 0.f), FVector(0.f, 0.f, -1.f)))
				: FTransform(UKismetMathLibrary::MakeRotFromXY(FVector(1.f, 0.f, 0.f), FVector(0.f, 0.f, -1.f)));

			FTransform TargetLocalTr = GenericTargetBoneTr * TargetHandToGenTr;

			OutTracks[TargetBoneName].PosKeys[FrameIndex] = (FVector3f)TargetLocalTr.GetTranslation();
			OutTracks[TargetBoneName].RotKeys[FrameIndex] = (FQuat4f)TargetLocalTr.GetRotation();
			OutTracks[TargetBoneName].ScaleKeys[FrameIndex] = (FVector3f)TargetLocalTr.GetScale3D();
		}
	}

	// Save new keys in DataModel
	IAnimationDataController& Controller = AnimationSequence->GetController();
	for (const auto& Track : OutTracks)
	{
		const FName& BoneName = Track.Key;

		Controller.RemoveBoneTrack(BoneName);
		Controller.AddBoneCurve(BoneName);
		Controller.SetBoneTrackKeys(BoneName, Track.Value.PosKeys, Track.Value.RotKeys, Track.Value.ScaleKeys);
	}
}

FRotator ULocalRetargetBone::AddLocalRotation(const FRotator& AdditionRot, const FRotator& BaseRot)
{
	return UKismetMathLibrary::ComposeRotators(BaseRot.GetInverse(), AdditionRot.GetInverse()).GetInverse();
}