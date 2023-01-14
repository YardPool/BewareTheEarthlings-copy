// Copyright Narrative Tools 2022. 

#include "DialogueEditorModes.h"
#include "DialogueEditorTabs.h"
#include "DialogueEditorToolbar.h"
#include "DialogueEditorDetails.h"
#include "DialogueEditorTabFactories.h"

#define LOCTEXT_NAMESPACE "DialogueGraphApplicationMode"

FDialogueEditorApplicationMode::FDialogueEditorApplicationMode(TSharedPtr<class FDialogueGraphEditor> InDialogueGraphEditor)
	: FApplicationMode(FDialogueGraphEditor::DialogueEditorMode, FDialogueGraphEditor::GetLocalizedMode)
{
	DialogueGraphEditor = InDialogueGraphEditor;

	DialogueEditorTabFactories.RegisterFactory(MakeShareable(new FDialogueDetailsSummoner(InDialogueGraphEditor)));

	

	TabLayout = FTabManager::NewLayout("Standalone_DialogueGraphEditor_Layout_v1")
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.3f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.7f)
					->SetHideTabWell(true)
					->AddTab(FDialogueEditorTabs::GraphEditorID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.3f)
					->SetHideTabWell(true)
					->AddTab(FDialogueEditorTabs::GraphDetailsID, ETabState::OpenedTab)
				)
			)
	);

	InDialogueGraphEditor->GetToolbarBuilder()->AddDialogueToolbar(ToolbarExtender);
	InDialogueGraphEditor->GetToolbarBuilder()->AddModesToolbar(ToolbarExtender);

}

void FDialogueEditorApplicationMode::RegisterTabFactories(TSharedPtr<class FTabManager> InTabManager)
{
	check(DialogueGraphEditor.IsValid());
	TSharedPtr<FDialogueGraphEditor> DialogueEditorPtr = DialogueGraphEditor.Pin();

	//DialogueEditorPtr->RegisterToolbarTab(InTabManager.ToSharedRef());

	// Mode-specific setup
	DialogueEditorPtr->PushTabFactories(DialogueEditorTabFactories);

	FApplicationMode::RegisterTabFactories(InTabManager);
}

void FDialogueEditorApplicationMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();

	check(DialogueGraphEditor.IsValid());
	TSharedPtr<FDialogueGraphEditor> DialogueEditorPtr = DialogueGraphEditor.Pin();

	DialogueEditorPtr->SaveEditedObjectState();
}

void FDialogueEditorApplicationMode::PostActivateMode()
{
	// Reopen any documents that were open when the blueprint was last saved
	check(DialogueGraphEditor.IsValid());
	TSharedPtr<FDialogueGraphEditor> DialogueEditorPtr = DialogueGraphEditor.Pin();
	DialogueEditorPtr->RestoreDialogueGraph();

	FApplicationMode::PostActivateMode();
}

#undef LOCTEXT_NAMESPACE
