// Copyright Narrative Tools 2022. 

#include "NarrativeComponent.h"
#include "NarrativeSaveGame.h"
#include "NarrativeFunctionLibrary.h"
#include "Quest.h"
#include "NarrativeTask.h"
#include "QuestSM.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/ActorChannel.h"
#include "DialogueSM.h"
#include "Dialogue.h"
#include "DialogueAsset.h"
#include "NarrativeCondition.h"
#include "NarrativeEvent.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"

DEFINE_LOG_CATEGORY(LogNarrative);

static TAutoConsoleVariable<bool> CVarShowQuestUpdates(
	TEXT("narrative.ShowQuestUpdates"),
	false,
	TEXT("Show updates to any of our quests on screen.\n")
);

// Sets default values for this component's properties
UNarrativeComponent::UNarrativeComponent()
{
	SetIsReplicatedByDefault(true);

	bAutosaveOnQuestUpdated = true;
	bAutosaveOnEndPlay = true;
	AutoSaveName = "NarrativeSaveData";

	OnQuestStarted.AddDynamic(this, &UNarrativeComponent::QuestStarted);
	OnQuestFailed.AddDynamic(this, &UNarrativeComponent::QuestFailed);
	OnQuestSucceeded.AddDynamic(this, &UNarrativeComponent::QuestSucceeded);
	OnQuestForgotten.AddDynamic(this, &UNarrativeComponent::QuestForgotten);
	OnQuestStepCompleted.AddDynamic(this, &UNarrativeComponent::QuestStepCompleted);
	OnQuestNewState.AddDynamic(this, &UNarrativeComponent::QuestNewState);
	OnQuestTaskProgressMade.AddDynamic(this, &UNarrativeComponent::QuestTaskProgressMade);
	OnBeginSave.AddDynamic(this, &UNarrativeComponent::BeginSave);
	OnBeginLoad.AddDynamic(this, &UNarrativeComponent::BeginLoad);
	OnSaveComplete.AddDynamic(this, &UNarrativeComponent::SaveComplete);
	OnLoadComplete.AddDynamic(this, &UNarrativeComponent::LoadComplete);

	OnDialogueBegan.AddDynamic(this, &UNarrativeComponent::DialogueBegan);
	OnDialogueFinished.AddDynamic(this, &UNarrativeComponent::DialogueFinished);
	OnDialogueUpdated.AddDynamic(this, &UNarrativeComponent::DialogueUpdated);
}


void UNarrativeComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPC = GetOwningController();
}

void UNarrativeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	Super::EndPlay(EndPlayReason);

	if (bAutosaveOnEndPlay)
	{
		PerformAutosave();
	}

}

void UNarrativeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UNarrativeComponent, PendingUpdateList);
	DOREPLIFETIME(UNarrativeComponent, CurrentDialogue);

	//Uncomment if you don't other players quests to replicate to you 
	//DOREPLIFETIME_CONDITION(UNarrativeComponent, PendingUpdateList, COND_OwnerOnly);
}

bool UNarrativeComponent::IsQuestStartedOrFinished(TSubclassOf<class UQuest> QuestClass) const
{
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress && QuestInProgress->GetClass()->IsChildOf(QuestClass))
		{
			return QuestInProgress->QuestCompletion != EQuestCompletion::QC_NotStarted;
		}
	}

	//If quest isnt in the quest list at all we can return false
	return false;
}

bool UNarrativeComponent::IsQuestInProgress(TSubclassOf<class UQuest> QuestClass) const
{  
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress && QuestInProgress->GetClass()->IsChildOf(QuestClass))
		{
			return QuestInProgress->QuestCompletion == EQuestCompletion::QC_Started;
		}
	}

	return false;
}

bool UNarrativeComponent::IsQuestSucceeded(TSubclassOf<class UQuest> QuestClass) const
{
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress && QuestInProgress->GetClass()->IsChildOf(QuestClass))
		{
			return QuestInProgress->QuestCompletion == EQuestCompletion::QC_Succeded;
		}
	}
	return false;
}

bool UNarrativeComponent::IsQuestFailed(TSubclassOf<class UQuest> QuestClass) const
{
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress && QuestInProgress->GetClass()->IsChildOf(QuestClass))
		{
			return QuestInProgress->QuestCompletion == EQuestCompletion::QC_Failed;
		}
	}
	return false;
}

