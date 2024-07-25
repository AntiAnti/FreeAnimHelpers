// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "TorsoOffset.h"
#include "FreeAnimHelpersLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Runtime/Launch/Resources/Version.h"

#define __rotator_direction(Rotator, Axis) FRotationMatrix(Rotator).GetScaledAxis(Axis)

UTorsoOffset::UTorsoOffset()
	: PelvisBoneName(TEXT("pelvis"))
	, FootBoneName_Right(TEXT("foot_r"))
	, FootBoneName_Left(TEXT("foot_l"))
	, RightOrientationConvert(FRotator(0.f, 0.f, 90.f))
	, LeftOrientationConvert(FRotator(0.f, 180.f, -90.f))
	, TorsoOffset(FVector::ZeroVector)
{
}

void UTorsoOffset::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	// Skeleton data
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const FReferenceSkeleton& RefSkeleton = IsValid(AnimationSequence->GetPreviewMesh())
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: Skeleton->GetReferenceSkeleton();
	TMap<FName, FRawAnimSequenceTrack> OutTracks;
	TArray<FName> RightLegBones, LeftLegBones;

	const int32 FootNameId = 0, CalfNameId = 1, ThighNameId = 2, ThighParentNameId = 3;

	FTransform JointTargetOffsetR, JointTargetOffsetL;
	FTransform ThighOrientationConverterR, ThighOrientationConverterL;
	FTransform CalfOrientationConverterR, CalfOrientationConverterL;
	EAxis::Type RightAxisR = EAxis::Type::Z, RightAxisL = EAxis::Type::Z;

	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	if (RefSkeleton.FindBoneIndex(PelvisBoneName) == INDEX_NONE) return;
	if (RefSkeleton.FindBoneIndex(FootBoneName_Right) == INDEX_NONE) return;
	if (RefSkeleton.FindBoneIndex(FootBoneName_Left) == INDEX_NONE) return;

	int32 PelvisParentIndex = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(PelvisBoneName));
	FName PelvisParentName = (PelvisParentIndex == INDEX_NONE) ? NAME_None : RefSkeleton.GetBoneName(PelvisParentIndex);

	OutTracks.Add(PelvisBoneName);
	RightLegBones.Add(FootBoneName_Right);
	LeftLegBones.Add(FootBoneName_Left);
	while (!RightLegBones.IsValidIndex(ThighParentNameId))
	{
		OutTracks.Add(RightLegBones.Last());
		OutTracks.Add(LeftLegBones.Last());

		int32 Parent = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(RightLegBones.Last()));
		if (Parent == INDEX_NONE) return;
		RightLegBones.Add(RefSkeleton.GetBoneName(Parent));

		Parent = RefSkeleton.GetParentIndex(RefSkeleton.FindBoneIndex(LeftLegBones.Last()));
		if (Parent == INDEX_NONE) return;
		LeftLegBones.Add(RefSkeleton.GetBoneName(Parent));
	}

	for (auto& Track : OutTracks)
	{
		Track.Value.PosKeys.SetNumUninitialized(KeysNum);
		Track.Value.RotKeys.SetNumUninitialized(KeysNum);
		Track.Value.ScaleKeys.SetNumUninitialized(KeysNum);
	}

	float ForwMul, DownMul;

	// Knee offsets
	const FVector ForwardDirection = FVector::RightVector;
	FTransform FootBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, RightLegBones[FootNameId]);
	FTransform CalfBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, RightLegBones[CalfNameId]);
	FTransform ThighBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, RightLegBones[ThighNameId]);
	FVector JointTargetLoc = ForwardDirection * 5.f + CalfBoneRefTr.GetTranslation();
	JointTargetOffsetR = FTransform(JointTargetLoc).GetRelativeTransform(CalfBoneRefTr);

	EAxis::Type ForwAxis = UFreeAnimHelpersLibrary::FindCoDirection(ThighBoneRefTr.Rotator(), ForwardDirection, ForwMul);
	EAxis::Type DownAxis = UFreeAnimHelpersLibrary::FindCoDirection(ThighBoneRefTr.Rotator(), (CalfBoneRefTr.GetTranslation() - ThighBoneRefTr.GetTranslation()), DownMul);
	/**/ if (ForwAxis != EAxis::Type::X && DownAxis != EAxis::Type::X) RightAxisR = EAxis::Type::X;
	else if (ForwAxis != EAxis::Type::Y && DownAxis != EAxis::Type::Y) RightAxisR = EAxis::Type::Y;

	// orientation converters
	FRotator tmpRot = UKismetMathLibrary::MakeRotFromXY(CalfBoneRefTr.GetTranslation() - ThighBoneRefTr.GetTranslation(), __rotator_direction(ThighBoneRefTr.Rotator(), RightAxisR));
	ThighOrientationConverterR = ThighBoneRefTr.GetRelativeTransform(FTransform(tmpRot, ThighBoneRefTr.GetTranslation()));
	tmpRot = UKismetMathLibrary::MakeRotFromXY(FootBoneRefTr.GetTranslation() - CalfBoneRefTr.GetTranslation(), __rotator_direction(CalfBoneRefTr.Rotator(), RightAxisR));
	CalfOrientationConverterR = CalfBoneRefTr.GetRelativeTransform(FTransform(tmpRot, CalfBoneRefTr.GetTranslation()));

	UE_LOG(LogTemp, Log, TEXT("ThighOrientationConverterR = %s"), *ThighOrientationConverterR.ToString());
	UE_LOG(LogTemp, Log, TEXT("CalfOrientationConverterR = %s"), *CalfOrientationConverterR.ToString());

	// Knee offsets
	FootBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, LeftLegBones[FootNameId]);
	CalfBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, LeftLegBones[CalfNameId]);
	ThighBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, LeftLegBones[ThighNameId]);
	JointTargetLoc = ForwardDirection * 5.f + CalfBoneRefTr.GetTranslation();
	JointTargetOffsetL = FTransform(JointTargetLoc).GetRelativeTransform(CalfBoneRefTr);

	ForwAxis = UFreeAnimHelpersLibrary::FindCoDirection(ThighBoneRefTr.Rotator(), ForwardDirection, ForwMul);
	DownAxis = UFreeAnimHelpersLibrary::FindCoDirection(ThighBoneRefTr.Rotator(), (CalfBoneRefTr.GetTranslation() - ThighBoneRefTr.GetTranslation()), DownMul);
	/**/ if (ForwAxis != EAxis::Type::X && DownAxis != EAxis::Type::X) RightAxisL = EAxis::Type::X;
	else if (ForwAxis != EAxis::Type::Y && DownAxis != EAxis::Type::Y) RightAxisL = EAxis::Type::Y;

	// orientation converters
	tmpRot = UKismetMathLibrary::MakeRotFromXY(CalfBoneRefTr.GetTranslation() - ThighBoneRefTr.GetTranslation(), __rotator_direction(ThighBoneRefTr.Rotator(), RightAxisL));
	ThighOrientationConverterL = ThighBoneRefTr.GetRelativeTransform(FTransform(tmpRot, ThighBoneRefTr.GetTranslation()));
	tmpRot = UKismetMathLibrary::MakeRotFromXY(FootBoneRefTr.GetTranslation() - CalfBoneRefTr.GetTranslation(), __rotator_direction(CalfBoneRefTr.Rotator(), RightAxisL));
	CalfOrientationConverterL = CalfBoneRefTr.GetRelativeTransform(FTransform(tmpRot, CalfBoneRefTr.GetTranslation()));

	UE_LOG(LogTemp, Log, TEXT("ThighOrientationConverterL = %s"), *ThighOrientationConverterL.ToString());
	UE_LOG(LogTemp, Log, TEXT("CalfOrientationConverterL = %s"), *CalfOrientationConverterL.ToString());

	// Update animation
	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);

		// get current bone transforms in component space
		FTransform PelvisParentTr = FTransform::Identity;
		if (PelvisParentIndex != INDEX_NONE) PelvisParentTr = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, PelvisParentName, Time);
		const FTransform OldPelvisTr = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, PelvisBoneName, Time);
		FTransform PelvisTr = OldPelvisTr;
		const FTransform FootRightTr = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, FootBoneName_Right, Time);
		const FTransform FootLeftTr = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, FootBoneName_Left, Time);

		// calculate pelvis
		PelvisTr.AddToTranslation(TorsoOffset);
		const FTransform PelvisTrRel = PelvisTr.GetRelativeTransform(PelvisParentTr);

		// update pelvis
		OutTracks[PelvisBoneName].PosKeys[FrameIndex] = (FVector3f)PelvisTrRel.GetTranslation();
		OutTracks[PelvisBoneName].RotKeys[FrameIndex] = (FQuat4f)PelvisTrRel.GetRotation();
		OutTracks[PelvisBoneName].ScaleKeys[FrameIndex] = (FVector3f)PelvisTrRel.GetScale3D();

		// stack is dying here
		LegIK(AnimationSequence, Time, OldPelvisTr, JointTargetOffsetR, FTransform(RightOrientationConvert), FTransform(RightOrientationConvert), RightLegBones, RightAxisR,
			OutTracks, FrameIndex);
		LegIK(AnimationSequence, Time, OldPelvisTr, JointTargetOffsetL, FTransform(LeftOrientationConvert), FTransform(LeftOrientationConvert), LeftLegBones, RightAxisL,
			OutTracks, FrameIndex);
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

