// Copyright Narrative Tools 2022. 

#include "DialogueGraphEditor.h"
#include "NarrativeDialogueEditorModule.h"
#include "ScopedTransaction.h"
#include "DialogueEditorTabFactories.h"
#include "GraphEditorActions.h"
#include "Framework/Commands/GenericCommands.h"
#include "DialogueGraphNode.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "EdGraphUtilities.h"
#include "DialogueGraph.h"
#include "DialogueGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "DialogueEditorModes.h"
#include "Widgets/Docking/SDockTab.h"
#include "DialogueEditorToolbar.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "DialogueSM.h"
#include "DialogueAsset.h"
#include "DialogueGraphNode.h"
#include "Dialogue.h"
#include "Windows/WindowsPlatformApplicationMisc.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h" 
#include "DialogueEditorCommands.h"
#include "DialogueEditorSettings.h"

#define LOCTEXT_NAMESPACE "DialogueAssetEditor"

const FName FDialogueGraphEditor::DialogueEditorMode(TEXT("DialogueEditor"));

//TODO change this to the documentation page after documentation is ready for use
static const FString NarrativeHelpURL("http://www.google.com");

FDialogueGraphEditor::FDialogueGraphEditor()
{
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor != NULL)
	{
		Editor->RegisterForUndo(this);
	}
}

FDialogueGraphEditor::~FDialogueGraphEditor()
{
	UEditorEngine* Editor = (UEditorEngine*)GEngine;
	if (Editor)
	{
		Editor->UnregisterForUndo(this);
	}
}

FGraphPanelSelectionSet FDialogueGraphEditor::GetSelectedNodes() const
{
	FGraphPanelSelectionSet CurrentSelection;
	if (TSharedPtr<SGraphEditor> FocusedGraphEd = UpdateGraphEdPtr.Pin())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	return CurrentSelection;
}

void FDialogueGraphEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	SelectedNodesCount = NewSelection.Num();

	if (SelectedNodesCount == 0)
	{
		DetailsView->SetObject(DialogueAsset->Dialogue);
		return;
	}

	UDialogueGraphNode* SelectedNode = nullptr;

	for (UObject* Selection : NewSelection)
	{
		if (UDialogueGraphNode* Node = Cast<UDialogueGraphNode>(Selection))
		{
			SelectedNode = Node;
			break;
		}
	}

	if (UDialogueGraph* MyGraph = Cast<UDialogueGraph>(DialogueAsset->DialogueGraph))
	{
		if (DetailsView.IsValid())
		{
			if (SelectedNode)
			{
				//Edit the underlying graph node object 
				if (UDialogueGraphNode* Node = Cast<UDialogueGraphNode>(SelectedNode))
				{
					DetailsView->SetObject(Node->DialogueNode);
				}
			}
		}
		else
		{
			DetailsView->SetObject(nullptr);
		}
	}
}

void FDialogueGraphEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	DocumentManager->SetTabManager(InTabManager);

	//FWorkflowCentricApplication::RegisterTabSpawners(InTabManager);
}

