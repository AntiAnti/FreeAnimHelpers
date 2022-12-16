// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#include "FreeAnimHelpersLibrary.h"
#include "AnimationBlueprintLibrary.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "ReferenceSkeleton.h"

#include "Serialization/Archive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/BufferArchive.h"


FTransform UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& BoneName)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetPreviewMesh()
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	return GetBoneRefPositionInComponentSpaceByIndex(AnimationSequence, RefSkeleton.FindBoneIndex(BoneName));
}

FTransform UFreeAnimHelpersLibrary::GetSocketRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& SocketName)
{
	const USkeletalMeshSocket* Socket = AnimationSequence->GetSkeleton()->FindSocket(SocketName);
	if (!Socket)
	{
		return FTransform::Identity;
	}

	FTransform BoneTr = GetBoneRefPositionInComponentSpace(AnimationSequence, Socket->BoneName);
	return FTransform(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale) * BoneTr;
}

FTransform UFreeAnimHelpersLibrary::GetBoneRefPositionInComponentSpaceByIndex(const UAnimSequence* AnimationSequence, int32 BoneIndex)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetPreviewMesh()
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 TransformIndex = BoneIndex;
	FTransform tr_bone = FTransform::Identity;

	// stop on root
	while (TransformIndex != INDEX_NONE)
	{
		tr_bone *= RefPoseSpaceBaseTMs[TransformIndex];
		TransformIndex = RefSkeleton.GetParentIndex(TransformIndex);
	}
	tr_bone.NormalizeRotation();

	return tr_bone;
}

FTransform UFreeAnimHelpersLibrary::GetRefSkeletonBonePositionByIndex(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex)
{
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 TransformIndex = BoneIndex;
	FTransform tr_bone = FTransform::Identity;

	// stop on root
	while (TransformIndex != INDEX_NONE)
	{
		tr_bone *= RefPoseSpaceBaseTMs[TransformIndex];
		TransformIndex = RefSkeleton.GetParentIndex(TransformIndex);
	}
	tr_bone.NormalizeRotation();
	return tr_bone;
}

FTransform UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time)
{
	return GetBonePositionAtTimeInCS_ToParent(AnimationSequence, BoneName, Time, FTransform(), INDEX_NONE);
}

void UFreeAnimHelpersLibrary::AddFloatCurveKey(UCurveFloat* Curve, float Time, float Value, bool bInterpCubic)
{
	if (IsValid(Curve))
	{
		FKeyHandle h = Curve->FloatCurve.UpdateOrAddKey(Time, Value);
		if (bInterpCubic)
		{
			Curve->FloatCurve.SetKeyInterpMode(h, ERichCurveInterpMode::RCIM_Cubic);
			Curve->FloatCurve.SetKeyTangentMode(h, ERichCurveTangentMode::RCTM_Auto);
		}
		else
		{
			Curve->FloatCurve.SetKeyInterpMode(h, ERichCurveInterpMode::RCIM_Linear);
			Curve->FloatCurve.SetKeyTangentMode(h, ERichCurveTangentMode::RCTM_Auto);
		}
	}
}

void UFreeAnimHelpersLibrary::AddVectorCurveKey(UCurveVector* Curve, float Time, FVector Value, bool bInterpCubic)
{
	if (IsValid(Curve))
	{
		FKeyHandle hX = Curve->FloatCurves[0].UpdateOrAddKey(Time, Value.X);
		FKeyHandle hY = Curve->FloatCurves[1].UpdateOrAddKey(Time, Value.Y);
		FKeyHandle hZ = Curve->FloatCurves[2].UpdateOrAddKey(Time, Value.Z);
		if (bInterpCubic)
		{
			Curve->FloatCurves[0].SetKeyInterpMode(hX, ERichCurveInterpMode::RCIM_Cubic);
			Curve->FloatCurves[0].SetKeyTangentMode(hX, ERichCurveTangentMode::RCTM_Auto);
			Curve->FloatCurves[1].SetKeyInterpMode(hY, ERichCurveInterpMode::RCIM_Cubic);
			Curve->FloatCurves[1].SetKeyTangentMode(hY, ERichCurveTangentMode::RCTM_Auto);
			Curve->FloatCurves[2].SetKeyInterpMode(hZ, ERichCurveInterpMode::RCIM_Cubic);
			Curve->FloatCurves[2].SetKeyTangentMode(hZ, ERichCurveTangentMode::RCTM_Auto);
		}
		else
		{
			Curve->FloatCurves[0].SetKeyInterpMode(hX, ERichCurveInterpMode::RCIM_Linear);
			Curve->FloatCurves[0].SetKeyTangentMode(hX, ERichCurveTangentMode::RCTM_Auto);
			Curve->FloatCurves[1].SetKeyInterpMode(hY, ERichCurveInterpMode::RCIM_Linear);
			Curve->FloatCurves[1].SetKeyTangentMode(hY, ERichCurveTangentMode::RCTM_Auto);
			Curve->FloatCurves[2].SetKeyInterpMode(hZ, ERichCurveInterpMode::RCIM_Linear);
			Curve->FloatCurves[2].SetKeyTangentMode(hZ, ERichCurveTangentMode::RCTM_Auto);
		}
	}
}

