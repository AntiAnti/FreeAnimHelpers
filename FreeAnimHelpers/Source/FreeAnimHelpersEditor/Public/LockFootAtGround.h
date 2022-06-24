// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "LockFootAtGround.generated.h"

class UMATCurveToBoneRules;

/**
 * Animation modifier to bake curves animation to bones using UMATCurveToBoneRules
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API USnapFootToGround : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	USnapFootToGround();

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName FootBoneName_Right;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName FootTipSocket_Right;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName FootBoneName_Left;

	UPROPERTY(EditAnywhere, Category = "Skeleton")
	FName FootTipSocket_Left;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bSnapFootRotation;

	UPROPERTY(EditAnywhere, Category = "Setup")
	float GroundLevel;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	virtual void OnRevert_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:
	void LegIK(UAnimSequence* AnimationSequence, const FName& FootBoneName, const FName& FootTipName) const;

	/* Find axis of rotator the closest to being parallel to the specified vectors. Returns +1.f in Multiplier if co-directed and -1.f otherwise */
	EAxis::Type FindCoDirection(const FRotator& BoneRotator, const FVector& Direction, float& ResultMultiplier) const;
	void FixBoneRelativeTransform(USkeleton* Skeleton, int32 BoneIndex, FTransform& InOutBone) const;
};