void FDialogueGraphEditor::InitDialogueEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* InObject)
{
	if (UDialogueAsset* DialogueAssetToEdit = Cast<UDialogueAsset>(InObject))
	{
		DialogueAsset = DialogueAssetToEdit;

		FSoftClassPath DialogueClassPath = GetDefault<UDialogueEditorSettings>()->DefaultDialogueClass;
		UClass* DialogueClass = (DialogueClassPath.IsValid() ? LoadObject<UClass>(NULL, *DialogueClassPath.ToString()) : UDialogue::StaticClass());

		if (DialogueClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Unable to load Dialogue Class '%s'. Falling back to generic UDialogue."), *DialogueClassPath.ToString());
			DialogueClass = UDialogue::StaticClass();
		}

		if (!DialogueAsset->Dialogue)
		{
			DialogueAsset->Dialogue = NewObject<UDialogue>(DialogueAsset, DialogueClass, DialogueAsset->GetFName());
			check(DialogueAsset->Dialogue);//shouldnt fail
		} 
		//Got serial mismatch errors with this, removing. Need to find the proper UE5 way of re-instancing objects after a parent class change 
		//else if (DialogueAsset->Dialogue && DialogueAsset->Dialogue->GetClass() != DialogueClass) //Dialogue class doesn't match the one in settings - reinstance
		//{
		//	//Make the dialogue a CDO so we can use it as a template.
		//	DialogueAsset->Dialogue->SetFlags(RF_ClassDefaultObject);

		//	UDialogue* NewDialogue = NewObject<UDialogue>(DialogueAsset, DialogueClass, DialogueAsset->GetFName(), DialogueAsset->Dialogue->GetFlags(), DialogueAsset->Dialogue);
		//	UE_LOG(LogTemp, Warning, TEXT("Dialogue asset uses stale class %s. Reinstancing asset %s to use new class %s."), *GetNameSafe(DialogueAsset->Dialogue->GetClass()), *GetNameSafe(DialogueAsset), *DialogueClassPath.ToString());
		//	DialogueAsset->Dialogue = NewDialogue;

		//	DialogueAsset->Dialogue->ClearFlags(RF_ClassDefaultObject);
		//}

		if (DialogueAsset->Dialogue->DialogueName.IsEmpty())
		{
			DialogueAsset->Dialogue->DialogueName = FText::FromString(FName::NameToDisplayString(DialogueAsset->GetName(), false));
		}

		if (DialogueAsset->Dialogue->DialogueDescription.IsEmpty())
		{
			DialogueAsset->Dialogue->DialogueDescription = FText::FromString("Enter a description for your Dialogue here.");
		}
	}

	TSharedPtr<FDialogueGraphEditor> ThisPtr(SharedThis(this));

	//We need to initialize the document manager
	if (!DocumentManager.IsValid())
	{
		DocumentManager = MakeShareable(new FDocumentTracker);
		DocumentManager->Initialize(ThisPtr);

		//Register our graph editor tab with the factory
		TSharedRef<FDocumentTabFactory> GraphEditorFactory = MakeShareable(new FDialogueGraphEditorSummoner(ThisPtr,
			FDialogueGraphEditorSummoner::FOnCreateGraphEditorWidget::CreateSP(this, &FDialogueGraphEditor::CreateGraphEditorWidget)
		));

		GraphEditorTabFactoryPtr = GraphEditorFactory;
		DocumentManager->RegisterDocumentFactory(GraphEditorFactory);
	}

	if (!ToolbarBuilder.IsValid())
	{
		ToolbarBuilder = MakeShareable(new FDialogueEditorToolbar(SharedThis(this)));
	}

	TArray<UObject*> ObjectsToEdit;
	ObjectsToEdit.Add(DialogueAsset);

	const TArray<UObject*>* EditedObjects = GetObjectsCurrentlyBeingEdited();
	if (EditedObjects == nullptr || EditedObjects->Num() == 0)
	{
		FGraphEditorCommands::Register();
		FDialogueEditorCommands::Register();

		const TSharedRef<FTabManager::FLayout> DummyLayout = FTabManager::NewLayout("NullLayout")->AddArea(FTabManager::NewPrimaryArea());
		const bool bCreateDefaultStandaloneMenu = true;
		const bool bCreateDefaultToolbar = true;
		InitAssetEditor(Mode, InitToolkitHost, FNarrativeDialogueEditorModule::DialogueEditorAppId, DummyLayout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, ObjectsToEdit);

		BindCommonCommands();
		ExtendMenu();
		CreateInternalWidgets();

		FNarrativeDialogueEditorModule& DialogueEditorModule = FModuleManager::LoadModuleChecked<FNarrativeDialogueEditorModule>("NarrativeDialogueEditor");
		AddMenuExtender(DialogueEditorModule.GetMenuExtensibilityManager()->GetAllExtenders(GetToolkitCommands(), GetEditingObjects()));

		AddApplicationMode(DialogueEditorMode, MakeShareable(new FDialogueEditorApplicationMode(SharedThis(this))));

	}

	SetCurrentMode(DialogueEditorMode);

	RegenerateMenusAndToolbars();

	DetailsView->SetObject(DialogueAsset->Dialogue);

}

FName FDialogueGraphEditor::GetToolkitFName() const
{
	return FName("Dialogue Editor");
}

FText FDialogueGraphEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "DialogueEditor");
}

FString FDialogueGraphEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "DialogueEditor").ToString();
}