void UFreeAnimHelpersLibrary::ClearFloatCurve(UCurveFloat* Curve)
{
	if (IsValid(Curve))
	{
		Curve->FloatCurve.Reset();
	}
}

void UFreeAnimHelpersLibrary::ClearVectorCurve(UCurveVector* Curve)
{
	if (IsValid(Curve))
	{
		Curve->FloatCurves[0].Reset();
		Curve->FloatCurves[1].Reset();
		Curve->FloatCurves[2].Reset();
	}
}

FTransform UFreeAnimHelpersLibrary::GetBonePositionAtTimeInCS_ToParent(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time, const FTransform& ParentBonePos, const int32 ParentBoneIndex)
{
	const FReferenceSkeleton& RefSkeleton = AnimationSequence->GetPreviewMesh()
		? AnimationSequence->GetPreviewMesh()->GetRefSkeleton()
		: AnimationSequence->GetSkeleton()->GetReferenceSkeleton();
	USkeleton* Skeleton = AnimationSequence->GetSkeleton();
	const TArray<FTransform>& RefPoseSpaceBaseTMs = RefSkeleton.GetRefBonePose();

	int32 TransformIndex = RefSkeleton.FindBoneIndex(BoneName);
	FTransform tr_bone = FTransform::Identity;

	// @TODO: optimize by using GetBonePosesForTime and set of bones
	while (TransformIndex != INDEX_NONE && TransformIndex != ParentBoneIndex)
	{
		FTransform ParentBoneTr;
		UAnimationBlueprintLibrary::GetBonePoseForTime(AnimationSequence, RefSkeleton.GetBoneName(TransformIndex), Time, false, ParentBoneTr);

		EBoneTranslationRetargetingMode::Type RetargetType = Skeleton->GetBoneTranslationRetargetingMode(TransformIndex);
		if (RetargetType == EBoneTranslationRetargetingMode::Type::Skeleton)
		{
			ParentBoneTr.SetTranslationAndScale3D(RefPoseSpaceBaseTMs[TransformIndex].GetTranslation(), RefPoseSpaceBaseTMs[TransformIndex].GetScale3D());
		}

		tr_bone *= ParentBoneTr;
		TransformIndex = RefSkeleton.GetParentIndex(TransformIndex);
	}

	if (ParentBoneIndex != INDEX_NONE && TransformIndex == ParentBoneIndex)
	{
		tr_bone *= ParentBonePos;
	}

	tr_bone.NormalizeRotation();

	return tr_bone;
}

/* Find axis of rotator the closest to being parallel to the specified vectors. Returns +1.f in Multiplier if co-directed and -1.f otherwise */
EAxis::Type UFreeAnimHelpersLibrary::FindCoDirection(const FRotator& BoneRotator, const FVector& Direction, float& ResultMultiplier)
{
	EAxis::Type RetAxis;
	FVector dir = Direction;
	dir.Normalize();

	const FVector v1 = FRotationMatrix(BoneRotator).GetScaledAxis(EAxis::X);
	const FVector v2 = FRotationMatrix(BoneRotator).GetScaledAxis(EAxis::Y);
	const FVector v3 = FRotationMatrix(BoneRotator).GetScaledAxis(EAxis::Z);

	const float dp1 = FVector::DotProduct(v1, dir);
	const float dp2 = FVector::DotProduct(v2, dir);
	const float dp3 = FVector::DotProduct(v3, dir);

	if (FMath::Abs(dp1) > FMath::Abs(dp2) && FMath::Abs(dp1) > FMath::Abs(dp3)) {
		RetAxis = EAxis::X;
		ResultMultiplier = dp1 > 0.f ? 1.f : -1.f;
	}
	else if (FMath::Abs(dp2) > FMath::Abs(dp1) && FMath::Abs(dp2) > FMath::Abs(dp3)) {
		RetAxis = EAxis::Y;
		ResultMultiplier = dp2 > 0.f ? 1.f : -1.f;
	}
	else {
		RetAxis = EAxis::Z;
		ResultMultiplier = dp3 > 0.f ? 1.f : -1.f;
	}

	return RetAxis;
}

FTransform UFreeAnimHelpersLibrary::GetSocketPositionAtTimeInCS(const UAnimSequence* AnimationSequence, const FName& SocketName, float Time)
{
	const USkeletalMeshSocket* Socket = AnimationSequence->GetSkeleton()->FindSocket(SocketName);
	if (Socket)
	{
		FTransform LocalTransform = FTransform(Socket->RelativeRotation, Socket->RelativeLocation, Socket->RelativeScale);
		return LocalTransform * GetBonePositionAtTimeInCS(AnimationSequence, Socket->BoneName, Time);
	}
	else if (AnimationSequence->GetSkeleton()->GetReferenceSkeleton().FindBoneIndex(SocketName) != INDEX_NONE)
	{
		return GetBonePositionAtTimeInCS(AnimationSequence, SocketName, Time);
	}
	else
	{
		return FTransform::Identity;
	}
}

