// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "TorsoOffset.generated.h"

/**
 * Move pelvis but, preserve feet position
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UTorsoOffset : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UTorsoOffset();

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName PelvisBoneName;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName FootBoneName_Right;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName FootBoneName_Left;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FRotator RightOrientationConvert;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FRotator LeftOrientationConvert;

	// Offset in component space
	UPROPERTY(EditAnywhere, Category = "Setup")
	FVector TorsoOffset;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:
	void LegIK(UAnimSequence* AnimationSequence, float Time,
		const FTransform& OldPelvisPos,
		const FTransform& KneeOffset,
		const FTransform& ThighOrientationConverter,
		const FTransform& CalfOrientationConverter,
		const TArray<FName>& BoneNames,
		EAxis::Type RightAxis,
		TMap<FName, FRawAnimSequenceTrack>& OutTracks,
		int32 FrameIndex) const;
};
