// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_DialogueAsset : public FAssetTypeActions_Base
{

public:

	FAssetTypeActions_DialogueAsset(uint32 InAssetCategory);

	uint32 Category;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_DialogueAsset", "Dialogue Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(255, 55, 120); }
	virtual UClass* GetSupportedClass() const override;
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	virtual uint32 GetCategories() override;

};