bool UNarrativeComponent::IsQuestFinished(TSubclassOf<class UQuest> QuestClass) const
{
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress && QuestInProgress->GetClass()->IsChildOf(QuestClass))
		{
			return QuestInProgress->QuestCompletion == EQuestCompletion::QC_Failed || QuestInProgress->QuestCompletion == EQuestCompletion::QC_Succeded;
		}
	}
	return false;
}

class UQuest* UNarrativeComponent::BeginQuest(TSubclassOf<class UQuest> QuestClass, FName StartFromID /*= NAME_None*/)
{
	if (IsValid(QuestClass))
	{
		if (UQuest* NewQuest = BeginQuest_Internal(QuestClass, StartFromID))
		{
			OnQuestStarted.Broadcast(NewQuest);

			return NewQuest;
		}
	}

	return nullptr;
}

bool UNarrativeComponent::RestartQuest(TSubclassOf<class UQuest> QuestClass, FName StartFromID)
{
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (int32 i = 0; i < QuestList.Num(); i++)
	{
		if (QuestList.IsValidIndex(i) && QuestList[i]->GetClass()->IsChildOf(QuestClass))
		{
			OnQuestRestarted.Broadcast(QuestList[i]);
			QuestList.RemoveAt(i);
			BeginQuest(QuestClass, StartFromID);

			if (GetOwner()->HasAuthority() && GetNetMode() != NM_Standalone)
			{
				SendNarrativeUpdate(FNarrativeUpdate::RestartQuest(QuestClass, StartFromID));
			}

			return true;
		}
	}

	return false;
}

bool UNarrativeComponent::ForgetQuest(TSubclassOf<class UQuest> QuestClass)
{
	if (!IsValid(QuestClass))
	{
		return false;
	}

	for (int32 i = 0; i < QuestList.Num(); i++)
	{
		if (QuestList.IsValidIndex(i) && QuestList[i]->GetClass()->IsChildOf(QuestClass))
		{
			OnQuestForgotten.Broadcast(QuestList[i]);
			QuestList.RemoveAt(i);

			if (GetOwner()->HasAuthority() && GetNetMode() != NM_Standalone)
			{
				SendNarrativeUpdate(FNarrativeUpdate::ForgetQuest(QuestClass));
			}
			return true;
		}
	}

	return false;
}

bool UNarrativeComponent::BeginDialogue(UDialogueAsset* DialogueAsset, class AActor* NPC, FText NPCName)
{
	if (GetOwner() && GetOwner()->HasAuthority() )
	{
		APlayerController* OwningController = GetOwningController();
		APawn* OwningPawn = GetOwningPawn();

		//Set these before we check conditions so BP conditions can access this info
		if (DialogueAsset && DialogueAsset->Dialogue && DialogueAsset->Dialogue->RootDialogue)
		{
			//Find the root node and all replies after it and send them to the UI
			TArray<UDialogueNode_NPC*> NPCReplyChain = DialogueAsset->Dialogue->RootDialogue->GetReplyChain(OwningController, OwningPawn, this);

			//Get the player responses to the last reply in the reply chain 
			TArray<UDialogueNode_Player*> PlayerReplies = NPCReplyChain.Num() ? NPCReplyChain.Last()->GetPlayerReplies(OwningController, OwningPawn, this) : TArray<UDialogueNode_Player*>();

			if (NPCReplyChain.Num() > 0)
			{
				//Rep the dialogue back to the client so it can begin
				CurrentDialogue.Dialogue = DialogueAsset->Dialogue;
				CurrentDialogue.NPC = NPC;
				CurrentDialogue.NPCName = !NPCName.IsEmpty() ? NPCName : DialogueAsset->Dialogue->NPCName;

				//OnRep_CurrentDialogue();

				//Tell client to begin the dialogue 
				ClientBeginDialogue(CurrentDialogue, NPCReplyChain, PlayerReplies);

				//Narrative tracks beginning dialogues 
				CompleteNarrativeTask("TalkToNPC", CurrentDialogue.NPCName.ToString());

				return true;
			}
			else
			{
				ExitDialogue();
				return false;
			}
		}
		else
		{
			UE_LOG(LogNarrative, Error, TEXT("Narrative tried beginning a dialogue but dialogue was null."));
		}
	}
	return false;
}

void UNarrativeComponent::ExitDialogue()
{
	if (CurrentDialogue.Dialogue)
	{
		ServerExitDialogue();
	}
}

void UNarrativeComponent::ServerExitDialogue_Implementation()
{
	if (CurrentDialogue.Dialogue)
	{
		CurrentDialogue.Dialogue = nullptr;
		OnRep_CurrentDialogue();
	}
}

