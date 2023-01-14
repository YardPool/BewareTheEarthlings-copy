// Copyright Narrative Tools 2022. 


#include "DialogueEditorSettings.h"
#include "Dialogue.h"
#include "DialogueSM.h"

UDialogueEditorSettings::UDialogueEditorSettings()
{
	RootNodeColor = FLinearColor(0.1f, 0.1f, 0.1f);

	DefaultNPCDialogueClass = UDialogueNode_NPC::StaticClass();
	DefaultPlayerDialogueClass = UDialogueNode_Player::StaticClass();
	DefaultDialogueClass = UDialogue::StaticClass();
}
