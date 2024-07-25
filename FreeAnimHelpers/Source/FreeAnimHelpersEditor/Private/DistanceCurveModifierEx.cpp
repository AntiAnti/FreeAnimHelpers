// Copyright Epic Games, Inc. All Rights Reserved.

#include "DistanceCurveModifierEx.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Runtime/Launch/Resources/Version.h"
#include "FreeAnimHelpersLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimationPoseData.h"
#include "Kismet/KismetMathLibrary.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"

// TODO: This logic works decently for simple clips but it should be reworked to be more robust:
//  * It could detect pivot points by change in direction.
//  * It should also account for clips that have multiple stop/pivot points.
//  * It should handle distance traveled for the ends of looping animations.
void UDistanceCurveModifierEx::OnApply_Implementation(UAnimSequence* Animation)
{
	if (Animation == nullptr)
	{
		UE_LOG(LogAnimation, Error, TEXT("DistanceCurveModifierEx failed. Reason: Invalid Animation"));
		return;
	}

	// build faked root motion curves
	GenerateRootMotion(Animation);

	const float AnimLength = Animation->GetPlayLength();
	float SampleInterval;
	int32 NumSteps;
	float TimeOfMinSpeed;

	// Find time mark of minimum speed
	if (ReferencePoint == EFAHDMotionRefPoint::StopAtEnd)
	{ 
		TimeOfMinSpeed = AnimLength;
	}
	else if (ReferencePoint == EFAHDMotionRefPoint::BeginAtStart)
	{
		TimeOfMinSpeed = 0.f;
	}
	else
	{
		// Perform a high resolution search to find the sample point with minimum speed.
		
		TimeOfMinSpeed = 0.f;
		float MinSpeedSq = FMath::Square(StopSpeedThreshold);

		SampleInterval = 1.f / 120.f;
		NumSteps = AnimLength / SampleInterval;
		for (int32 Step = 0; Step < NumSteps; ++Step)
		{
			const float Time = Step * SampleInterval;

			const bool bAllowLooping = false;
			const FVector RootMotionTranslation = FindRootMotion(Time, SampleInterval);
				//Animation->ExtractRootMotion(Time, SampleInterval, bAllowLooping).GetTranslation();
			const float RootMotionSpeedSq = CalculateMagnitudeSq(RootMotionTranslation, Axis) / SampleInterval;

			if (RootMotionSpeedSq < MinSpeedSq)
			{
				MinSpeedSq = RootMotionSpeedSq;
				TimeOfMinSpeed = Time;
			}
		}
	}

	FName RootCurveX = FName(RootCurveName.ToString() + TEXT("_X"));
	FName RootCurveY = FName(RootCurveName.ToString() + TEXT("_Y"));
	FName RootCurveZ = FName(RootCurveName.ToString() + TEXT("_Z"));

	//
	// 1. Save raw root motion data
	// 

	if (bRootMotionToCurves)
	{
		UE_LOG(LogTemp, Log, TEXT("bRootMotionToCurves"));
		if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, RootCurveX, ERawCurveTrackTypes::RCT_Float))
		{
			UAnimationBlueprintLibrary::RemoveCurve(Animation, RootCurveX);
		}
		if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, RootCurveY, ERawCurveTrackTypes::RCT_Float))
		{
			UAnimationBlueprintLibrary::RemoveCurve(Animation, RootCurveY);
		}
		if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, RootCurveZ, ERawCurveTrackTypes::RCT_Float))
		{
			UAnimationBlueprintLibrary::RemoveCurve(Animation, RootCurveZ);
		}
		UAnimationBlueprintLibrary::AddCurve(Animation, RootCurveX, ERawCurveTrackTypes::RCT_Float, false);
		UAnimationBlueprintLibrary::AddCurve(Animation, RootCurveY, ERawCurveTrackTypes::RCT_Float, false);
		UAnimationBlueprintLibrary::AddCurve(Animation, RootCurveZ, ERawCurveTrackTypes::RCT_Float, false);

		SampleInterval = 1.f / SampleRate;
		NumSteps = FMath::CeilToInt(AnimLength / SampleInterval);
		float Time = 0.0f;
		for (int32 Step = 0; Step <= NumSteps && Time < AnimLength; ++Step)
		{
			Time = FMath::Min(Step * SampleInterval, AnimLength);
			FVector rm = RootOffset.GetValue(Time);

			UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, RootCurveX, Time, rm.X);
			UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, RootCurveY, Time, rm.Y);
			UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, RootCurveZ, Time, rm.Z);
		}

		Animation->bEnableRootMotion = true;
		Animation->RootMotionRootLock = ERootMotionRootLock::AnimFirstFrame;
	}

	if (bRootMotionToRootBone)
	{
		const int32 KeysNum = Animation->GetDataModel()->GetNumberOfKeys();
		const FReferenceSkeleton& RefSkeleton = Animation->GetSkeleton()->GetReferenceSkeleton();
		FName RootBoneName = RefSkeleton.GetBoneName(0);
		TArray<FName> AttachBoneNames;
		TArray<FTransform> AttachBonePositions;

		bool bAnimateRootBoneFromCurves = bRootMotionFromCurves && !bRootMotionToCurves;
		UE_LOG(LogTemp, Log, TEXT("Animation keys number = %d"), KeysNum);

		if (bAnimateRootBoneFromCurves)
		{
			FAnimationCurveIdentifier IdCurveX, IdCurveY, IdCurveZ;
			const FFloatCurve* CurveX = UFreeAnimHelpersLibrary::GetFloatCurve(Animation, RootCurveX, IdCurveX);
			const FFloatCurve* CurveY = UFreeAnimHelpersLibrary::GetFloatCurve(Animation, RootCurveY, IdCurveY);
			const FFloatCurve* CurveZ = UFreeAnimHelpersLibrary::GetFloatCurve(Animation, RootCurveZ, IdCurveZ);
			if (!CurveX || !CurveY)
			{
				UE_LOG(LogTemp, Log, TEXT("Invalid root motion curves. X: %d, Y: %d, Z: %d"), CurveX ? 1 : 0, CurveY ? 1 : 0, CurveZ ? 1 : 0);
				return;
			}

			for (auto& Curve : RootOffset.VectorCurves)
				Curve.Reset();

			for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
			{
				const float Time = Animation->GetTimeAtFrame(FrameIndex);
				const FVector NewPoint = FVector(
					CurveX->Evaluate(Time),
					CurveY->Evaluate(Time),
					CurveZ ? CurveZ->Evaluate(Time) : 0.f
				);
				AddVectorCurveKey(RootOffset, Time, NewPoint);
			}

			const int32 NumBones = RefSkeleton.GetNum();
			for (int32 ChildIndex = 1; ChildIndex < NumBones; ChildIndex++)
			{
				if (RefSkeleton.GetParentIndex(ChildIndex) == 0)
				{
					AttachBoneNames.Add(RefSkeleton.GetBoneName(ChildIndex));
				}
			}
			AttachBonePositions.SetNum(AttachBoneNames.Num());
		}

		FRawAnimSequenceTrack RootTrack;
		RootTrack.PosKeys.SetNumUninitialized(KeysNum);
		RootTrack.RotKeys.SetNumUninitialized(KeysNum);
		RootTrack.ScaleKeys.SetNumUninitialized(KeysNum);

		TArray<FRawAnimSequenceTrack> ChildTrachs;
		ChildTrachs.SetNum(AttachBoneNames.Num());
		for (auto& ChildTrack : ChildTrachs)
		{
			ChildTrack.PosKeys.SetNumUninitialized(KeysNum);
			ChildTrack.RotKeys.SetNumUninitialized(KeysNum);
			ChildTrack.ScaleKeys.SetNumUninitialized(KeysNum);
		}

		// Update animation
		for (int32 FrameIndex = 0; FrameIndex < KeysNum; FrameIndex++)
		{
			float Time = Animation->GetTimeAtFrame(FrameIndex);

			FTransform RootFramePose;
			UFreeAnimHelpersLibrary::GetBonePoseForTime(Animation, RootBoneName, Time, false, RootFramePose);

			const FVector NewRootLocation = RootOffset.GetValue(Time);

			if (bAnimateRootBoneFromCurves)
			{
				UFreeAnimHelpersLibrary::GetBonePosesForTime(Animation, AttachBoneNames, Time, false, AttachBonePositions, Animation->GetPreviewMesh());

				FTransform NewRootFramePose = RootFramePose;
				NewRootFramePose.SetTranslation(NewRootLocation);

				for (int32 i = 0; i < AttachBoneNames.Num(); i++)
				{
					FTransform BoneNewTr = (AttachBonePositions[i] * RootFramePose).GetRelativeTransform(NewRootFramePose);
					ChildTrachs[i].PosKeys[FrameIndex] = (FVector3f)BoneNewTr.GetTranslation();
					ChildTrachs[i].RotKeys[FrameIndex] = (FQuat4f)BoneNewTr.GetRotation();
					ChildTrachs[i].ScaleKeys[FrameIndex] = (FVector3f)BoneNewTr.GetScale3D();
				}
			}

			RootTrack.PosKeys[FrameIndex] = (FVector3f)NewRootLocation;
			RootTrack.RotKeys[FrameIndex] = (FQuat4f)RootFramePose.GetRotation();
			RootTrack.ScaleKeys[FrameIndex] = (FVector3f)RootFramePose.GetScale3D();
		}

		IAnimationDataController& Controller = Animation->GetController();
		Controller.RemoveBoneTrack(RootBoneName);
		Controller.AddBoneCurve(RootBoneName);
		Controller.SetBoneTrackKeys(RootBoneName, RootTrack.PosKeys, RootTrack.RotKeys, RootTrack.ScaleKeys);

		for (int32 i = 0; i < AttachBoneNames.Num(); i++)
		{
			Controller.RemoveBoneTrack(AttachBoneNames[i]);
			Controller.AddBoneCurve(AttachBoneNames[i]);
			Controller.SetBoneTrackKeys(AttachBoneNames[i], ChildTrachs[i].PosKeys, ChildTrachs[i].RotKeys, ChildTrachs[i].ScaleKeys);
		}

		Animation->RefreshCacheData();
	}

	//
	// 2. Save distance data
	//

	if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, CurveName, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::RemoveCurve(Animation, CurveName);
	}
	UAnimationBlueprintLibrary::AddCurve(Animation, CurveName, ERawCurveTrackTypes::RCT_Float, false);

	SampleInterval = 1.f / SampleRate;
	NumSteps = FMath::CeilToInt(AnimLength / SampleInterval);
	float Time = 0.0f;
	float DistanceRangeA = 999999.f, DistanceRangeB = -999999.f;
	for (int32 Step = 0; Step <= NumSteps && Time < AnimLength; ++Step)
	{
		Time = FMath::Min(Step * SampleInterval, AnimLength);

		// Assume that during any time before the stop/pivot point, the animation is approaching that point.
		const float ValueSign = (Time < TimeOfMinSpeed) ? -1.0f : 1.0f;

		const FVector RootMotionTranslation = FindRootMotionInRange(TimeOfMinSpeed, Time);
			//Animation->ExtractRootMotionFromRange(TimeOfMinSpeed, Time).GetTranslation();
		float Magnitude = ValueSign * CalculateMagnitude(RootMotionTranslation, Axis);
		UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, CurveName, Time, Magnitude);

		DistanceRangeA = FMath::Min(DistanceRangeA, Magnitude);
		DistanceRangeB = FMath::Max(DistanceRangeB, Magnitude);
	}

	if (bOptimizedDistanceCurveFormat)
	{
		FName OptimizedCurveName = FName(CurveName.ToString() + TEXT("_Optimized"));
		if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, OptimizedCurveName, ERawCurveTrackTypes::RCT_Float))
		{
			UAnimationBlueprintLibrary::RemoveCurve(Animation, OptimizedCurveName);
		}
		UAnimationBlueprintLibrary::AddCurve(Animation, OptimizedCurveName, ERawCurveTrackTypes::RCT_Float, false);

		// Save magnitude
		UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, OptimizedCurveName, -1.f, DistanceRangeA);
		UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, OptimizedCurveName, -0.5f, DistanceRangeB);

		int32 RootKeysNum = RootOffset.VectorCurves[0].GetNumKeys();
		for (int32 KeyIndex = 0; KeyIndex < RootKeysNum; KeyIndex++)
		{
			Time = RootOffset.VectorCurves[0].Keys[KeyIndex].Time;
			const FVector RootMotionTranslation = FindRootMotionInRange(TimeOfMinSpeed, Time);

			// Assume that during any time before the stop/pivot point, the animation is approaching that point.
			const float ValueSign = (Time < TimeOfMinSpeed) ? -1.0f : 1.0f;
			float Magnitude = ValueSign * CalculateMagnitude(RootMotionTranslation, Axis);

			float MappedMagnitude = FMath::GetMappedRangeValueClamped(FVector2D(DistanceRangeA, DistanceRangeB), FVector2D(0.f, AnimLength), Magnitude);

			UAnimationBlueprintLibrary::AddFloatCurveKey(Animation, OptimizedCurveName, MappedMagnitude, Time);
		}
	}
}