bool UNarrativeComponent::IsInDialogue()
{
	return CurrentDialogue.Dialogue != nullptr;
}

void UNarrativeComponent::SelectDialogueOption(class UDialogueNode_Player* PlayerReply)
{
	if (PlayerReply)
	{
		ServerSelectDialogueOption(PlayerReply);
	}
}

void UNarrativeComponent::ServerSelectDialogueOption_Implementation(class UDialogueNode_Player* PlayerReply)
{
	if (CurrentDialogue.Dialogue && PlayerReply)
	{
		APlayerController* OwningController = GetOwningController();
		APawn* OwningPawn = GetOwningPawn();

		//Client may have sent us a reply it wasn't actually allowed to select, check conditions before we process Tasks 
		if (!PlayerReply->AreConditionsMet(OwningPawn, OwningController, this))
		{
			return;
		}

		PlayerReply->ProcessEvents(OwningPawn, OwningController, this);

		//Narrative tracks selecting dialogue options as one of its out of the box tasks 
		CompleteNarrativeTask("SelectDialogueReply", PlayerReply->ID.ToString());

		//If the player says something the NPC doesn't have a reply to, then we've reached the end of the dialogue
		if (PlayerReply->NPCReplies.Num() <= 0)
		{
			ExitDialogue();
			return;
		}

		//Find the first valid NPC reply after our player reply. 
		UDialogueNode_NPC* NPCReply = nullptr;
		
		for (auto& NextNPCReply : PlayerReply->NPCReplies)
		{
			if (NextNPCReply->AreConditionsMet(OwningPawn, OwningController, this))
			{
				NPCReply = NextNPCReply;
				break;
			}
		}

		//If no valid NPC replies, exit dialogue
		if(!NPCReply)
		{
			ExitDialogue();
			return;
		}

		TArray<UDialogueNode_NPC*> NPCReplyChain = NPCReply->GetReplyChain(OwningController, OwningPawn, this);
		TArray<UDialogueNode_Player*> PlayerReplies = NPCReplyChain.Num() ? NPCReplyChain.Last()->GetPlayerReplies(OwningController, OwningPawn, this) : TArray<UDialogueNode_Player*>();

		ClientRecieveDialogueOptions(NPCReplyChain, PlayerReplies);
	}
}

void UNarrativeComponent::OnRep_CurrentDialogue()
{
	if (GetOwningController() && GetOwningController()->IsLocalController())
	{
		//UI binds to these to start or end dialogue 
		if (CurrentDialogue.Dialogue)
		{
			OnDialogueBegan.Broadcast(CurrentDialogue.Dialogue);
		}
		else if (!CurrentDialogue.Dialogue)
		{
			OnDialogueFinished.Broadcast(CurrentDialogue.Dialogue);
		}
	}
}

void UNarrativeComponent::ClientBeginDialogue_Implementation(const FDialogueInfo& Dialogue, const TArray<class UDialogueNode_NPC*>& NPCReplies, const TArray<class UDialogueNode_Player*>& PlayerReplies)
{
	//Set this via RPC as well as the OnRep_Dialogue - that way it doesn't matter which order they fire in, dialogue will always be valid
	CurrentDialogue = Dialogue;

	//Client received some new dialogue options, put them on the screen
	OnDialogueBegan.Broadcast(CurrentDialogue.Dialogue);
	OnDialogueUpdated.Broadcast(CurrentDialogue.Dialogue, NPCReplies, PlayerReplies);
}

void UNarrativeComponent::ClientRecieveDialogueOptions_Implementation(const TArray<class UDialogueNode_NPC*>& NPCReplies, const TArray<class UDialogueNode_Player*>& PlayerReplies)
{
	//Client received some new dialogue options, put them on the screen
	OnDialogueUpdated.Broadcast(CurrentDialogue.Dialogue, NPCReplies, PlayerReplies);
}

bool UNarrativeComponent::CompleteNarrativeTask(const UNarrativeTask* Task, const FString& Argument)
{
	/**
	Behind the scenes, narrative just uses lightweight strings for narrative Tasks. UNarrativeTasks just serve as nice containers for strings ,
	and prevents designers mistyping strings when making quests, since they don't have to type out the string every time, its stored in the asset.

	UNarrativeTasks also do other nice things for us like allowing us to store metadata about the Task, where a string wouldn't let us do that. 
	*/
	if (Task)
	{
		return CompleteNarrativeTask(Task->TaskName, Argument);
	}

	return false;
}

