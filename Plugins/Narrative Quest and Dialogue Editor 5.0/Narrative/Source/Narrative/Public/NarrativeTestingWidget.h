// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "QuestSM.h"
#include "Blueprint/UserWidget.h"
#include "NarrativeTestingWidget.generated.h"

class UQuestBlueprint;

/**
 * Base class for the Narrative demo widget that ships with narrative. 
 */
UCLASS()
class NARRATIVE_API UNarrativeTestingWidget : public UUserWidget
{
	GENERATED_BODY()

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

public:

	UFUNCTION(BlueprintCallable, Category = "Narrative")
	void Init();

	//The narrative component to show quest updates from. Most of the time this will just be the same as DialogueNarrativeComp
	UPROPERTY(BlueprintReadWrite, Category = "Narrative", meta = (ExposeOnSpawn=true))
	class UNarrativeComponent* QuestNarrativeComp;
	
	//The narrative component to show dialogue updates from. Most of the time this will just be the same as QuestNarrativeComp
	UPROPERTY(BlueprintReadWrite, Category = "Narrative", meta = (ExposeOnSpawn = true))
	class UNarrativeComponent* DialogueNarrativeComp;

	UFUNCTION()
	void OnQuestStarted(const UQuest* Quest);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestStarted(const UQuest* Quest);

	UFUNCTION()
	void OnQuestFailed(const UQuest* Quest, const FText& QuestFailedMessage);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestFailed(const UQuest* Quest, const FText& QuestFailedMessage);

	UFUNCTION()
	void OnQuestSucceeded(const UQuest* Quest, const FText& QuestSucceededMessage);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestSucceeded(const UQuest* Quest, const FText& QuestSucceededMessage);

	UFUNCTION()
	void OnQuestNewState(UQuest* Quest, const UQuestState* NewState);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestNewState(UQuest* Quest, const UQuestState* NewState);

	UFUNCTION()
	void OnQuestTaskProgressMade(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step, int32 CurrentProgress, int32 RequiredProgress);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestTaskProgressMade(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step, int32 CurrentProgress, int32 RequiredProgress);

	UFUNCTION()
	void OnQuestTaskCompleted(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestTaskCompleted(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step);

	UFUNCTION()
	void OnQuestStepCompleted(const UQuest* Quest, const class UQuestBranch* Step);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnQuestStepCompleted(const UQuest* Quest, const class UQuestBranch* Step);

	UFUNCTION()
	void OnBeginSave(FString SaveName);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnBeginSave(const FString& SaveName);

	UFUNCTION()
	void OnBeginLoad(FString SaveName);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnBeginLoad(const FString& SaveName);

	UFUNCTION()
	void OnSaveComplete(FString SaveName);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnSaveComplete(const FString& SaveName);

	UFUNCTION()
	void OnLoadComplete(FString SaveName);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnLoadComplete(const FString& SaveName);

	UFUNCTION()
	void OnDialogueUpdated(class UDialogue* Dialogue, const TArray<class UDialogueNode_NPC*>& NPCReplies, const TArray<class UDialogueNode_Player*>& PlayerReplies);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnDialogueUpdated(class UDialogue* Dialogue, const TArray<class UDialogueNode_NPC*>& NPCReplies, const TArray<class UDialogueNode_Player*>& PlayerReplies);

	UFUNCTION()
	void OnDialogueBegan(class UDialogue* Dialogue);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnDialogueBegan(class UDialogue* Dialogue);

	UFUNCTION()
	void OnDialogueFinished(class UDialogue* Dialogue);

	UFUNCTION(BlueprintImplementableEvent)
	void BPOnDialogueFinished(class UDialogue* Dialogue);

};