void UTorsoOffset::LegIK(
	UAnimSequence* AnimationSequence,
	float Time,
	const FTransform& OldPelvisPos,
	const FTransform& KneeOffset,
	const FTransform& ThighOrientationConverter,
	const FTransform& CalfOrientationConverter,
	const TArray<FName>& BoneNames,
	EAxis::Type RightAxis,
	TMap<FName, FRawAnimSequenceTrack>& OutTracks,
	int32 FrameIndex) const
{
	const int32 FootNameId = 0, CalfNameId = 1, ThighNameId = 2, ThighParentNameId = 3;

	TArray<FTransform> BonePos;
	BonePos.SetNumUninitialized(BoneNames.Num());
	for (int32 i = 0; i <= ThighParentNameId; i++)
	{
		if (i < ThighParentNameId || BoneNames[i] != PelvisBoneName)
		{
			BonePos[i] = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, BoneNames[i], Time);
			if (FrameIndex == 5)
			{
				UE_LOG(LogTemp, Log, TEXT("bone[%i] name = %s"), i, *BoneNames[i].ToString());
			}
		}
		else
		{
			if (FrameIndex == 5)
			{
				UE_LOG(LogTemp, Log, TEXT("ThighParentName = %s"), *PelvisBoneName.ToString());
			}
			BonePos[i] = OldPelvisPos;
		}
	}

	// save current location as target
	FVector EffectorLocation = BonePos[FootNameId].GetTranslation();
	// apply offset!
	for (auto& Pos : BonePos) Pos.AddToTranslation(TorsoOffset);

	float KneeTargetAlpha = FVector::DotProduct(
		(BonePos[CalfNameId].GetTranslation() - BonePos[ThighNameId].GetTranslation()).GetSafeNormal(),
		(BonePos[FootNameId].GetTranslation() - BonePos[ThighNameId].GetTranslation()).GetSafeNormal());
	KneeTargetAlpha = (KneeTargetAlpha < 0.85f)
		? 0.f
		: (KneeTargetAlpha - 0.85f) / (1.f - 0.85f);

	FVector KneeTargetModified = (KneeOffset * BonePos[CalfNameId]).GetTranslation();
	FVector KneeTarget = FMath::Lerp(KneeTarget, KneeTargetModified, KneeTargetAlpha);

	//  compute Two Bone IK
	FVector OutCalfLocation, OutFootLocation;
	UKismetAnimationLibrary::K2_TwoBoneIK(
		BonePos[ThighNameId].GetTranslation(),
		BonePos[CalfNameId].GetTranslation(),
		BonePos[FootNameId].GetTranslation(),
		KneeTarget,
		EffectorLocation,
		OutCalfLocation,
		OutFootLocation,
		false, 0.5f, 1.f);

	// Calc generic rotation
	FRotator ThighRotatorBase = UKismetMathLibrary::MakeRotFromXY(OutCalfLocation - BonePos[ThighNameId].GetTranslation(), __rotator_direction(BonePos[ThighNameId].Rotator(), RightAxis));
	FRotator CalfRotatorBase = UKismetMathLibrary::MakeRotFromXY(OutFootLocation - OutCalfLocation, __rotator_direction(BonePos[CalfNameId].Rotator(), RightAxis));

	// Build adjusted component-space positions of leg bones
	FTransform FinalFrameThighTr = BonePos[ThighNameId]; FinalFrameThighTr.SetRotation((ThighOrientationConverter * FTransform(ThighRotatorBase)).GetRotation());
	FTransform FinalFrameCalfTr = FTransform((CalfOrientationConverter * FTransform(CalfRotatorBase)).GetRotation(), OutCalfLocation, BonePos[CalfNameId].GetScale3D());
	FTransform FinalFrameFootTr = BonePos[FootNameId]; FinalFrameFootTr.SetTranslation(OutFootLocation);

	// Calc relative transforms of the leg bones
	FTransform RelThighTr = FinalFrameThighTr.GetRelativeTransform(BonePos[ThighParentNameId]);
	FTransform RelCalfTr = FinalFrameCalfTr.GetRelativeTransform(FinalFrameThighTr);
	FTransform RelFootTr = FinalFrameFootTr.GetRelativeTransform(FinalFrameCalfTr);

	// Apply rotations to tracks

	OutTracks[BoneNames[ThighNameId]].PosKeys[FrameIndex] = (FVector3f)RelThighTr.GetTranslation();
	OutTracks[BoneNames[ThighNameId]].RotKeys[FrameIndex] = (FQuat4f)RelThighTr.GetRotation();
	OutTracks[BoneNames[ThighNameId]].ScaleKeys[FrameIndex] = (FVector3f)RelThighTr.GetScale3D();

	OutTracks[BoneNames[CalfNameId]].PosKeys[FrameIndex] = (FVector3f)RelCalfTr.GetTranslation();
	OutTracks[BoneNames[CalfNameId]].RotKeys[FrameIndex] = (FQuat4f)RelCalfTr.GetRotation();
	OutTracks[BoneNames[CalfNameId]].ScaleKeys[FrameIndex] = (FVector3f)RelCalfTr.GetScale3D();

	OutTracks[BoneNames[FootNameId]].PosKeys[FrameIndex] = (FVector3f)RelFootTr.GetTranslation();
	OutTracks[BoneNames[FootNameId]].RotKeys[FrameIndex] = (FQuat4f)RelFootTr.GetRotation();
	OutTracks[BoneNames[FootNameId]].ScaleKeys[FrameIndex] = (FVector3f)RelFootTr.GetScale3D();
}

#undef __rotator_direction