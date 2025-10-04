// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "MirrorAnimation.generated.h"

/** Axes to calculate the distance value from */
UENUM(BlueprintType)
enum class EFAHRegularAxis : uint8
{
	None UMETA(Hidden),
	X,
	Y,
	Z
};


/**
 * Modify fingers rotation by adding specified rotator
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UMirrorAnimation : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UMirrorAnimation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	EFAHRegularAxis MirrorAxis = EFAHRegularAxis::X;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:

	static FRotator AddLocalRotation(const FRotator& AdditionRot, const FRotator& BaseRot);
};
