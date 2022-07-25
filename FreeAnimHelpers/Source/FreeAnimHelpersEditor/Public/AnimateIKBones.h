// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "AnimateIKBones.generated.h"

/**
 * Copy transform in component space from skeleton bones to MetaHuman/Mannequin IK bones
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UAnimateIKBones : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UAnimateIKBones();

	// Order matters for bones in hierarchy
	UPROPERTY(EditAnywhere, meta=(DisplayName="IK bone to FK bone"), Category = "Setup")
	TMap<FName, FName> IKtoFK;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:
};