FLinearColor FDialogueGraphEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.0f, 0.0f, 0.2f, 0.5f);
}

FText FDialogueGraphEditor::GetToolkitName() const
{
	if(DialogueAsset)
	{
		return FAssetEditorToolkit::GetLabelForObject(DialogueAsset);
	}
	return FText();
}

FText FDialogueGraphEditor::GetToolkitToolTipText() const
{
	if (DialogueAsset)
	{
		return FAssetEditorToolkit::GetToolTipTextForObject(DialogueAsset);
	}
	return FText();
}

void FDialogueGraphEditor::CreateCommandList()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	GraphEditorCommands = MakeShareable(new FUICommandList);

	// Can't use CreateSP here because derived editor are already implementing TSharedFromThis<FAssetEditorToolkit>
	// however it should be safe, since commands are being used only within this editor
	// if it ever crashes, this function will have to go away and be reimplemented in each derived class

	GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::SelectAllNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanSelectAllNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::DeleteSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanDeleteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CopySelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanCopyNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CutSelectedNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanCutNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::PasteNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanPasteNodes)
	);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::DuplicateNodes),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanDuplicateNodes)
	);

	GraphEditorCommands->MapAction(FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CreateComment),
		FCanExecuteAction::CreateRaw(this, &FDialogueGraphEditor::CanCreateComment)
	);

}

void FDialogueGraphEditor::SelectAllNodes()
{
	if (TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin())
	{
		CurrentGraphEditor->SelectAllNodes();
	}
}

bool FDialogueGraphEditor::CanSelectAllNodes() const
{
	return true;
}

void FDialogueGraphEditor::DeleteSelectedNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());
	CurrentGraphEditor->GetCurrentGraph()->Modify();

	const FGraphPanelSelectionSet SelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*NodeIt))
		{
			if (Node->CanUserDeleteNode())
			{
				Node->Modify();
				Node->DestroyNode();
			}
		}
	}
}

bool FDialogueGraphEditor::CanDeleteNodes() const
{
	// If any of the nodes can be deleted then we should allow deleting
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanUserDeleteNode())
		{
			return true;
		}
	}

	return false;
}

void FDialogueGraphEditor::DeleteSelectedDuplicatableNodes()
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}
	const FGraphPanelSelectionSet OldSelectedNodes = CurrentGraphEditor->GetSelectedNodes();
	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	CurrentGraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			CurrentGraphEditor->SetNodeSelection(Node, true);
		}
	}
}

void FDialogueGraphEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	DeleteSelectedDuplicatableNodes();
}

bool FDialogueGraphEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FDialogueGraphEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard
	FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	FString ExportedText;

	for (FGraphPanelSelectionSet::TIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		UDialogueGraphNode* DialogueNode = Cast<UDialogueGraphNode>(Node);
		if (Node == nullptr)
		{
			SelectedIter.RemoveCurrent();
			continue;
		}

		Node->PrepareForCopying();
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

}

bool FDialogueGraphEditor::CanCopyNodes() const
{

	//Copying nodes is disabled for now
	return false;

	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if (Node && Node->CanDuplicateNode())
		{
			return true;
		}
	}

	return false;
}

void FDialogueGraphEditor::PasteNodes()
{

	if (TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin())
	{
		PasteNodesHere(CurrentGraphEditor->GetPasteLocation());
	}
}

void FDialogueGraphEditor::PasteNodesHere(const FVector2D& Location)
{
	TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return;
	}

	// Undo/Redo support
	const FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());
	UDialogueGraph* DialogueGraph = Cast<UDialogueGraph>(CurrentGraphEditor->GetCurrentGraph());

	DialogueGraph->Modify();

	UDialogueGraphNode* SelectedParent = NULL;
	bool bHasMultipleNodesSelected = false;

	const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	// Clear the selection set (newly pasted stuff will be selected)
	CurrentGraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(DialogueGraph, TextToImport, /*out*/ PastedNodes);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* PasteNode = *It;
		UDialogueGraphNode* PasteDialogueNode = Cast<UDialogueGraphNode>(PasteNode);

		if (PasteNode && (PasteDialogueNode == nullptr))
		{
			// Select the newly pasted stuff
			CurrentGraphEditor->SetNodeSelection(PasteNode, true);

			PasteNode->NodePosX = 200.f;
			PasteNode->NodePosY = 200.f;

			PasteNode->SnapToGrid(16);

			// Give new node a different Guid from the old one
			PasteNode->CreateNewGuid();
		}
	}

	// Update UI
	CurrentGraphEditor->NotifyGraphChanged();

	UObject* GraphOwner = DialogueGraph->GetOuter();
	if (GraphOwner)
	{
		GraphOwner->PostEditChange();
		GraphOwner->MarkPackageDirty();
	}
}