void UDistanceCurveModifierEx::OnRevert_Implementation(UAnimSequence* Animation)
{
	const bool bRemoveNameFromSkeleton = false;
	if (UAnimationBlueprintLibrary::DoesCurveExist(Animation, CurveName, ERawCurveTrackTypes::RCT_Float))
	{
		UAnimationBlueprintLibrary::RemoveCurve(Animation, CurveName, bRemoveNameFromSkeleton);
	}

	if (bRootMotionToCurves)
	{	
		/*
		UE_LOG(LogTemp, Log, TEXT("Delete root motion curves :("));
		UAnimationBlueprintLibrary::RemoveCurve(Animation, FName(RootCurveName.ToString() + TEXT("_X")), bRemoveNameFromSkeleton);
		UAnimationBlueprintLibrary::RemoveCurve(Animation, FName(RootCurveName.ToString() + TEXT("_Y")), bRemoveNameFromSkeleton);
		UAnimationBlueprintLibrary::RemoveCurve(Animation, FName(RootCurveName.ToString() + TEXT("_Z")), bRemoveNameFromSkeleton);
		*/
	}
}

float UDistanceCurveModifierEx::CalculateMagnitude(const FVector& Vector, EFAHDistanceCurve_Axis Axis)
{
	switch (Axis)
	{
		case EFAHDistanceCurve_Axis::X:		return FMath::Abs(Vector.X); break;
		case EFAHDistanceCurve_Axis::Y:		return FMath::Abs(Vector.Y); break;
		case EFAHDistanceCurve_Axis::Z:		return FMath::Abs(Vector.Z); break;
		default: return FMath::Sqrt(CalculateMagnitudeSq(Vector, Axis)); break;
	}
}

