// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "LockFootAtGround.h"
#include "FreeAnimHelpersLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "KismetAnimationLibrary.h"
#include "Animation/AnimData/AnimDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Engine/SkeletalMeshSocket.h"

#define __rotator_direction(Rotator, Axis) FRotationMatrix(Rotator).GetScaledAxis(Axis)

USnapFootToGround::USnapFootToGround()
	: FootBoneName_Right(TEXT("foot_r"))
	, FootTipSocket_Right(TEXT("foot_tip_r"))
	, FootBoneName_Left(TEXT("foot_l"))
	, FootTipSocket_Left(TEXT("foot_tip_l"))
	, bSnapFootRotation(false)
	, GroundLevel(0.f)
{
}

void USnapFootToGround::OnApply_Implementation(UAnimSequence* AnimationSequence)
{
	LegIK(AnimationSequence, FootBoneName_Right, FootTipSocket_Right);
	LegIK(AnimationSequence, FootBoneName_Left, FootTipSocket_Left);
}

void USnapFootToGround::OnRevert_Implementation(UAnimSequence* AnimationSequence)
{
	Super::OnRevert_Implementation(AnimationSequence);
}

void USnapFootToGround::LegIK(UAnimSequence* AnimationSequence, const FName& FootBoneName, const FName& FootTipName) const
{
	const USkeletalMeshSocket* Socket = AnimationSequence->GetSkeleton()->FindSocket(FootTipName);
	if (!Socket)
	{
		FString s = "Error: invalid socket (" + FootTipName.ToString() + ").";
		FMessageDialog::Open(EAppMsgType::Type::Ok, FText::FromString(s));
		return;
	}

	const FTransform TipOffsetTr = FTransform(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale);
	const FTransform FootBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, FootBoneName);
	FVector v = FootBoneRefTr.GetTranslation();
	const FTransform FootBoneGroundRefTr = FTransform(FootBoneRefTr.GetRotation(), FVector(v.X, v.Y, (TipOffsetTr * FootBoneRefTr).GetTranslation().Z), FootBoneRefTr.GetScale3D());
	const FTransform HeelOffsetTr = FootBoneGroundRefTr.GetRelativeTransform(FootBoneRefTr);

	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetSkeleton()->GetReferenceSkeleton();

	// names of leg bones (foot, calf, thigh)
	TArray<FName> UpdateBoneNames;
	UpdateBoneNames.SetNum(3);
	TMap<FName, int32> UpdateBoneIds;
	// animation tracks (foot, calf, thigh)
	TMap<FName, FRawAnimSequenceTrack> OutTracks;

	const int32 FootId = 0;
	const int32 CalfId = 1;
	const int32 ThighId = 2;

	// foot
	int32 PrevIndex = RefSkeleton.FindBoneIndex(FootBoneName);
	UpdateBoneNames[FootId] = FootBoneName;
	UpdateBoneIds.Add(FootBoneName, PrevIndex);
	// calf
	PrevIndex = RefSkeleton.GetParentIndex(PrevIndex);
	UpdateBoneNames[CalfId] = RefSkeleton.GetBoneName(PrevIndex);
	UpdateBoneIds.Add(UpdateBoneNames[CalfId], PrevIndex);
	// thigh
	PrevIndex = RefSkeleton.GetParentIndex(PrevIndex);
	UpdateBoneNames[ThighId] = RefSkeleton.GetBoneName(PrevIndex);
	UpdateBoneIds.Add(UpdateBoneNames[ThighId], PrevIndex);
	const FName ThighParentBoneName = RefSkeleton.GetBoneName(RefSkeleton.GetParentIndex(PrevIndex));

	const FTransform CalfBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, UpdateBoneNames[CalfId]);
	const FTransform ThighBoneRefTr = UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(AnimationSequence, UpdateBoneNames[ThighId]);

	// Orientation transformers

	const FTransform TipSocketRefTr = UFreeAnimHelpersLibrary::GetSocketRefPositionInComponentSpace(AnimationSequence, FootTipName);
	FVector ForwardDirection = TipSocketRefTr.GetTranslation() - FootBoneRefTr.GetTranslation();

	if (FMath::Abs(ForwardDirection.X) > FMath::Abs(ForwardDirection.Y))
	{
		ForwardDirection.Y = ForwardDirection.Z = 0.f; ForwardDirection.Normalize();
	}
	else
	{
		ForwardDirection.X = ForwardDirection.Z = 0.f; ForwardDirection.Normalize();
	}
	
	// a. knee target
	FVector JointTargetLoc = ForwardDirection * 5.f + CalfBoneRefTr.GetTranslation();
	const FTransform JointTargetOffset = FTransform(JointTargetLoc).GetRelativeTransform(CalfBoneRefTr);

	// b. get right-direction bone
	float ForwMul, DownMul;
	EAxis::Type ForwAxis = UFreeAnimHelpersLibrary::FindCoDirection(ThighBoneRefTr.Rotator(), ForwardDirection, ForwMul);
	EAxis::Type DownAxis = UFreeAnimHelpersLibrary::FindCoDirection(ThighBoneRefTr.Rotator(), (CalfBoneRefTr.GetTranslation() - ThighBoneRefTr.GetTranslation()), DownMul);
	EAxis::Type RightAxis = EAxis::Type::Z;
	/**/ if (ForwAxis != EAxis::Type::X && DownAxis != EAxis::Type::X) RightAxis = EAxis::Type::X;
	else if (ForwAxis != EAxis::Type::Y && DownAxis != EAxis::Type::Y) RightAxis = EAxis::Type::Y;

	// c. thigh orientation converter
	FRotator tmpRot = UKismetMathLibrary::MakeRotFromXY(CalfBoneRefTr.GetTranslation() - ThighBoneRefTr.GetTranslation(), __rotator_direction(ThighBoneRefTr.Rotator(), RightAxis));
	FTransform ThighOrientationConverter = ThighBoneRefTr.GetRelativeTransform(FTransform(tmpRot, ThighBoneRefTr.GetTranslation()));

	// d. calf orientation converter
	tmpRot = UKismetMathLibrary::MakeRotFromXY(FootBoneRefTr.GetTranslation() - CalfBoneRefTr.GetTranslation(), __rotator_direction(CalfBoneRefTr.Rotator(), RightAxis));
	FTransform CalfOrientationConverter = CalfBoneRefTr.GetRelativeTransform(FTransform(tmpRot, CalfBoneRefTr.GetTranslation()));

	// e. also need foot orientation
	EAxis::Type FootForwAxis;
	FTransform FootOrientationConverter;
	if (bSnapFootRotation)
	{
		float FootForwMul;
		FVector fd = (TipSocketRefTr.GetTranslation() - FootBoneRefTr.GetTranslation()).GetSafeNormal2D();
		FootForwAxis = UFreeAnimHelpersLibrary::FindCoDirection(FootBoneRefTr.Rotator(), fd, FootForwMul);

		FVector FootForward = __rotator_direction(FootBoneRefTr.Rotator(), FootForwAxis).GetSafeNormal2D();
		tmpRot = UKismetMathLibrary::MakeRotFromXZ(FootForward, CalfBoneRefTr.GetTranslation() - FootBoneRefTr.GetTranslation());
		FootOrientationConverter = FootBoneRefTr.GetRelativeTransform(FTransform(tmpRot, FootBoneRefTr.GetTranslation()));
	}

	// Init output data

	const int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();

	for (const auto& Bone : UpdateBoneNames)
	{
		OutTracks.Add(Bone);
		OutTracks[Bone].PosKeys.SetNumUninitialized(KeysNum);
		OutTracks[Bone].RotKeys.SetNumUninitialized(KeysNum);
		OutTracks[Bone].ScaleKeys.SetNumUninitialized(KeysNum);
	}

	// To read frame transforms
	TArray<FTransform> UpdateBonePoses;
	UpdateBonePoses.SetNumUninitialized(UpdateBoneNames.Num());

	// Update animation
	for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
	{
		float Time;
		UAnimationBlueprintLibrary::GetTimeAtFrame(AnimationSequence, FrameIndex, Time);

		UAnimationBlueprintLibrary::GetBonePosesForFrame(AnimationSequence, UpdateBoneNames, FrameIndex, false, UpdateBonePoses);
		for (int32 i = 0; i < 3; i++)
		{
			FixBoneRelativeTransform(AnimationSequence->GetSkeleton(), UpdateBoneIds[UpdateBoneNames[i]], UpdateBonePoses[i]);
		}
		const FTransform FrameThighParentTr = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, ThighParentBoneName, Time);
		const FTransform FrameThighTr = UpdateBonePoses[ThighId] * FrameThighParentTr;
		const FTransform FrameCalfTr = UpdateBonePoses[CalfId] * FrameThighTr;
		FTransform FrameFootTr = UpdateBonePoses[FootId] * FrameCalfTr;

		float FootZ;
		FRotator FootSnappedRot;
		if (bSnapFootRotation)
		{
			FVector ForwFootVec = __rotator_direction(FrameFootTr.Rotator(), FootForwAxis).GetSafeNormal2D();
			FootSnappedRot = UKismetMathLibrary::MakeRotFromXZ(ForwFootVec, FVector::UpVector);

			FrameFootTr = FootOrientationConverter * FTransform(FootSnappedRot, FrameFootTr.GetTranslation(), FrameFootTr.GetScale3D());
			FootZ = (HeelOffsetTr * FrameFootTr).GetTranslation().Z;
		}
		else
		{
			float TipZ = (TipOffsetTr * FrameFootTr).GetTranslation().Z;
			float HeelZ = (HeelOffsetTr * FrameFootTr).GetTranslation().Z;
			FootZ = FMath::Min(TipZ, HeelZ);
		}

		if (FootZ > GroundLevel)
		{
			// IK Target with modified Z coordinate
			FTransform FootTargetIK = FrameFootTr;
			FootTargetIK.AddToTranslation(FVector(0.f, 0.f, GroundLevel - FootZ));

			//  compute Two Bone IK
			FVector OutCalfLocation, OutFootLocation;
			UKismetAnimationLibrary::K2_TwoBoneIK(
				FrameThighTr.GetTranslation(),
				FrameCalfTr.GetTranslation(),
				FrameFootTr.GetTranslation(),
				(JointTargetOffset * FrameCalfTr).GetTranslation(),
				FootTargetIK.GetTranslation(),
				OutCalfLocation,
				OutFootLocation,
				false, 0.5f, 1.f);

			// Calc generic rotation
			FRotator ThighRotatorBase = UKismetMathLibrary::MakeRotFromXY(OutCalfLocation - FrameThighTr.GetTranslation(), __rotator_direction(FrameThighTr.Rotator(), RightAxis));
			FRotator CalfRotatorBase = UKismetMathLibrary::MakeRotFromXY(OutFootLocation - OutCalfLocation, __rotator_direction(FrameCalfTr.Rotator(), RightAxis));

			// Build adjusted component-space positions of leg bones
			FTransform FinalFrameThighTr = FrameThighTr; FinalFrameThighTr.SetRotation((ThighOrientationConverter * FTransform(ThighRotatorBase)).GetRotation());
			FTransform FinalFrameCalfTr = FTransform((CalfOrientationConverter * FTransform(CalfRotatorBase)).GetRotation(), OutCalfLocation, FrameCalfTr.GetScale3D());
			FTransform FinalFrameFootTr = FrameFootTr; FinalFrameFootTr.SetTranslation(OutFootLocation);

			// Calc relative transforms of the leg bones
			FTransform RelThighTr = FinalFrameThighTr.GetRelativeTransform(FrameThighParentTr);
			FTransform RelCalfTr = FinalFrameCalfTr.GetRelativeTransform(FinalFrameThighTr);
			FTransform RelFootTr = FinalFrameFootTr.GetRelativeTransform(FinalFrameCalfTr);

			// Apply rotations to tracks

			OutTracks[UpdateBoneNames[ThighId]].PosKeys[FrameIndex] = (FVector3f)UpdateBonePoses[ThighId].GetTranslation();
			OutTracks[UpdateBoneNames[ThighId]].RotKeys[FrameIndex] = (FQuat4f)RelThighTr.GetRotation();
			OutTracks[UpdateBoneNames[ThighId]].ScaleKeys[FrameIndex] = (FVector3f)UpdateBonePoses[ThighId].GetScale3D();
			
			OutTracks[UpdateBoneNames[CalfId]].PosKeys[FrameIndex] = (FVector3f)UpdateBonePoses[CalfId].GetTranslation();
			OutTracks[UpdateBoneNames[CalfId]].RotKeys[FrameIndex] = (FQuat4f)RelCalfTr.GetRotation();
			OutTracks[UpdateBoneNames[CalfId]].ScaleKeys[FrameIndex] = (FVector3f)UpdateBonePoses[CalfId].GetScale3D();

			OutTracks[UpdateBoneNames[FootId]].PosKeys[FrameIndex] = (FVector3f)UpdateBonePoses[FootId].GetTranslation();
			OutTracks[UpdateBoneNames[FootId]].RotKeys[FrameIndex] = (FQuat4f)RelFootTr.GetRotation();
			OutTracks[UpdateBoneNames[FootId]].ScaleKeys[FrameIndex] = (FVector3f)UpdateBonePoses[FootId].GetScale3D();
		}
		else
		{
			// don't modify
			int32 Index = 0;
			for (const auto& Bone : UpdateBoneNames)
			{
				OutTracks[Bone].PosKeys[FrameIndex] = (FVector3f)UpdateBonePoses[Index].GetTranslation();
				OutTracks[Bone].RotKeys[FrameIndex] = (FQuat4f)UpdateBonePoses[Index].GetRotation();
				OutTracks[Bone].ScaleKeys[FrameIndex] = (FVector3f)UpdateBonePoses[Index].GetScale3D();
				Index++;
			}
		}
	}


	// Save new keys in DataModel
	IAnimationDataController& Controller = AnimationSequence->GetController();
	for (const auto& Bone : UpdateBoneNames)
	{
		Controller.RemoveBoneTrack(Bone);
		Controller.AddBoneTrack(Bone);
		Controller.SetBoneTrackKeys(Bone, OutTracks[Bone].PosKeys, OutTracks[Bone].RotKeys, OutTracks[Bone].ScaleKeys);
	}
}

void USnapFootToGround::FixBoneRelativeTransform(USkeleton* Skeleton, int32 BoneIndex, FTransform& InOutBone) const
{
	const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	EBoneTranslationRetargetingMode::Type RetargetType = Skeleton->GetBoneTranslationRetargetingMode(BoneIndex);
	if (RetargetType == EBoneTranslationRetargetingMode::Type::Skeleton)
	{
		InOutBone.SetTranslationAndScale3D(RefPoseSpaceBaseTMs[BoneIndex].GetTranslation(), RefPoseSpaceBaseTMs[BoneIndex].GetScale3D());
	}
}

#undef __rotator_direction