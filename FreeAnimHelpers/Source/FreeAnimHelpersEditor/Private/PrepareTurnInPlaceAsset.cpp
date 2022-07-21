// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "PrepareTurnInPlaceAsset.h"
#include "FreeAnimHelpersLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/Skeleton.h"
#include "ReferenceSkeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"

UPrepareTurnInPlaceAsset::UPrepareTurnInPlaceAsset()
	: PelvisBoneName(TEXT("pelvis"))
	, bSnapZeroFrameToCenter(true)
	, bTurningToRight(true)
	, bDefaultRootBoneOffset(true)
	, RootBoneOffset(FVector2D(0.3f, 1.f))
{
}

void UPrepareTurnInPlaceAsset::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	// Skeleton data
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = IsValid(AnimationSequence->GetPreviewMesh())
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: Skeleton->GetReferenceSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 PelvisIndex = RefSkeleton.FindBoneIndex(PelvisBoneName);
	if (PelvisIndex == INDEX_NONE)
	{
		return;
	}
	int32 PelvisParentIndex = RefSkeleton.GetParentIndex(PelvisIndex);
	FName PelvisParentName = RefSkeleton.GetBoneName(PelvisParentIndex);

	FVector RootOffset;
	FVector GlobalTranslationOffset;
	FTransform ComponentToPelvisRotation;

	const FTransform PelvisCSRefSpace = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, PelvisBoneName);
	ComponentToPelvisRotation = FTransform(PelvisCSRefSpace.GetTranslation()).GetRelativeTransform(PelvisCSRefSpace);

	if (bDefaultRootBoneOffset)
	{
		const FVector RelRootTr = RefPoseSpaceBaseTMs[0].GetRelativeTransform(PelvisCSRefSpace).GetTranslation();
		RootOffset = FVector(RelRootTr.X, RelRootTr.Y, 0.f);
	}
	else
	{
		RootOffset = FVector(RootBoneOffset.X, RootBoneOffset.Y, 0.f);
	}

	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();
	const float AnimationDuration = AnimationSequence->GetPlayLength();
	const float DirectionMul = bTurningToRight ? 1.f : -1.f;

	FRawAnimSequenceTrack OutTrack;
	OutTrack.PosKeys.SetNum(KeysNum);
	OutTrack.RotKeys.SetNum(KeysNum);
	OutTrack.ScaleKeys.SetNum(KeysNum);

	// Create curves
	const FName CurveNameRootX = TEXT("Root_X");
	const FName CurveNameRootY = TEXT("Root_Y");
	if (!UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, CurveNameRootX, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::AddCurve(AnimationSequence, CurveNameRootX, ERawCurveTrackTypes::RCT_Float);
	}
	if (!UAnimationBlueprintLibrary::DoesCurveExist(AnimationSequence, CurveNameRootY, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::AddCurve(AnimationSequence, CurveNameRootY, ERawCurveTrackTypes::RCT_Float);
	}
	FRichCurve Curve_X, Curve_Y;

	// Process animation
	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);
		float TurnAngle = FRotator::NormalizeAxis((Time / AnimationDuration) * 360.f * DirectionMul);

		FTransform PelvisParentCS = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, PelvisParentName, Time);
		FTransform PelvisCS = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS_ToParent(AnimationSequence, PelvisBoneName, Time, PelvisParentCS, PelvisParentIndex);

		// Get location and rotation of virtual root bone (out real root isn't animated)
		FQuat VirtualRootRotation = FRotator(0.f, TurnAngle, 0.f).Quaternion() * RefPoseSpaceBaseTMs[0].GetRotation();
		VirtualRootRotation.Normalize();
		FVector VirtualRootLocation = PelvisCS.GetTranslation() + VirtualRootRotation.RotateVector(RootOffset);
		VirtualRootLocation.Z = 0.f;

		// Initialize initial offset?
		if (FrameIndex == 0 && bSnapZeroFrameToCenter)
		{
			GlobalTranslationOffset = VirtualRootLocation * -1.f;
		}

		// Apply global offset
		PelvisCS.AddToTranslation(GlobalTranslationOffset);
		VirtualRootLocation += GlobalTranslationOffset;

		// Fill curves
		Curve_X.AddKey(Time, VirtualRootLocation.X);
		Curve_Y.AddKey(Time, VirtualRootLocation.Y);

		// Remove rotation and X-Y offset from pelvis pose
		FTransform VirtualRootCS = FTransform(VirtualRootRotation, VirtualRootLocation);
		PelvisCS = PelvisCS.GetRelativeTransform(VirtualRootCS);

		// Build local pelvis transform
		FTransform PelvisRel = PelvisCS.GetRelativeTransform(PelvisParentCS);

		// Save in track
		OutTrack.PosKeys[FrameIndex] = (FVector3f)PelvisRel.GetTranslation();
		OutTrack.RotKeys[FrameIndex] = (FQuat4f)PelvisRel.GetRotation();
		OutTrack.ScaleKeys[FrameIndex] = (FVector3f)RefPoseSpaceBaseTMs[0].GetScale3D();
	}

	// Save new keys in DataModel
	IAnimationDataController& Controller = AnimationSequence->GetController();

	// Update pelvis
	Controller.RemoveBoneTrack(PelvisBoneName);
	Controller.AddBoneTrack(PelvisBoneName);
	Controller.SetBoneTrackKeys(PelvisBoneName, OutTrack.PosKeys, OutTrack.RotKeys, OutTrack.ScaleKeys);

	// Fill curves
	FAnimationCurveIdentifier CurveRootXId, CurveRootYId;
	UFreeAnimHelpersLibrary::GetFloatCurve(AnimationSequence, CurveNameRootX, CurveRootXId);
	UFreeAnimHelpersLibrary::GetFloatCurve(AnimationSequence, CurveNameRootY, CurveRootYId);

	Controller.SetCurveKeys(CurveRootXId, Curve_X.Keys);
	Controller.SetCurveKeys(CurveRootYId, Curve_Y.Keys);
}