bool UNarrativeComponent::CompleteNarrativeTask(const FString& TaskName, const FString& Argument)
{
	if (GetOwnerRole() >= ROLE_Authority)
	{
		if (TaskName.IsEmpty() || Argument.IsEmpty())
		{
			UE_LOG(LogNarrative, Warning, TEXT("Narrative tried to process an Task that was empty, or argument was empty."));
			return false;
		}

		//We need to lookup the asset to call the delegate
		if (UNarrativeTask* TaskAsset = UNarrativeFunctionLibrary::GetTaskByName(this, TaskName))
		{
			OnNarrativeTaskCompleted.Broadcast(TaskAsset, Argument);
		}
		else
		{
			UE_LOG(LogNarrative, Warning, TEXT("Narrative tried finding the asset for Task %s, but couldn't find it."), *TaskName);
		}

		FString TaskString = (TaskName + '_' + Argument).ToLower();
		TaskString.RemoveSpacesInline();

		//Convert the Task into an FString and run it through our active quests state machines
		return CompleteNarrativeTask_Internal(TaskString, false);
	}
	else
	{
		//Client cant update quests 
		UE_LOG(LogNarrative, Log, TEXT("Client called UNarrativeComponent::CompleteNarrativeTask. This must be called by the server as quests are server authoritative."));
		return false;
	}

	return false;
}

class UQuest* UNarrativeComponent::BeginQuest_Internal(TSubclassOf<class UQuest> QuestClass, FName StartFromID /*= NAME_None*/)
{
	if (IsValid(QuestClass))
	{
		if (IsQuestInProgress(QuestClass))
		{
			UE_LOG(LogNarrative, Warning, TEXT("Narrative was asked to begin a quest the player is already doing. Use RestartQuest() to replay a started quest. "));
			return nullptr;
		}

		//Construct quest object from quest class since UBlueprints are nulled in packaged games
		UQuest* NewQuest = NewObject<UQuest>(GetOwner(), QuestClass);

		const bool bInitializedSuccessfully = NewQuest->Initialize(this, StartFromID);

		if (bInitializedSuccessfully && NewQuest)
		{
			check(NewQuest->GetCurrentState());

			if (NewQuest->GetCurrentState())
			{
				QuestList.Add(NewQuest);

				//TODO: Send initial quantity in update so clients quantity stays in sync since we added initial quantities
				if (GetOwner()->HasAuthority() && GetNetMode() != NM_Standalone)
				{
					SendNarrativeUpdate(FNarrativeUpdate::BeginQuest(QuestClass, StartFromID));
				}

				return NewQuest;
			}
		}
	}
	return nullptr;
}

bool UNarrativeComponent::CompleteNarrativeTask_Internal(const FString& RawTaskString, const bool bFromReplication)
{
	if (GetOwnerRole() >= ROLE_Authority || bFromReplication)
	{
		/*We consider an Task relevant if it was completed for the first time ever or if it updated a quest. 
		Relevant Tasks get sent to the client for it to re-run, otherwise the server just saves network by not sending relevant Tasks*/
		bool bTaskWasRelevant = false;

		if (int32* TimesCompleted = MasterTaskList.Find(RawTaskString))
		{
			++(*TimesCompleted);
		}
		else
		{
			MasterTaskList.Add(RawTaskString, 1);
			bTaskWasRelevant = true;
		}

		for (auto& QuestInProgress : QuestList)
		{
			if (UpdateQuest(QuestInProgress, RawTaskString) != EQuestProgress::QP_NoChange)
			{
				bTaskWasRelevant = true;
			}
		}

		//To keep things efficient, the server will only tell the client to replay Tasks that updated a quest
		if (GetOwnerRole() >= ROLE_Authority && GetNetMode() != NM_Standalone) // && bTaskWasRelevant) Task relevancy causing some issues with sync for now so disabled
		{
			//For some reason unreal crashes during replication if FNarrativeUpdate doesn't have a quest class supplied, so just send static class along with task string 
			SendNarrativeUpdate(FNarrativeUpdate::CompleteTask(UQuest::StaticClass(), RawTaskString)); 
		}

		return bTaskWasRelevant;
	}
	return false;
}

EQuestProgress UNarrativeComponent::UpdateQuest(UQuest* QuestToUpdate, const FString& TaskString)
{
	if (QuestToUpdate)
	{
		const EQuestProgress Progress = QuestToUpdate->UpdateQuest(this, TaskString);

		switch (Progress)
		{
		case EQuestProgress::QP_Succeeded:
		{
			OnQuestSucceeded.Broadcast(QuestToUpdate, QuestToUpdate->GetCurrentState()->Description);
		}
		break;
		case EQuestProgress::QP_Failed:
		{
			OnQuestFailed.Broadcast(QuestToUpdate, QuestToUpdate->GetCurrentState()->Description);
		}
		break;
		}

		return Progress;
	}

	return EQuestProgress::QP_NoChange;
}

