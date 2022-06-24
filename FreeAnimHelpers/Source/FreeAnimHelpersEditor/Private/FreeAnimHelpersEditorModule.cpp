// Copyright Epic Games, Inc. All Rights Reserved.

#include "FreeAnimHelpersEditorModule.h"

static const FName FreeAnimHelpersTabName("FreeAnimHelpers");

#define LOCTEXT_NAMESPACE "FFreeAnimHelpersModule"

void FFreeAnimHelpersEditorModule::StartupModule()
{
}

void FFreeAnimHelpersEditorModule::ShutdownModule()
{
}


#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FFreeAnimHelpersEditorModule, FreeAnimHelpersEditor)