bool FDialogueGraphEditor::CanPasteNodes() const
{
	//Pasting nodes is disabled for now
	return false;

	TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	if (!CurrentGraphEditor.IsValid())
	{
		return false;
	}

	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(CurrentGraphEditor->GetCurrentGraph(), ClipboardContent);
}

void FDialogueGraphEditor::DuplicateNodes()
{
	CopySelectedNodes();
	PasteNodes();
}

bool FDialogueGraphEditor::CanDuplicateNodes() const
{
	//Duplicating nodes is disabled for now
	return false;

	return CanCopyNodes();
}

void FDialogueGraphEditor::CreateComment()
{
	TSharedPtr<SGraphEditor> GraphEditor = UpdateGraphEdPtr.Pin();
	if (GraphEditor.IsValid())
	{
		if (UEdGraph* Graph = GraphEditor->GetCurrentGraph())
		{
			if (const UEdGraphSchema* Schema = Graph->GetSchema())
			{
				FDialogueSchemaAction_AddComment CommentAction;
				CommentAction.PerformAction(Graph, NULL, GraphEditor->GetPasteLocation());
			}
		}
	}
}

bool FDialogueGraphEditor::CanCreateComment() const
{
	return true;
}

void FDialogueGraphEditor::ShowDialogueDetails()
{
	DetailsView->SetObject(DialogueAsset);
}

bool FDialogueGraphEditor::CanShowDialogueDetails() const
{
	return DialogueAsset && DialogueAsset->Dialogue;
}

void FDialogueGraphEditor::OpenNarrativeTutorialsInBrowser()
{
	FPlatformProcess::LaunchURL(*NarrativeHelpURL, NULL, NULL);
}

bool FDialogueGraphEditor::CanOpenNarrativeTutorialsInBrowser() const
{
	return true;
}