bool UNarrativeComponent::HasCompletedTask(const UNarrativeTask* Task, const FString& Name, const int32 Quantity /*= 1 */)
{
	if (!Task)
	{
		return false;
	}

	return GetNumberOfTimesTaskWasCompleted(Task, Name) >= Quantity;
}

int32 UNarrativeComponent::GetNumberOfTimesTaskWasCompleted(const UNarrativeTask* Task, const FString& Name)
{
	if (!Task)
	{
		return 0;
	}

	if (int32* TimesCompleted = MasterTaskList.Find(Task->MakeTaskString(Name)))
	{
		return *TimesCompleted;
	}
	return 0;
}

bool UNarrativeComponent::HasCompletedTaskInQuest(TSubclassOf<class UQuest> QuestClass, const UNarrativeTask* Task, const FString& Name) const
{
	if (!Task || !IsValid(QuestClass))
	{
		return false;
	}

	const FString FormattedAction = Task->MakeTaskString(Name);

	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress && QuestInProgress->GetClass()->IsChildOf(QuestClass))
		{
			return QuestInProgress->QuestActivities.Contains(FormattedAction);
		}
	}

	return false;
}


void UNarrativeComponent::SendNarrativeUpdate(const FNarrativeUpdate& Update)
{
	PendingUpdateList.Add(Update);

	ensure(PendingUpdateList.Num());
	PendingUpdateList.Last().CreationTime = GetWorld()->GetTimeSeconds();

	//Remove stale updates from the pendingupdatelist, otherwise it might grow to be huge
	//TODO could keeping the update list history around be used to add more functionality? Its essentially a history of everything the player has done in order, could be valuable
	uint32 LatestStaleUpdateIdx = -1;
	bool bFoundStaleUpdates = false;
	const float UpdateStaleLimit = 30.f;

	if (PendingUpdateList.Num() > 1)
	{
		for (LatestStaleUpdateIdx = PendingUpdateList.Num() - 2; LatestStaleUpdateIdx > 0; --LatestStaleUpdateIdx)
		{
			if (PendingUpdateList.IsValidIndex(LatestStaleUpdateIdx) && GetWorld()->TimeSince(PendingUpdateList[LatestStaleUpdateIdx].CreationTime) > UpdateStaleLimit)
			{
				bFoundStaleUpdates = true;
				break;
			}
		}

		if (bFoundStaleUpdates)
		{
			PendingUpdateList.RemoveAt(0, LatestStaleUpdateIdx + 1);
			UE_LOG(LogNarrative, Verbose, TEXT("Found and removed %d stale updates. PendingUpdateList is now %d in size."), LatestStaleUpdateIdx + 1, PendingUpdateList.Num());
		}
	}
}

void UNarrativeComponent::OnRep_PendingUpdateList()
{
	//Process any updates the server has ran in the same order to ensure sync without having to replace a whole array of uquests 
	if (GetOwnerRole() < ROLE_Authority)
	{
		for (auto& Update : PendingUpdateList)
		{
			if (!Update.bAcked)
			{
				switch (Update.UpdateType)
				{
					case EUpdateType::UT_CompleteTask:
					{
						CompleteNarrativeTask_Internal(Update.Payload, true);
					}
					break;
					case EUpdateType::UT_BeginQuest:
					{
						BeginQuest(Update.QuestClass, FName(Update.Payload));
					}
					break;
					case EUpdateType::UT_RestartQuest:
					{
						RestartQuest(Update.QuestClass, FName(Update.Payload));
					}
					break;
					case EUpdateType::UT_ForgetQuest:
					{
						ForgetQuest(Update.QuestClass);
					}
					break;
				}

				Update.bAcked = true;
			}
		}

		//Clear the inputs out once we've processed them 
		PendingUpdateList.Empty();
	}
}

