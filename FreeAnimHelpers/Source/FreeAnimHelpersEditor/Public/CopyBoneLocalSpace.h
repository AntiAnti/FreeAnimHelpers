// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "AnimationModifier.h"
#include "CopyBoneLocalSpace.generated.h"

/** Point at float curve (Time, Value) */
USTRUCT(BlueprintType)
struct FBoneCopyWrapper
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Copy Settings")
	FName ChainEndBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Copy Settings")
	int32 ChainLength = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Copy Settings")
	bool bCopyTranslation = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bone Copy Settings")
	bool bCopyRotation = true;

	FBoneCopyWrapper() {}
	FBoneCopyWrapper(const FName& Bone, int32 Length)
		: ChainEndBoneName(Bone), ChainLength(Length)
	{}
};

USTRUCT()
struct FBoneCopyRuntimeData
{
	GENERATED_BODY()

	//FTransform LocalPosition;
	bool bCopyTranslation = false;
	bool bCopyRotation = true;

	FBoneCopyRuntimeData() {};
	FBoneCopyRuntimeData(const FBoneCopyWrapper& SrcData)
		: bCopyTranslation(SrcData.bCopyTranslation), bCopyRotation(SrcData.bCopyRotation)
	{}
};

/**
 * Modify fingers rotation by adding specified rotator
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UCopyBoneLocalSpace : public UAnimationModifier
{
	GENERATED_BODY()
	
public:
	UCopyBoneLocalSpace();

	UPROPERTY(EditAnywhere, Category = "Setup")
	class UAnimSequence* SourceSequence = nullptr;

	UPROPERTY(EditAnywhere, Category = "Setup")
	bool bLoopSourceData = true;

	UPROPERTY(EditAnywhere, Category = "Setup")
	TArray<FBoneCopyWrapper> Bones;

	/* UAnimationModifier overrides */
	virtual void OnApply_Implementation(UAnimSequence* AnimationSequence) override;
	/* UAnimationModifier overrides end */
};
