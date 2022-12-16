// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

class FFreeAnimHelpersEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void ResetRootScale(TArray<FAssetData> SelectedAssets);

protected:
	TSharedPtr<FUICommandList> CommandList;

	FDelegateHandle ContentBrowserMenuExtenderHandle;
};