bool UNarrativeComponent::IsQuestValid(const UQuest* Quest, FString& OutError)
{
	
	if (!Quest)
	{
		OutError = "Narrative was given a null quest asset.";
		return false;
	}

	//This has already repped back to client and so if client is replaying narrative will complain about quest 
	//being started twice
	for (auto& QuestInProgress : QuestList)
	{
		if (QuestInProgress == Quest)
		{
			if (QuestInProgress->QuestCompletion == EQuestCompletion::QC_Succeded)
			{
				OutError = "Narrative was given a quest that has already been completed. Use RestartQuest to replay quests";
			}
			else if (QuestInProgress->QuestCompletion == EQuestCompletion::QC_Failed)
			{
				OutError = "Narrative was given a quest that has already been failed. Use RestartQuest to replay quests";
			}
			return false;
		}
	}

	TSet<FName> StateNames;
	for (auto State : Quest->States)
	{
		if (State->ID.IsNone())
		{
			OutError = FString::Printf(TEXT("Narrative was given a quest %s that has a state with no name. Please ensure all states are given names."), *Quest->GetQuestName().ToString());
			return false;
		}

		if (!StateNames.Contains(State->ID))
		{
			StateNames.Add(State->ID);
		}
		else
		{
			OutError = FString::Printf(TEXT("Narrative was given a quest %s that has more then one state with the same name. Please ensure all states have a unique name."), *Quest->GetQuestName().ToString());
			return false;
		}
	}
	return true;
}

void UNarrativeComponent::PerformAutosave()
{
	//Save quests and then queue the next autosave
	Save(AutoSaveName);
}

void UNarrativeComponent::QuestStarted(const UQuest* Quest)
{
	if (Quest)
	{
		if (bAutosaveOnQuestUpdated)
		{
			PerformAutosave();
		}

		UE_LOG(LogNarrative, Log, TEXT("Quest started: %s"), *GetNameSafe(Quest));
	}
}

void UNarrativeComponent::QuestForgotten(const UQuest* Quest)
{
	if (Quest)
	{
		if (bAutosaveOnQuestUpdated)
		{
			PerformAutosave();
		}

		UE_LOG(LogNarrative, Log, TEXT("Quest forgotten: %s"), *GetNameSafe(Quest));
	}
}

void UNarrativeComponent::QuestFailed(const UQuest* Quest, const FText& QuestFailedMessage)
{
	if (Quest)
	{

		if (bAutosaveOnQuestUpdated)
		{
			PerformAutosave();
		}


		UE_LOG(LogNarrative, Log, TEXT("Quest failed: %s. Failure state: %s"), *GetNameSafe(Quest), *QuestFailedMessage.ToString());
	}
}

void UNarrativeComponent::QuestSucceeded(const UQuest* Quest, const FText& QuestSucceededMessage)
{
	// No need to autosave on quest succeeded because QuestObjectiveCompleted already performs an autosave and is called on quest completion
	if (Quest)
	{
		UE_LOG(LogNarrative, Log, TEXT("Quest succeeded: %s. Succeeded state: %s"), *GetNameSafe(Quest), *QuestSucceededMessage.ToString());
	}
}

void UNarrativeComponent::QuestNewState(UQuest* Quest, const UQuestState* NewState)
{
	// No need to autosave on new objective because QuestObjectiveCompleted already performs an autosave and is called when we get a new objective
	if (Quest)
	{
		if (NewState)
		{
			UE_LOG(LogNarrative, Log, TEXT("Reached new state: %s in quest: %s"), *NewState->Description.ToString(), *GetNameSafe(Quest));

			APawn* OwningPawn = GetOwningPawn();

			for (auto& Event : NewState->Events)
			{
				if (Event)
				{
					Event->ExecuteEvent(OwnerPC->GetPawn(), OwnerPC, this);
				}
			}

			/**
			* Certain features like retroactive tasks require that we try update quests immediately, but networked games need to wait for input
			* because we need to be certain the client and server components sync properly.
			*/
			if (GetNetMode() == NM_Standalone)
			{
				UpdateQuest(Quest);
			}
		}
	}
}

void UNarrativeComponent::QuestTaskProgressMade(const UQuest* Quest, const FQuestTask& ProgressedObjective, const class UQuestBranch* Step, int32 CurrentProgress, int32 RequiredProgress)
{
	if (Quest)
	{
		if (bAutosaveOnQuestUpdated)
		{
			PerformAutosave();
		}
	}
}

void UNarrativeComponent::QuestTaskCompleted(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step)
{

}

void UNarrativeComponent::QuestStepCompleted(const UQuest* Quest, const class UQuestBranch* Step)
{

}

void UNarrativeComponent::BeginSave(FString SaveName)
{
	UE_LOG(LogNarrative, Verbose, TEXT("Begun saving using save name: %s"), *SaveName);
}

void UNarrativeComponent::BeginLoad(FString SaveName)
{
	UE_LOG(LogNarrative, Verbose, TEXT("Begun loading using save name: %s"), *SaveName);
}