void FDialogueGraphEditor::QuickAddNode()
{
	//Disabled for now until we can find out why creating actions cause a nullptr crash
	return;

	//TSharedPtr<SGraphEditor> CurrentGraphEditor = UpdateGraphEdPtr.Pin();
	//if (!CurrentGraphEditor.IsValid())
	//{
	//	return;
	//}

	//const FScopedTransaction Transaction(FDialogueEditorCommands::Get().QuickAddNode->GetDescription());
	//UDialogueGraph* DialogueGraph = Cast<UDialogueGraph>(CurrentGraphEditor->GetCurrentGraph());

	//DialogueGraph->Modify();

	//const FGraphPanelSelectionSet SelectedNodes = GetSelectedNodes();

	//if (SelectedNodes.Num() == 0)
	//{
	//	return;
	//}

	//if (const UDialogueGraphSchema* Schema = Cast<UDialogueGraphSchema>(DialogueGraph->GetSchema()))
	//{
	//	for (auto& SelectedNode : SelectedNodes)
	//	{
	//		FDialogueSchemaAction_NewNode AddNewNode;
	//		UDialogueGraphNode* Node;
	//		//We we're quick adding from an action, add a new state after the action
	//		if (UDialogueGraphNode_Action* ActionNode = Cast<UDialogueGraphNode_Action>(SelectedNode))
	//		{
	//			//Dont do anything if we're already linked somewhere
	//			if (ActionNode->GetOutputPin()->LinkedTo.Num() == 0)
	//			{
	//				Node = NewObject<UDialogueGraphNode_State>(DialogueGraph, UDialogueGraphNode_State::StaticClass());
	//				AddNewNode.NodeTemplate = Node;

	//				FVector2D NewStateLocation = FVector2D(ActionNode->NodePosX, ActionNode->NodePosY);

	//				NewStateLocation.X += 300.f;

	//				AddNewNode.PerformAction(DialogueGraph, ActionNode->GetOutputPin(), NewStateLocation);


	//				// Update UI
	//				CurrentGraphEditor->NotifyGraphChanged();

	//				UObject* GraphOwner = DialogueGraph->GetOuter();
	//				if (GraphOwner)
	//				{
	//					GraphOwner->PostEditChange();
	//					GraphOwner->MarkPackageDirty();
	//				}
	//			}
	//		}
	//		else if (UDialogueGraphNode_State* StateNode = Cast<UDialogueGraphNode_State>(SelectedNode))
	//		{
	//			//Dont allow adding nodes from a failure or success node
	//			if (!(StateNode->IsA<UDialogueGraphNode_Failure>() || StateNode->IsA<UDialogueGraphNode_Success>()))
	//			{
	//				//For some reason this line crashes the editor and so quick add is disabled for now.
	//				Node = NewObject<UDialogueGraphNode_Action>(DialogueGraph, UDialogueGraphNode_Action::StaticClass());
	//				AddNewNode.NodeTemplate = Node;

	//				FVector2D NewActionLocation = FVector2D(ActionNode->NodePosX, ActionNode->NodePosY);

	//				NewActionLocation.X += 300.f;

	//				//Make sure new node doesn't overlay any others
	//				for (auto& LinkedTo : ActionNode->GetOutputPin()->LinkedTo)
	//				{
	//					if (LinkedTo->GetOwningNode()->NodePosY < NewActionLocation.Y)
	//					{
	//						NewActionLocation.Y = LinkedTo->GetOwningNode()->NodePosY - 200.f;
	//					}
	//				}

	//				AddNewNode.PerformAction(DialogueGraph, ActionNode->GetOutputPin(), NewActionLocation);


	//				// Update UI
	//				CurrentGraphEditor->NotifyGraphChanged();

	//				UObject* GraphOwner = DialogueGraph->GetOuter();
	//				if (GraphOwner)
	//				{
	//					GraphOwner->PostEditChange();
	//					GraphOwner->MarkPackageDirty();
	//				}
	//			}
	//		}
	//	}
	//}
}

bool FDialogueGraphEditor::CanQuickAddNode() const
{
	return true;
}

void FDialogueGraphEditor::FocusWindow(UObject* ObjectToFocusOn /*= NULL*/)
{
	if (ObjectToFocusOn == DialogueAsset)
	{
		SetCurrentMode(DialogueEditorMode);
	}
	FWorkflowCentricApplication::FocusWindow(ObjectToFocusOn);
}

bool FDialogueGraphEditor::GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const
{
	TSharedPtr<SGraphEditor> FocusedGraphEd = UpdateGraphEdPtr.Pin();
	return FocusedGraphEd.IsValid() && FocusedGraphEd->GetBoundsForSelectedNodes(Rect, Padding);
}

void FDialogueGraphEditor::PostUndo(bool bSuccess)
{
	if (bSuccess)
	{
	}

	FEditorUndoClient::PostUndo(bSuccess);

	if (DialogueAsset->DialogueGraph)
	{
		// Update UI
		DialogueAsset->DialogueGraph->NotifyGraphChanged();
		if (DialogueAsset)
		{
			DialogueAsset->PostEditChange();
			DialogueAsset->MarkPackageDirty();
		}
	}

}

void FDialogueGraphEditor::PostRedo(bool bSuccess)
{
	if (bSuccess)
	{
	}

	FEditorUndoClient::PostRedo(bSuccess);

	// Update UI
	if (DialogueAsset->DialogueGraph)
	{
		DialogueAsset->DialogueGraph->NotifyGraphChanged();
		if (DialogueAsset)
		{
			DialogueAsset->PostEditChange();
			DialogueAsset->MarkPackageDirty();
		}
	}
}

void FDialogueGraphEditor::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	//TODO implement any changes
}

void FDialogueGraphEditor::OnNodeDoubleClicked(class UEdGraphNode* Node)
{

}

void FDialogueGraphEditor::OnGraphEditorFocused(const TSharedRef<SGraphEditor>& InGraphEditor)
{
	UpdateGraphEdPtr = InGraphEditor;

	FGraphPanelSelectionSet CurrentSelection;
	CurrentSelection = InGraphEditor->GetSelectedNodes();
	//OnSelectedNodesChanged(CurrentSelection);
}

