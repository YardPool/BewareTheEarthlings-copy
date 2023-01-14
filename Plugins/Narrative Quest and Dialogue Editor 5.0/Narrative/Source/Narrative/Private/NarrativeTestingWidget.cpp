// Copyright Narrative Tools 2022. 


#include "NarrativeTestingWidget.h"
#include "NarrativeComponent.h"
#include <NarrativeFunctionLibrary.h>

void UNarrativeTestingWidget::NativeConstruct()
{
	Init();
}

void UNarrativeTestingWidget::NativeDestruct()
{

}

void UNarrativeTestingWidget::Init()
{
	if (QuestNarrativeComp)
	{
		QuestNarrativeComp->OnQuestStarted.AddDynamic(this, &UNarrativeTestingWidget::OnQuestStarted);
		QuestNarrativeComp->OnQuestFailed.AddDynamic(this, &UNarrativeTestingWidget::OnQuestFailed);
		QuestNarrativeComp->OnQuestSucceeded.AddDynamic(this, &UNarrativeTestingWidget::OnQuestSucceeded);
		QuestNarrativeComp->OnQuestStepCompleted.AddDynamic(this, &UNarrativeTestingWidget::OnQuestStepCompleted);
		QuestNarrativeComp->OnQuestNewState.AddDynamic(this, &UNarrativeTestingWidget::OnQuestNewState);
		QuestNarrativeComp->OnQuestTaskProgressMade.AddDynamic(this, &UNarrativeTestingWidget::OnQuestTaskProgressMade);
		QuestNarrativeComp->OnQuestTaskCompleted.AddDynamic(this, &UNarrativeTestingWidget::OnQuestTaskCompleted);
		QuestNarrativeComp->OnBeginSave.AddDynamic(this, &UNarrativeTestingWidget::OnBeginSave);
		QuestNarrativeComp->OnBeginLoad.AddDynamic(this, &UNarrativeTestingWidget::OnBeginLoad);
		QuestNarrativeComp->OnSaveComplete.AddDynamic(this, &UNarrativeTestingWidget::OnSaveComplete);
		QuestNarrativeComp->OnLoadComplete.AddDynamic(this, &UNarrativeTestingWidget::OnLoadComplete);
	}

	if (DialogueNarrativeComp)
	{
		DialogueNarrativeComp->OnDialogueFinished.AddDynamic(this, &UNarrativeTestingWidget::OnDialogueFinished);
		DialogueNarrativeComp->OnDialogueBegan.AddDynamic(this, &UNarrativeTestingWidget::OnDialogueBegan);
		DialogueNarrativeComp->OnDialogueUpdated.AddDynamic(this, &UNarrativeTestingWidget::OnDialogueUpdated);
	}
}

void UNarrativeTestingWidget::OnQuestStarted(const UQuest* Quest)
{
	BPOnQuestStarted(Quest);
}

void UNarrativeTestingWidget::OnQuestFailed(const UQuest* Quest, const FText& QuestFailedMessage)
{
	BPOnQuestFailed(Quest, QuestFailedMessage);
}

void UNarrativeTestingWidget::OnQuestSucceeded(const UQuest* Quest, const FText& QuestSucceededMessage)
{
	BPOnQuestSucceeded(Quest, QuestSucceededMessage);
}

void UNarrativeTestingWidget::OnQuestNewState(UQuest* Quest, const UQuestState* NewState)
{
	BPOnQuestNewState(Quest, NewState);
}

void UNarrativeTestingWidget::OnQuestTaskProgressMade(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step, int32 CurrentProgress, int32 RequiredProgress)
{
	BPOnQuestTaskProgressMade(Quest, Task, Step, CurrentProgress, RequiredProgress);
}

void UNarrativeTestingWidget::OnQuestTaskCompleted(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step)
{
	BPOnQuestTaskCompleted(Quest, Task, Step);
}

void UNarrativeTestingWidget::OnQuestStepCompleted(const UQuest* Quest, const class UQuestBranch* Step)
{
	BPOnQuestStepCompleted(Quest, Step);
}

void UNarrativeTestingWidget::OnBeginSave(FString SaveName)
{
	BPOnBeginSave(SaveName);
}

void UNarrativeTestingWidget::OnBeginLoad(FString SaveName)
{
	BPOnBeginLoad(SaveName);
}

void UNarrativeTestingWidget::OnSaveComplete(FString SaveName)
{
	BPOnSaveComplete(SaveName);
}

void UNarrativeTestingWidget::OnLoadComplete(FString SaveName)
{
	BPOnLoadComplete(SaveName);
}

void UNarrativeTestingWidget::OnDialogueUpdated(class UDialogue* Dialogue, const TArray<UDialogueNode_NPC*>& NPCReplies, const TArray<UDialogueNode_Player*>& PlayerReplies)
{
	BPOnDialogueUpdated(Dialogue, NPCReplies, PlayerReplies);
}

void UNarrativeTestingWidget::OnDialogueBegan(class UDialogue* Dialogue)
{
	BPOnDialogueBegan(Dialogue);
}

void UNarrativeTestingWidget::OnDialogueFinished(class UDialogue* Dialogue)
{
	BPOnDialogueFinished(Dialogue);
}
