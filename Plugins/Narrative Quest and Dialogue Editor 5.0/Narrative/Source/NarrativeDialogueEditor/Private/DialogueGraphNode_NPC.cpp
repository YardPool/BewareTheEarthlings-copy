// Copyright Narrative Tools 2022. 

#include "DialogueGraphNode_NPC.h"
#include "DialogueGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "DialogueEditorTypes.h"
#include "DialogueGraphNode.h"
#include "DialogueSM.h"
#include "Dialogue.h"

void UDialogueGraphNode_NPC::AllocateDefaultPins()
{
	CreatePin(EGPD_Input, UDialogueEditorTypes::PinCategory_SingleNode, TEXT(""));
	CreatePin(EGPD_Output, UDialogueEditorTypes::PinCategory_SingleNode, TEXT(""));
}
