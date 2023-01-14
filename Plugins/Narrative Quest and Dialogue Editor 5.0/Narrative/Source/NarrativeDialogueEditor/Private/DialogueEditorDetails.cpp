// Copyright Narrative Tools 2022. 

#include "DialogueEditorDetails.h"
#include "DetailLayoutBuilder.h"
#include "QuestSM.h"
#include "Dialogue.h"
#include "DialogueAsset.h"
#include "DetailCategoryBuilder.h"

#define LOCTEXT_NAMESPACE "DialogueEditorDetails"

TSharedRef<IDetailCustomization> FDialogueEditorDetails::MakeInstance()
{
	return MakeShareable(new FDialogueEditorDetails);
}

void FDialogueEditorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	LayoutBuilder = &DetailLayout;

	TArray<TWeakObjectPtr<UObject>> EditedObjects;
	DetailLayout.GetObjectsBeingCustomized(EditedObjects);

	if (EditedObjects.Num() > 0 && EditedObjects.IsValidIndex(0))
	{

	}
}

#undef LOCTEXT_NAMESPACE