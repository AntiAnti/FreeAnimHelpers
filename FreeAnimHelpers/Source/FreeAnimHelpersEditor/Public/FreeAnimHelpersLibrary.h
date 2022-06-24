// (c) Yuri N. K. 2022. All rights reserved.
// ykasczc@gmail.com

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FreeAnimHelpersLibrary.generated.h"

class UAnimSequence;
class USkeletalMesh;

/**
 * Global functions for Editor module
 */
UCLASS()
class FREEANIMHELPERSEDITOR_API UFreeAnimHelpersLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static FTransform GetBoneRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& BoneName);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static FTransform GetSocketRefPositionInComponentSpace(const UAnimSequence* AnimationSequence, const FName& SocketName);

	static FTransform GetBoneRefPositionInComponentSpaceByIndex(const UAnimSequence* AnimationSequence, int32 BoneIndex);

	UFUNCTION(BlueprintCallable, Category = "FreeAnimHelpersLibrary")
	static FTransform GetBonePositionAtTimeInComponentSpace(const UAnimSequence* AnimationSequence, const FName& BoneName, float Time);
};