void UNarrativeComponent::SaveComplete(FString SaveName)
{
	UE_LOG(LogNarrative, Verbose, TEXT("Save complete for save name: %s"), *SaveName);
}

void UNarrativeComponent::LoadComplete(FString SaveName)
{
	UE_LOG(LogNarrative, Verbose, TEXT("Load complete for save name: %s"), *SaveName);
}

void UNarrativeComponent::DialogueUpdated(UDialogue* Dialogue, const TArray<UDialogueNode_NPC*>& NPCReplies, const TArray<UDialogueNode_Player*>& PlayerReplies)
{

}

//Start/Stop dialogue sequence
void UNarrativeComponent::DialogueBegan(UDialogue* Dialogue)
{
	if (Dialogue->DialogueSequence)
	{
		PlayDialogueShot(Dialogue->DialogueSequence, Dialogue->DialogueSequenceSettings);
	}
}

void UNarrativeComponent::DialogueFinished(UDialogue* Dialogue)
{
	StopDialogueSequence();
}

UDialogue* UNarrativeComponent::NewDialogueFromAsset(class UDialogueAsset* DialogueAsset, class AActor* NPC, const FText& NPCName)
{
	if (DialogueAsset)
	{
		//Duplicate the dialogue from its template and set required data on it 
		UDialogue* Dialogue = DuplicateObject<UDialogue>(DialogueAsset->Dialogue, this);

		Dialogue->Initialize(NPC, NPCName);

		return Dialogue;
	}

	return nullptr;
}

APawn* UNarrativeComponent::GetOwningPawn() const
{

	if (OwnerPC)
	{
		return OwnerPC->GetPawn();
	}

	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (OwningPawn)
	{
		return OwningPawn;
	}

	if (!OwningPawn && OwningController)
	{
		return OwningController->GetPawn();
	}

	return nullptr;
}

APlayerController* UNarrativeComponent::GetOwningController() const
{
	//We cache this on beginplay as to not re-find it every time 
	if (OwnerPC)
	{
		return OwnerPC;
	}

	APlayerController* OwningController = Cast<APlayerController>(GetOwner());
	APawn* OwningPawn = Cast<APawn>(GetOwner());

	if (OwningController)
	{
		return OwningController;
	}

	if (!OwningController && OwningPawn)
	{
		return Cast<APlayerController>(OwningPawn->GetController());
	}

	return nullptr;
}

TArray<UQuest*> UNarrativeComponent::GetFailedQuests() const
{
	TArray<UQuest*> FailedQuests;
	for (auto QIP : QuestList)
	{
		if (QIP->QuestCompletion == EQuestCompletion::QC_Failed)
		{
			FailedQuests.Add(QIP);
		}
	}
	return FailedQuests;
}


TArray<UQuest*> UNarrativeComponent::GetSucceededQuests() const
{
	TArray<UQuest*> SucceededQuests;
	for (auto QIP : QuestList)
	{
		if (QIP->QuestCompletion == EQuestCompletion::QC_Succeded)
		{
			SucceededQuests.Add(QIP);
		}
	}
	return SucceededQuests;
}

TArray<UQuest*> UNarrativeComponent::GetInProgressQuests() const
{
	TArray<UQuest*> InProgressQuests;
	for (auto QIP : QuestList)
	{
		if (QIP->QuestCompletion == EQuestCompletion::QC_Started)
		{
			InProgressQuests.Add(QIP);
		}
	}
	return InProgressQuests;
}

TArray<UQuest*> UNarrativeComponent::GetAllQuests() const
{
	return QuestList;
}

class UQuest* UNarrativeComponent::GetQuest(TSubclassOf<class UQuest> QuestClass) const
{
	for (auto& QIP : QuestList)
	{
		if (QIP && QIP->GetClass()->IsChildOf(QuestClass))
		{
			return QIP;
		}
	}
	return nullptr;
}

