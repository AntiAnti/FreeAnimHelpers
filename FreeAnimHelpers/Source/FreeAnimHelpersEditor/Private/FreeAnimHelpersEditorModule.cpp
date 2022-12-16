// Copyright Epic Games, Inc. All Rights Reserved.

#include "FreeAnimHelpersEditorModule.h"
#include "ContentBrowserModule.h"
#include "ScopedTransaction.h"
#include "Framework/Commands/UICommandInfo.h"
#include "Framework/Commands/UICommandList.h"
#include "FreeAnimHelpersLibrary.h"

static const FName FreeAnimHelpersTabName("FreeAnimHelpers");

#define LOCTEXT_NAMESPACE "FFreeAnimHelpersModule"

void FFreeAnimHelpersEditorModule::StartupModule()
{
	CommandList = MakeShareable(new FUICommandList);

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(
		FContentBrowserMenuExtender_SelectedAssets::CreateLambda([this](const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		if (SelectedAssets.ContainsByPredicate([](const FAssetData& AssetData) { return AssetData.GetClass() == USkeletalMesh::StaticClass(); }))
		{
			// Create UYnnkVoiceLipsyncData
			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				CommandList,
				FMenuExtensionDelegate::CreateLambda([this, SelectedAssets](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("ResetRootScale", "Reset Root Bone Scale"),
					LOCTEXT("ResetRootScaleToolTip", "Reset scale of root bone, but preserve size of the model"),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateRaw(this, &FFreeAnimHelpersEditorModule::ResetRootScale, SelectedAssets)));
			}));
		}

		return Extender;
	}));
	ContentBrowserMenuExtenderHandle = ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Last().GetHandle();
}

void FFreeAnimHelpersEditorModule::ShutdownModule()
{
	FContentBrowserModule* ContentBrowserModule = FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));
	if (ContentBrowserModule && ContentBrowserMenuExtenderHandle.IsValid())
	{
		ContentBrowserModule->GetAllAssetViewContextMenuExtenders().RemoveAll(
			[=](const FContentBrowserMenuExtender_SelectedAssets& InDelegate)
			{
				return ContentBrowserMenuExtenderHandle == InDelegate.GetHandle();
			}
		);
	}
}

void FFreeAnimHelpersEditorModule::ResetRootScale(TArray<FAssetData> SelectedAssets)
{
	for (auto& Asset : SelectedAssets)
	{
		if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Asset.GetAsset()))
		{
			{
				const FScopedTransaction Transaction(LOCTEXT("ResetRootScale", "Reset root bone scale"));
				Mesh->Modify();
				Mesh->GetSkeleton()->Modify();
				UFreeAnimHelpersLibrary::ResetSkinndeAssetRootBoneScale(Mesh);
			}			
		}
	}
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFreeAnimHelpersEditorModule, FreeAnimHelpersEditor)