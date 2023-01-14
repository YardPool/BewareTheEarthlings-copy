// Copyright Narrative Tools 2022. 


#include "DialogueAsset.h"
#include "Dialogue.h"

UDialogueAsset::UDialogueAsset()
{
	Dialogue = CreateDefaultSubobject<UDialogue>(TEXT("Dialogue"));
}

FText UDialogueAsset::GetDialogueName() const
{
	return Dialogue ? Dialogue->DialogueName : FText();
}

FText UDialogueAsset::GetDialogueDescription() const
{
	return Dialogue ? Dialogue->DialogueDescription : FText();
}
