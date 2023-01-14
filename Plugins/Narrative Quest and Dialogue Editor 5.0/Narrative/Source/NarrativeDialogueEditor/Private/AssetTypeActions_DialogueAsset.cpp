// Copyright Narrative Tools 2022. 


#include "AssetTypeActions_DialogueAsset.h"
#include "DialogueGraphEditor.h"
#include "NarrativeDialogueEditorModule.h"
#include "DialogueAsset.h"

FAssetTypeActions_DialogueAsset::FAssetTypeActions_DialogueAsset(uint32 InAssetCategory) : Category(InAssetCategory)
{

}

UClass* FAssetTypeActions_DialogueAsset::GetSupportedClass() const
{
	return UDialogueAsset::StaticClass();
}

void FAssetTypeActions_DialogueAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor /*= TSharedPtr<IToolkitHost>()*/)
{
	for (auto Object : InObjects)
	{
		if (UDialogueAsset* DialogueAsset = Cast<UDialogueAsset>(Object))
		{
			bool bFoundExisting = false;

			FDialogueGraphEditor* ExistingInstance = nullptr;

			if (UAssetEditorSubsystem* AESubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				ExistingInstance = static_cast<FDialogueGraphEditor*>(AESubsystem->FindEditorForAsset(DialogueAsset, false));
			}

			if (ExistingInstance != nullptr && ExistingInstance->GetDialogueAsset() == nullptr)
			{
				ExistingInstance->InitDialogueEditor(EToolkitMode::Standalone, EditWithinLevelEditor, DialogueAsset);
				bFoundExisting = true;
			}

			if (!bFoundExisting)
			{
				FNarrativeDialogueEditorModule& DialogueEditorModule = FModuleManager::GetModuleChecked<FNarrativeDialogueEditorModule>("NarrativeDialogueEditor");
				TSharedRef<IDialogueEditor> NewEditor = DialogueEditorModule.CreateDialogueEditor(EToolkitMode::Standalone, EditWithinLevelEditor, DialogueAsset);
			}
		}

	}
}

uint32 FAssetTypeActions_DialogueAsset::GetCategories()
{
	return Category;
}