float UDistanceCurveModifierEx::CalculateMagnitudeSq(const FVector& Vector, EFAHDistanceCurve_Axis Axis)
{
	switch (Axis)
	{
		case EFAHDistanceCurve_Axis::X:		return FMath::Square(FMath::Abs(Vector.X)); break;
		case EFAHDistanceCurve_Axis::Y:		return FMath::Square(FMath::Abs(Vector.Y)); break;
		case EFAHDistanceCurve_Axis::Z:		return FMath::Square(FMath::Abs(Vector.Z)); break;
		case EFAHDistanceCurve_Axis::XY:	return Vector.X * Vector.X + Vector.Y * Vector.Y; break;
		case EFAHDistanceCurve_Axis::XZ:	return Vector.X * Vector.X + Vector.Z * Vector.Z; break;
		case EFAHDistanceCurve_Axis::YZ:	return Vector.Y * Vector.Y + Vector.Z * Vector.Z; break;
		case EFAHDistanceCurve_Axis::XYZ:	return Vector.X * Vector.X + Vector.Y * Vector.Y + Vector.Z * Vector.Z; break;
		default: check(false); break;
	}

	return 0.f;
}

FVector UDistanceCurveModifierEx::DirectionAsVector(EMATMovementDirection Direction)
{
	switch (Direction)
	{
		case EMATMovementDirection::MD_X:		return FVector(1.f, 0.f, 0.f); break;
		case EMATMovementDirection::MD_XY:		return FVector(1.f, 1.f, 0.f); break;
		case EMATMovementDirection::MD_Y:		return FVector(0.f, 1.f, 0.f); break;
		case EMATMovementDirection::MD_XnY:		return FVector(-1.f, 1.f, 0.f); break;
		case EMATMovementDirection::MD_Xn:		return FVector(-1.f, 0.f, 0.f); break;
		case EMATMovementDirection::MD_XnYn:	return FVector(-1.f, -1.f, 0.f); break;
		case EMATMovementDirection::MD_Yn:		return FVector(0.f, -1.f, 0.f); break;
		case EMATMovementDirection::MD_XYn:		return FVector(1.f, -1.f, 0.f); break;
		case EMATMovementDirection::MD_Z:		return FVector(0.f, 0.f, 1.f); break;
	}

	return FVector::RightVector;
}

