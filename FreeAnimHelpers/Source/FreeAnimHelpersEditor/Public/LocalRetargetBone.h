// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "LocalRetargetBone.generated.h"

/**
 * Modify fingers rotation by adding specified rotator
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API ULocalRetargetBone : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	ULocalRetargetBone();

	// Path to source JA animation
	UPROPERTY(EditAnywhere, Category = "Setup")
	FString SourceAnimPath = TEXT("/Game/SkeletalJA_54/Animations");

	UPROPERTY(EditAnywhere, Category = "Setup")
	float TranslationScale = 2.46f;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TArray<FName> BoneNames;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TArray<FName> SourceBoneNames;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */

private:

	static FRotator AddLocalRotation(const FRotator& AdditionRot, const FRotator& BaseRot);
};
