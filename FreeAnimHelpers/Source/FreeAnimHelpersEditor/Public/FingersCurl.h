// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "FingersCurl.generated.h"

/** Point at float curve (Time, Value) */
USTRUCT(BlueprintType)
struct FFingerRotationWrapper
{
	GENERATED_BODY()

	/** Key: last finger bone (for ex. index_03_r). Value: rotation addend. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Finger Rotation Addend")
	TMap<FName, FRotator> Fingers;
};

/**
 * Modify fingers rotation by adding specified rotator
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UFingersCurl : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UFingersCurl();

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bApplyRightHand = true;

	// Set of fingers for the right hand
	UPROPERTY(EditAnywhere, meta=(EditCondition="bApplyRightHand"), Category = "Setup")
	TMap<FName, FRotator> HandRight;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bApplyLeftHand = true;

	// Set of fingers for the left hand
	UPROPERTY(EditAnywhere, meta = (EditCondition = "bApplyLeftHand"), Category = "Setup")
	TMap<FName, FRotator> HandLeft;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:

	static FRotator AddLocalRotation(const FRotator& AdditionRot, const FRotator& BaseRot);
};