void FDialogueGraphEditor::OnNodeTitleCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* NodeBeingChanged)
{
	if (NodeBeingChanged)
	{
		static const FText TranscationTitle = FText::FromString(FString(TEXT("Rename Node")));
		const FScopedTransaction Transaction(TranscationTitle);
		NodeBeingChanged->Modify();
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	}
}

void FDialogueGraphEditor::OnAddInputPin()
{
	FGraphPanelSelectionSet CurrentSelection;
	TSharedPtr<SGraphEditor> FocusedGraphEd = UpdateGraphEdPtr.Pin();
	if (FocusedGraphEd.IsValid())
	{
		CurrentSelection = FocusedGraphEd->GetSelectedNodes();
	}

	// Iterate over all nodes, and add the pin
	for (FGraphPanelSelectionSet::TConstIterator It(CurrentSelection); It; ++It)
	{
		//UBehaviorTreeDecoratorGraphNode_Logic* LogicNode = Cast<UBehaviorTreeDecoratorGraphNode_Logic>(*It);
		//if (LogicNode)
		//{
		//	const FScopedTransaction Transaction(LOCTEXT("AddInputPin", "Add Input Pin"));

		//	LogicNode->Modify();
		//	LogicNode->AddInputPin();

		//	const UEdGraphSchema* Schema = LogicNode->GetSchema();
		//	Schema->ReconstructNode(*LogicNode);
		//}
	}

	// Refresh the current graph, so the pins can be updated
	if (FocusedGraphEd.IsValid())
	{
		FocusedGraphEd->NotifyGraphChanged();
	}
}

bool FDialogueGraphEditor::CanAddInputPin() const
{
	return true;
}

void FDialogueGraphEditor::OnRemoveInputPin()
{

}

bool FDialogueGraphEditor::CanRemoveInputPin() const
{
	return true;
}

FText FDialogueGraphEditor::GetDialogueEditorTitle() const
{
	if (DialogueAsset && DialogueAsset->Dialogue)
	{
		return DialogueAsset->GetDialogueName();
	}
	return FText::GetEmpty();
}

bool FDialogueGraphEditor::IsPropertyEditable() const
{
	return true;
}

void FDialogueGraphEditor::OnPackageSaved(const FString& PackageFileName, UObject* Outer)
{

}

void FDialogueGraphEditor::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{

}

FGraphAppearanceInfo FDialogueGraphEditor::GetGraphAppearance() const
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "NARRATIVE DIALOGUE EDITOR");
	return AppearanceInfo;
}

bool FDialogueGraphEditor::InEditingMode(bool bGraphIsEditable) const
{
	return bGraphIsEditable;
}

bool FDialogueGraphEditor::CanAccessDialogueEditorMode() const
{
	return IsValid(DialogueAsset);
}

FText FDialogueGraphEditor::GetLocalizedMode(FName InMode)
{
	static TMap< FName, FText > LocModes;

	if (LocModes.Num() == 0)
	{
		LocModes.Add(DialogueEditorMode, LOCTEXT("DialogueEditorMode", "Dialogue Graph"));
	}

	check(InMode != NAME_None);
	const FText* OutDesc = LocModes.Find(InMode);
	check(OutDesc);
	return *OutDesc;
}

UDialogueAsset* FDialogueGraphEditor::GetDialogueAsset() const
{
	return DialogueAsset;
}

TSharedRef<SWidget> FDialogueGraphEditor::SpawnProperties()
{
	return
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.HAlign(HAlign_Fill)
		[
			DetailsView.ToSharedRef()
		];
}

