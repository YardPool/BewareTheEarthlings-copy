// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"
#include "Widgets/SWidget.h"
#include "GraphEditor.h"
#include "DialogueAsset.h"
#include "IDialogueEditor.h"
#include "WorkFlowOrientedApp/WorkflowTabManager.h"

class FDialogueGraphEditor : public IDialogueEditor, public FEditorUndoClient, public FNotifyHook
{
	
public:

	FDialogueGraphEditor();
	virtual ~FDialogueGraphEditor();
	
	FGraphPanelSelectionSet GetSelectedNodes() const;
	virtual void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	void InitDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* InObject);

	//~ Begin IToolkit Interface
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FText GetToolkitName() const override;
	virtual FText GetToolkitToolTipText() const override;
	//~ End IToolkit Interface

	//~ Begin IDialogueEditor Interface
	virtual void FocusWindow(UObject* ObjectToFocusOn = NULL) override;
	virtual uint32 GetSelectedNodesCount() const override { return SelectedNodesCount; }
	virtual bool GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const override;
	//~ End IDialogueEditor Interface

	//~ Begin FEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	// End of FEditorUndoClient

	//~ Begin FNotifyHook Interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	// End of FNotifyHook

	void CreateCommandList();

	// Delegates for general graph editor commands
	void SelectAllNodes();
	bool CanSelectAllNodes() const;
	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;
	void DeleteSelectedDuplicatableNodes();
	void CutSelectedNodes();
	bool CanCutNodes() const;
	void CopySelectedNodes();
	bool CanCopyNodes() const;
	void PasteNodes();
	void PasteNodesHere(const FVector2D& Location);
	bool CanPasteNodes() const;
	void DuplicateNodes();
	bool CanDuplicateNodes() const;
	void CreateComment();
	bool CanCreateComment() const;

	// Delegates for custom Dialogue graph editor commands
	void ShowDialogueDetails();
	bool CanShowDialogueDetails() const;
	void OpenNarrativeTutorialsInBrowser();
	bool CanOpenNarrativeTutorialsInBrowser() const;
	void QuickAddNode();
	bool CanQuickAddNode() const;

	// Delegates
	void OnNodeDoubleClicked(class UEdGraphNode* Node);
	void OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor);
	void OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged);

	void OnAddInputPin();
	bool CanAddInputPin() const;
	void OnRemoveInputPin();
	bool CanRemoveInputPin() const;

	FText GetDialogueEditorTitle() const;

	//void SearchTree();
	//bool CanSearchTree() const;

	void JumpToNode(const UEdGraphNode* Node);

	bool IsPropertyEditable() const;
	void OnPackageSaved(const FString& PackageFileName, UObject* Outer);
	void OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent);
	//virtual void OnClassListUpdated() override;

	FGraphAppearanceInfo GetGraphAppearance() const;
	bool InEditingMode(bool bGraphIsEditable) const;

	TWeakPtr<SGraphEditor> GetFocusedGraphPtr() const;

	/** Check whether the behavior tree mode can be accessed (i.e whether we have a valid tree to edit) */
	bool CanAccessDialogueEditorMode() const;

	/**
	* Get the localized text to display for the specified mode
	* @param	InMode	The mode to display
	* @return the localized text representation of the mode
	*/
	static FText GetLocalizedMode(FName InMode);

	/** Access the toolbar builder for this editor */
	TSharedPtr<class FDialogueEditorToolbar> GetToolbarBuilder() { return ToolbarBuilder; }

	UDialogueAsset* GetDialogueAsset() const;

	/** Spawns the tab with the update graph inside */
	TSharedRef<SWidget> SpawnProperties();

	/** Spawns the search tab */
	TSharedRef<SWidget> SpawnSearch();

	/** Spawn Dialogue editor tab */
	//TSharedRef<SWidget> SpawnDialogueEditor();

	// @todo This is a hack for now until we reconcile the default toolbar with application modes [duplicated from counterpart in Blueprint Editor]
	void RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager);

	/** Restores the behavior tree graph we were editing or creates a new one if none is available */
	void RestoreDialogueGraph();

	/** Save the graph state for later editing */
	void SaveEditedObjectState();

	/** Check to see if we can create a new state node */
	bool CanCreateNewState() const;

	/** Check to see if we can create a new action node */
	bool CanCreateNewAction() const;

	/** Create the menu used to make a new state node */
	TSharedRef<SWidget> HandleCreateNewStateMenu() const;

	/** Create the menu used to make a new action node */
	TSharedRef<SWidget> HandleCreateNewActionMenu() const;

	/** Handler for when a node class is picked */
	void HandleNewNodeClassPicked(UClass* InClass) const;

protected:

	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() override;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	/** Currently focused graph */
	TWeakPtr<SGraphEditor> UpdateGraphEdPtr;

private:

	/** Create widget for graph editing */
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Add custom menu options */
	void ExtendMenu();

	/** Setup common commands */
	void BindCommonCommands();

	/** Called when the selection changes in the GraphEditor */
	//virtual void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection) override;

	TSharedPtr<FDocumentTracker> DocumentManager;
	TWeakPtr<FDocumentTabFactory> GraphEditorTabFactoryPtr;

	/**The Dialogue asset being edited*/
	UDialogueAsset* DialogueAsset;

	/** Property View */
	TSharedPtr<class IDetailsView> DetailsView;

	uint32 SelectedNodesCount;

	TSharedPtr<class FDialogueEditorToolbar> ToolbarBuilder;


	/** Handle to the registered OnPackageSave delegate */
	FDelegateHandle OnPackageSavedDelegateHandle;

public:

	static const FName DialogueEditorMode;

};