bool UNarrativeComponent::Save(const FString& SaveName/** = "NarrativeSaveData"*/, const int32 Slot/** = 0*/)
{
	OnBeginSave.Broadcast(SaveName);
	if (UNarrativeSaveGame* NarrativeSaveGame = Cast<UNarrativeSaveGame>(UGameplayStatics::CreateSaveGameObject(UNarrativeSaveGame::StaticClass())))
	{
		NarrativeSaveGame->MasterTaskList = MasterTaskList;

		for (auto& Quest : QuestList)
		{
			FNarrativeSavedQuest Save;
			Save.QuestClass = Quest->GetClass();
			Save.CurrentStateID = Quest->GetCurrentState()->ID;

			//Store all the branches in the save file 
			for (UQuestBranch* Branch : Quest->Branches)
			{
				Save.Branches.Add(Branch->ID, FSavedQuestBranch(Branch->Tasks));
			}

			//Store all the reached states in the save file
			for (UQuestState* State : Quest->ReachedStates)
			{
				Save.ReachedStateNames.Add(State->ID);
			}

			NarrativeSaveGame->SavedQuests.Add(Save);
		}

		if (UGameplayStatics::SaveGameToSlot(NarrativeSaveGame, SaveName, Slot))
		{
			OnSaveComplete.Broadcast(SaveName);
			return true;
		}
	}
	return false;
}

bool UNarrativeComponent::Load(const FString& SaveName/** = "NarrativeSaveData"*/, const int32 Slot/** = 0*/)
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveName, 0))
	{
		return false;
	}

	OnBeginLoad.Broadcast(SaveName);

	if (UNarrativeSaveGame* NarrativeSaveGame = Cast<UNarrativeSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveName, Slot)))
	{
		for (auto& SaveQuest : NarrativeSaveGame->SavedQuests)
		{
			//Restore the quest from the last state we were in 
			if (UQuest* BegunQuest = BeginQuest_Internal(SaveQuest.QuestClass, SaveQuest.CurrentStateID))
			{
				//Restore all quest branches
				if (UQuestState* CurrentState = BegunQuest->GetCurrentState())
				{
					for (auto& Branch : CurrentState->Branches)
					{
						if (SaveQuest.Branches.Contains(Branch->ID))
						{
							FSavedQuestBranch SavedBranch = *SaveQuest.Branches.Find(Branch->ID);

							Branch->Tasks = SavedBranch.Tasks;
						}
					}
				}
				
				//Restore all reached states
				TArray<UQuestState*> AllReachedStates;

				//Remove the last state, it will have already been added by BeginQuest_Internal
				BegunQuest->ReachedStates.Empty();

				for (const FName StateName : SaveQuest.ReachedStateNames)
				{
					BegunQuest->ReachedStates.Add(BegunQuest->GetState(StateName));
				}
			}
		}

		MasterTaskList = NarrativeSaveGame->MasterTaskList;
		OnLoadComplete.Broadcast(SaveName);
		return true;
	}
	return false;
}

bool UNarrativeComponent::DeleteSave(const FString& SaveName /*= "NarrativeSaveData"*/, const int32 Slot/** = 0*/)
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveName, 0))
	{
		return false;
	}

	return UGameplayStatics::DeleteGameInSlot(SaveName, Slot);
}

void UNarrativeComponent::PlayDialogueShot(class ULevelSequence* Shot, const FMovieSceneSequencePlaybackSettings& Settings)
{
	if (Shot && OwnerPC && OwnerPC->IsLocalPlayerController())
	{
		//Stop any currently running sequence 
		StopDialogueSequence();

		//Narrative needs to initialize its cutscene player 
		if (!CurrentDialogue.DialogueSequencePlayer)
		{
			ULevelSequencePlayer::CreateLevelSequencePlayer(GetWorld(), Shot, Settings, CurrentDialogue.DialogueSequencePlayer);
		}
		else // Cutscene player already exists, but we need to re-apply settings 
		{
			if (CurrentDialogue.DialogueSequencePlayer && CurrentDialogue.DialogueSequencePlayer->SequencePlayer)
			{
				FLevelSequenceCameraSettings CamSettings;
				CurrentDialogue.DialogueSequencePlayer->SequencePlayer->Initialize(Shot, OwnerPC->GetLevel(), Settings, CamSettings);
			}
		}

		if(CurrentDialogue.DialogueSequencePlayer)
		{
			check(CurrentDialogue.DialogueSequencePlayer && CurrentDialogue.DialogueSequencePlayer->SequencePlayer);

			if (CurrentDialogue.DialogueSequencePlayer && CurrentDialogue.DialogueSequencePlayer->SequencePlayer)
			{
				CurrentDialogue.DialogueSequencePlayer->SequencePlayer->Play();
			}
		}

	}
}

void UNarrativeComponent::StopDialogueSequence()
{
	if (OwnerPC && OwnerPC->IsLocalPlayerController())
	{
		if (CurrentDialogue.DialogueSequencePlayer && CurrentDialogue.DialogueSequencePlayer->SequencePlayer)
		{
			CurrentDialogue.DialogueSequencePlayer->SequencePlayer->Stop();
		}
	}
}