void FDialogueGraphEditor::RegisterToolbarTab(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FDialogueGraphEditor::RestoreDialogueGraph()
{
	UDialogueGraph* MyGraph = Cast<UDialogueGraph>(DialogueAsset->DialogueGraph);
	const bool bNewGraph = MyGraph == NULL;
	if (MyGraph == NULL)
	{
		DialogueAsset->DialogueGraph = FBlueprintEditorUtils::CreateNewGraph(DialogueAsset, TEXT("Dialogue Graph"), UDialogueGraph::StaticClass(), UDialogueGraphSchema::StaticClass());
		MyGraph = Cast<UDialogueGraph>(DialogueAsset->DialogueGraph);

		// Initialize the behavior tree graph
		const UEdGraphSchema* Schema = MyGraph->GetSchema();
		Schema->CreateDefaultNodesForGraph(*MyGraph);

		MyGraph->OnCreated();
	}
	else
	{
		MyGraph->OnLoaded();
	}

	MyGraph->Initialize();

	TSharedRef<FTabPayload_UObject> Payload = FTabPayload_UObject::Make(MyGraph);
	TSharedPtr<SDockTab> DocumentTab = DocumentManager->OpenDocument(Payload, bNewGraph ? FDocumentTracker::OpenNewDocument : FDocumentTracker::RestorePreviousDocument);

	if (DialogueAsset->LastEditedDocuments.Num() > 0)
	{
		TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(DocumentTab->GetContent());
		GraphEditor->SetViewLocation(DialogueAsset->LastEditedDocuments[0].SavedViewOffset, DialogueAsset->LastEditedDocuments[0].SavedZoomAmount);
	}
}

void FDialogueGraphEditor::SaveEditedObjectState()
{
	DialogueAsset->LastEditedDocuments.Empty();
	DocumentManager->SaveAllState();
}

void FDialogueGraphEditor::SaveAsset_Execute()
{
	IDialogueEditor::SaveAsset_Execute();
}

TSharedRef<class SGraphEditor> FDialogueGraphEditor::CreateGraphEditorWidget(UEdGraph* InGraph)
{
	check(InGraph);

	//Register commands
	if (!GraphEditorCommands.IsValid())
	{
		CreateCommandList();

		//Commented out as narrative doesn't allow removing/adding execution pins
		//GraphEditorCommands->MapAction(FGraphEditorCommands::Get().RemoveExecutionPin,
		//	FExecuteAction::CreateSP(this, &FDialogueGraphEditor::OnRemoveInputPin),
		//	FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanRemoveInputPin)
		//);

		//GraphEditorCommands->MapAction(FGraphEditorCommands::Get().AddExecutionPin,
		//	FExecuteAction::CreateSP(this, &FDialogueGraphEditor::OnAddInputPin),
		//	FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanAddInputPin)
		//);
	}

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FDialogueGraphEditor::OnSelectedNodesChanged);
	InEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateSP(this, &FDialogueGraphEditor::OnNodeDoubleClicked);
	InEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FDialogueGraphEditor::OnNodeTitleCommitted);

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
		.HAlign(HAlign_Center)
		.FillWidth(1.f)
		[
			SNew(STextBlock)
			.Text(this, &FDialogueGraphEditor::GetDialogueEditorTitle)
			.TextStyle(FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
		]
		];

	// Make full graph editor
	const bool bGraphIsEditable = InGraph->bEditable;
	return SNew(SGraphEditor)
		.AdditionalCommands(GraphEditorCommands)
		.IsEditable(this, &FDialogueGraphEditor::InEditingMode, bGraphIsEditable)
		.Appearance(this, &FDialogueGraphEditor::GetGraphAppearance)
		.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents);

}

void FDialogueGraphEditor::CreateInternalWidgets()
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = false;
	DetailsViewArgs.NotifyHook = this;
	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(NULL);
	DetailsView->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateSP(this, &FDialogueGraphEditor::IsPropertyEditable));
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FDialogueGraphEditor::OnFinishedChangingProperties);
}

void FDialogueGraphEditor::ExtendMenu()
{
}

void FDialogueGraphEditor::BindCommonCommands()
{
	ToolkitCommands->MapAction(FDialogueEditorCommands::Get().ShowDialogueDetails,
		FExecuteAction::CreateSP(this, &FDialogueGraphEditor::ShowDialogueDetails),
		FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanShowDialogueDetails));

	ToolkitCommands->MapAction(FDialogueEditorCommands::Get().ViewTutorial,
		FExecuteAction::CreateSP(this, &FDialogueGraphEditor::OpenNarrativeTutorialsInBrowser),
		FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanOpenNarrativeTutorialsInBrowser));

	ToolkitCommands->MapAction(FDialogueEditorCommands::Get().QuickAddNode,
		FExecuteAction::CreateSP(this, &FDialogueGraphEditor::QuickAddNode),
		FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanQuickAddNode));
}

#undef LOCTEXT_NAMESPACE