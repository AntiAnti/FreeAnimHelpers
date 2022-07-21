// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "PrepareTurnInPlaceAsset.generated.h"

/**
 * This modifier is for internal usage.
 * Input: raw turn in-place animation without root motion (root bone isn't animated)
 * Output: animation with extracted linear rotation and extracted movement in horizontal plane (X-Y)
 * 		   X-Y coordinates are stored to curves
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UPrepareTurnInPlaceAsset : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UPrepareTurnInPlaceAsset();

	UPROPERTY(EditAnywhere, Category = "Setup")
	FName PelvisBoneName;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bSnapZeroFrameToCenter;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bTurningToRight;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bDefaultRootBoneOffset;

	UPROPERTY(EditAnywhere, meta=(EditCondition="!bDefaultRootBoneOffset"), Category = "Setup")
	FVector2D RootBoneOffset;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:
};