void UDistanceCurveModifierEx::AddVectorCurveKey(FRuntimeVectorCurve& Curve, float Time, const FVector& Value)
{
	FKeyHandle h1 = Curve.VectorCurves[0].AddKey(Time, Value.X);
	FKeyHandle h2 = Curve.VectorCurves[1].AddKey(Time, Value.Y);
	FKeyHandle h3 = Curve.VectorCurves[2].AddKey(Time, Value.Z);
	Curve.VectorCurves[0].SetKeyInterpMode(h1, ERichCurveInterpMode::RCIM_Linear);
	Curve.VectorCurves[1].SetKeyInterpMode(h2, ERichCurveInterpMode::RCIM_Linear);
	Curve.VectorCurves[2].SetKeyInterpMode(h3, ERichCurveInterpMode::RCIM_Linear);
}

FVector UDistanceCurveModifierEx::GetVectorCurveValue(const FRuntimeVectorCurve& Curve, int32 KeyIndex)
{
	return FVector(Curve.VectorCurves[0].Keys[KeyIndex].Value, Curve.VectorCurves[1].Keys[KeyIndex].Value, Curve.VectorCurves[2].Keys[KeyIndex].Value);
}

void UDistanceCurveModifierEx::GenerateRootMotion(UAnimSequence* AnimationSequence)
{
	int32 KeysNum = AnimationSequence->GetDataModel()->GetNumberOfKeys();
	UE_LOG(LogTemp, Log, TEXT("GenerateRootMotion for %d animation keys"), KeysNum);

	FVector LastPelvis, LastFootRight, LastFootLeft;
	float LastDistanceR = 0.f, LastDistanceL = 0.f;
	bool bUseRightFoot = true;

	RootOffset.ExternalCurve = nullptr;
	AddVectorCurveKey(RootOffset, 0.f, FVector::ZeroVector);

	// find initial base foot
	FVector MovementDirection = DirectionAsVector(InitialDirection);
	if (InitialDirection != EMATMovementDirection::MD_Z)
	{
		for (int32 KeyIndex = 0; KeyIndex < KeysNum; KeyIndex++)
		{
			float Time = AnimationSequence->GetTimeAtFrame(KeyIndex);

			const FVector FootRight = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, FootRightBone, Time).GetTranslation();
			const FVector FootLeft = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, FootLeftBone, Time).GetTranslation();

			if (KeyIndex > 0)
			{
				bool bRightDP = FVector::DotProduct((FootRight - LastFootRight).GetSafeNormal2D(), MovementDirection) < 0.f;
				bool bLeftDP = FVector::DotProduct((FootLeft - LastFootLeft).GetSafeNormal2D(), MovementDirection) < 0.f;

				if (bRightDP && !bLeftDP)
				{
					bUseRightFoot = true; break;
				}
				else if (!bRightDP && bLeftDP)
				{
					bUseRightFoot = false; break;
				}
			}
			LastFootRight = FootRight; LastFootLeft = FootLeft;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("GenerateRootMotion starts with leg (1 = right, 0 = left): %d"), (int)bUseRightFoot);

	FVector LastFrameRoot = FVector::ZeroVector;
	for (int32 KeyIndex = 0; KeyIndex < KeysNum; KeyIndex++)
	{
		float Time = AnimationSequence->GetTimeAtFrame(KeyIndex);

		const FVector Pelvis = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, PelvisBone, Time).GetTranslation();
		const FVector FootRight = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, FootRightBone, Time).GetTranslation();
		const FVector FootLeft = UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(AnimationSequence, FootLeftBone, Time).GetTranslation();

		float DistanceR = FVector::Dist2D(Pelvis, FootRight),
			DistanceL = FVector::Dist2D(Pelvis, FootLeft);

		if (KeyIndex > 0)
		{
			if (InitialDirection == EMATMovementDirection::MD_Z)
			{
				LastFrameRoot = FVector(0.f, 0.f, FMath::Max(0.f, FMath::Min(FootRight.Z, FootLeft.Z)));
			}
			else
			{
				// update bearing foot
				if (bUseRightFoot)
				{
					if (DistanceR < LastDistanceR && FVector::DotProduct(MovementDirection.GetSafeNormal2D(), Pelvis - FootRight) > 0.f)
					{
						bUseRightFoot = false;
					}
				}
				else
				{
					if (DistanceL < LastDistanceL && FVector::DotProduct(MovementDirection.GetSafeNormal2D(), Pelvis - FootLeft) > 0.f)
					{
						bUseRightFoot = true;
					}
				}

				FVector MovementDirectionR = (Pelvis - FootRight) - (LastPelvis - LastFootRight);
				MovementDirectionR.Z = 0.f;
				FVector MovementDirectionL = (Pelvis - FootLeft) - (LastPelvis - LastFootLeft);
				MovementDirectionL.Z = 0.f;

				if (FVector::DotProduct(MovementDirection, MovementDirectionR) > FVector::DotProduct(MovementDirection, MovementDirectionL))
					MovementDirection = MovementDirectionR;
				else
					MovementDirection = MovementDirectionL;

				LastFrameRoot += MovementDirection;
			}

			// save key
			UE_LOG(LogTemp, Log, TEXT("[%d] Root location at time %f = %s"), (int)bUseRightFoot, Time, *LastFrameRoot.ToString());

			AddVectorCurveKey(RootOffset, Time, LastFrameRoot);
		}

		LastPelvis = Pelvis; LastFootRight = FootRight; LastFootLeft = FootLeft;
		LastDistanceR = DistanceR; LastDistanceL = DistanceL;
	}

	// update extremums
	int32 RootKeysNum = RootOffset.VectorCurves[0].Keys.Num();
	TArray< TArray<int32>> Extremums;
	Extremums.SetNum(3);
	Extremums[0].Add(0); Extremums[0].Add(1); Extremums[2].Add(0);

	const int32 CheckArea = 2;
	for (int32 i = CheckArea; i < RootKeysNum - CheckArea; i++)
	{
		FVector CurrentValue = GetVectorCurveValue(RootOffset, i);

		bool bExtremumX = true, bExtremumY = true, bExtremumZ = true;
		int32 AllSameSizeX = 0, AllSameSizeY = 0, AllSameSizeZ = 0;

		for (int32 AreaIndex = i - CheckArea; AreaIndex <= i + CheckArea; AreaIndex++)
		{
			if (AreaIndex == i) continue;
			FVector Delta = CurrentValue - GetVectorCurveValue(RootOffset, AreaIndex);

			if (AllSameSizeX == 0)
			{
				AllSameSizeX = (int32)FMath::Sign(Delta.X);
			}
			else if (AllSameSizeX != FMath::Sign(Delta.X) && Delta.X != 0.f)
			{
				bExtremumX = false;
			}
			if (AllSameSizeY == 0)
			{
				AllSameSizeY = (int32)FMath::Sign(Delta.Y);
			}
			else if (AllSameSizeY != FMath::Sign(Delta.Y) && Delta.Y != 0.f)
			{
				bExtremumY = false;
			}
			if (AllSameSizeZ == 0)
			{
				AllSameSizeZ = (int32)FMath::Sign(Delta.Z);
			}
			else if (AllSameSizeZ != FMath::Sign(Delta.Z) && Delta.Z != 0.f)
			{
				bExtremumZ = false;
			}
		}

		if (bExtremumX) Extremums[0].Add(i);
		if (bExtremumY) Extremums[1].Add(i);
		if (bExtremumZ) Extremums[2].Add(i);
		UE_LOG(LogTemp, Log, TEXT("[%f] Extremums %d %d %d"), RootOffset.VectorCurves[0].Keys[i].Time, (int)bExtremumX, (int)bExtremumY, (int)bExtremumZ);
	}
	Extremums[0].Add(RootKeysNum - 1); Extremums[0].Add(RootKeysNum - 1); Extremums[2].Add(RootKeysNum - 1);

	// cleanup intermediate extremums
	for (int32 Axis3d = 0; Axis3d < 3; Axis3d++)
	{
		TSet<int32> IndToRemove;
		for (int32 ExtrIndex = 1; ExtrIndex < Extremums[Axis3d].Num() - 1; ExtrIndex++)
		{
			FRichCurveKey Prev = RootOffset.VectorCurves[Axis3d].Keys[Extremums[Axis3d][ExtrIndex - 1]];
			FRichCurveKey Curr = RootOffset.VectorCurves[Axis3d].Keys[Extremums[Axis3d][ExtrIndex]];
			FRichCurveKey Next = RootOffset.VectorCurves[Axis3d].Keys[Extremums[Axis3d][ExtrIndex + 1]];

			if (Curr.Time - Prev.Time < SmoothenAccuracy && Next.Time - Curr.Time > SmoothenAccuracy)
			{
				if (ExtrIndex == 1)
				{
					IndToRemove.Add(Extremums[Axis3d][ExtrIndex]);
				}
				else
				{
					Prev = RootOffset.VectorCurves[Axis3d].Keys[Extremums[Axis3d][ExtrIndex - 2]];
					if (FMath::Sign(Prev.Value - Curr.Value) != FMath::Sign(Next.Value - Curr.Value))
					{
						IndToRemove.Add(Extremums[Axis3d][ExtrIndex]);
					}
				}
			}
			else if (Next.Time - Curr.Time < SmoothenAccuracy && Curr.Time - Prev.Time > SmoothenAccuracy)
			{
				if (ExtrIndex == Extremums[Axis3d].Num() - 2)
				{
					IndToRemove.Add(Extremums[Axis3d][ExtrIndex]);
				}
				else
				{
					Next = RootOffset.VectorCurves[Axis3d].Keys[Extremums[Axis3d][ExtrIndex + 2]];
					if (FMath::Sign(Prev.Value - Curr.Value) != FMath::Sign(Next.Value - Curr.Value))
					{
						IndToRemove.Add(Extremums[Axis3d][ExtrIndex]);
					}
				}
			}
		}
	}

	// smoothen root offsets with respect to extremums
	int32 Iterations = 8;
	for (int32 Iter = 0; Iter < Iterations; Iter++)
	{
		// smoothen
		for (int32 i = 1; i < RootKeysNum - 1; i++)
		{
			FVector Value = (GetVectorCurveValue(RootOffset, i) + GetVectorCurveValue(RootOffset, i - 1) + GetVectorCurveValue(RootOffset, i + 1)) / 3.f;
			if (!Extremums[0].Contains(i)) RootOffset.VectorCurves[0].Keys[i].Value = Value.X;
			if (!Extremums[1].Contains(i)) RootOffset.VectorCurves[1].Keys[i].Value = Value.Y;
			if (!Extremums[2].Contains(i)) RootOffset.VectorCurves[2].Keys[i].Value = Value.Z;
		}
	}
}

FVector UDistanceCurveModifierEx::FindRootMotion(float StartTime, float DeltaTime) const
{
	return RootOffset.GetValue(StartTime + DeltaTime) - RootOffset.GetValue(StartTime);
}

FVector UDistanceCurveModifierEx::FindRootMotionInRange(float StartTime, float EndTime) const
{
	return RootOffset.GetValue(EndTime) - RootOffset.GetValue(StartTime);
}