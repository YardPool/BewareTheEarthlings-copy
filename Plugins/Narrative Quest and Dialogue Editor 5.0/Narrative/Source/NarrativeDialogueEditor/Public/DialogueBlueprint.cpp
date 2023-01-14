// Copyright Narrative Tools 2022. 


#include "DialogueBlueprint.h"
#include "Dialogue.h"
#include "DialogueSM.h"
#include "DialogueBlueprintGeneratedClass.h"

UDialogueBlueprint::UDialogueBlueprint()
{
	DialogueTemplate = CreateDefaultSubobject<UDialogue>(TEXT("DialogueTemplate"));
	DialogueTemplate->SetFlags(RF_Transactional | RF_ArchetypeObject);

	//Any dialogues created prior to the speakers update need a speaker added 
	if (DialogueTemplate->Speakers.Num() == 0)
	{
		FSpeakerInfo DefaultSpeaker;
		DefaultSpeaker.DefaultShot = nullptr;
		DefaultSpeaker.SpeakerID = GetFName();
		DialogueTemplate->Speakers.Add(DefaultSpeaker);
	}
}

UClass* UDialogueBlueprint::GetBlueprintClass() const
{
	return UDialogueBlueprintGeneratedClass::StaticClass();
}

void UDialogueBlueprint::PostLoad()
{
	Super::PostLoad();

	DialogueTemplate->SetFlags(RF_Transactional | RF_ArchetypeObject);
}

bool UDialogueBlueprint::ValidateGeneratedClass(const UClass* InClass)
{
	return true;
}
