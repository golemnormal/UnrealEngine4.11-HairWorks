// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Asset editor common commands */
class FAssetEditorCommonCommands : public TCommands< FAssetEditorCommonCommands >
{

public:

	FAssetEditorCommonCommands()
		: TCommands< FAssetEditorCommonCommands >( TEXT("AssetEditor"), NSLOCTEXT("Contexts", "AssetEditor", "Asset Editor"), TEXT("EditorViewport"), FEditorStyle::GetStyleSetName() )
	{
	}	

	virtual void RegisterCommands() override;

	TSharedPtr< FUICommandInfo > SaveAsset;
	TSharedPtr< FUICommandInfo > ReimportAsset;
	TSharedPtr< FUICommandInfo > SwitchToStandaloneEditor;
	TSharedPtr< FUICommandInfo > SwitchToWorldCentricEditor;
};

