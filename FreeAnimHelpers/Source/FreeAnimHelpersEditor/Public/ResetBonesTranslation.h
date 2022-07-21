// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "ResetBonesTranslation.generated.h"

/**
 * Set real local translation of all bones to Retargeting Option specified in the Skeleton asset
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UResetBonesTranslation : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UResetBonesTranslation();

	UPROPERTY(EditAnywhere, Category = "Setup")
	class USkeletalMesh* PreviewMesh;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:
};
