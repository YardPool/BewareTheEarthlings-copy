// Copyright Narrative Tools 2022. 

#include "Dialogue.h"
#include "Net/UnrealNetwork.h"
#include "NarrativeComponent.h"
#include "DialogueSM.h"

UDialogue::UDialogue()
{
	DialogueSequenceSettings.bPauseAtEnd = true;
	PlayerTalkingSettings.bPauseAtEnd = true;
	NPCTalkingSettings.bPauseAtEnd = true;
	SelectReplySettings.bPauseAtEnd = true;
}

void UDialogue::Initialize(class AActor* NPC, const FText& InNPCName)
{
	//Outer should always be a narrative comp
	NarrativeComp = CastChecked<UNarrativeComponent>(GetOuter());
	NPCActor = NPC;
	NPCName = InNPCName;

	if (RootDialogue)
	{
		RootDialogue->OwningDialogue = this;
	}

	for (auto& Reply : NPCReplies)
	{
		if (Reply)
		{
			Reply->OwningDialogue = this;
			Reply->OwningComponent = NarrativeComp;
		}
	}

	for (auto& Reply : PlayerReplies)
	{
		if (Reply)
		{
			Reply->OwningDialogue = this;
			Reply->OwningComponent = NarrativeComp;
		}
	}
}