void UFreeAnimHelpersLibrary::ResetSkinndeAssetRootBoneScale(USkeletalMesh* SkeletalMesh)
{
	if (!IsValid(SkeletalMesh))
	{
		return;
	}
	USkeleton* Skeleton = SkeletalMesh->GetSkeleton();
	FReferenceSkeleton& MeshRefSkeleton = SkeletalMesh->GetRefSkeleton();
	const TArray<FTransform>& MeshRefPose = MeshRefSkeleton.GetRefBonePose();
	if (MeshRefPose.IsEmpty())
	{
		return;
	}

	// Update reference skeleton
	{
		FTransform RootBoneTr = MeshRefPose[0];
		float ApplyScale = (RootBoneTr.GetScale3D().X + RootBoneTr.GetScale3D().Y + RootBoneTr.GetScale3D().Z) / 3.f;
		RootBoneTr.SetScale3D(FVector::OneVector);

		FReferenceSkeletonModifier RefSkelModifier(MeshRefSkeleton, Skeleton);

		RefSkelModifier.UpdateRefPoseTransform(0, RootBoneTr);
		for (int32 BoneIndex = 1; BoneIndex < MeshRefPose.Num(); ++BoneIndex)
		{
			FTransform UnscaledBoneTr = MeshRefPose[BoneIndex];
			UnscaledBoneTr.ScaleTranslation(ApplyScale);
			RefSkelModifier.UpdateRefPoseTransform(BoneIndex, UnscaledBoneTr);
		}
	}
	// this is called actually in FReferenceSkeletonModifier destructor
	MeshRefSkeleton.RebuildRefSkeleton(Skeleton, true);

	// Update skeleton
	Skeleton->UpdateReferencePoseFromMesh(SkeletalMesh);

	// Update bounds
	FVector MeshBoxBounds;
	int32 BonesNum = MeshRefSkeleton.GetRefBonePose().Num();
	for (int32 BoneIndex = 0; BoneIndex < BonesNum; BoneIndex++)
	{
		FTransform t = GetRefSkeletonBonePositionByIndex(MeshRefSkeleton, BoneIndex);
		FVector BoneOffset = t.GetTranslation().GetAbs();
		
		MeshBoxBounds.X = FMath::Max(MeshBoxBounds.X, BoneOffset.X);
		MeshBoxBounds.Y = FMath::Max(MeshBoxBounds.Y, BoneOffset.Y);
		MeshBoxBounds.Z = FMath::Max(MeshBoxBounds.Z, BoneOffset.Z);
	}
	MeshBoxBounds.Z *= 0.5f;
		
	FBoxSphereBounds Bounds;
	Bounds.BoxExtent = MeshBoxBounds;
	Bounds.Origin = FVector(0.f, 0.f, MeshBoxBounds.Z);
	Bounds.SphereRadius = (MeshBoxBounds.X + MeshBoxBounds.Y + MeshBoxBounds.Z) / 3.f;
	SkeletalMesh->SetImportedBounds(Bounds);
	SkeletalMesh->SetPositiveBoundsExtension(FVector::ZeroVector);
	SkeletalMesh->SetNegativeBoundsExtension(FVector::ZeroVector);
	SkeletalMesh->CalculateExtendedBounds();

	FBoxSphereBounds b = SkeletalMesh->GetBounds();
	
	UE_LOG(LogTemp, Log, TEXT("New Bounds = %s | Radius = %f"), *b.BoxExtent.ToString(), (float)b.SphereRadius);

	FMessageDialog::Open(EAppMsgType::Type::Ok, NSLOCTEXT("FreeAnimHelpersLibrary", "RootBoneScaleSucceed", "Root bone was rescaled successfully. Please restart Unreal Editor."));
}

const FFloatCurve* UFreeAnimHelpersLibrary::GetFloatCurve(const UAnimSequence* AnimationSequence, const FName& CurveName, FAnimationCurveIdentifier& OutCurveId)
{
	// Get Container name
	const FName ContainerName = UAnimationBlueprintLibrary::RetrieveContainerNameForCurve(AnimationSequence, CurveName);
	if (ContainerName == NAME_None) return nullptr;
	// Read Curve ID
	const FSmartName CurveSmartName = UAnimationBlueprintLibrary::RetrieveSmartNameForCurve(AnimationSequence, CurveName, ContainerName);
	OutCurveId = FAnimationCurveIdentifier(CurveSmartName, ERawCurveTrackTypes::RCT_Float);
	// Get Curve Data
	return static_cast<const FFloatCurve*>(AnimationSequence->GetDataModel()->FindCurve(OutCurveId));
}