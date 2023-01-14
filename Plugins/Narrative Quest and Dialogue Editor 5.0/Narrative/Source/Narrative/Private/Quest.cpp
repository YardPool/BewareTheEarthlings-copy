// Copyright Narrative Tools 2022. 

#include "Quest.h"
#include "NarrativeTask.h"
#include "QuestSM.h"
#include "Net/UnrealNetwork.h"
#include "QuestBlueprintGeneratedClass.h"
#include "NarrativeComponent.h"

UQuest::UQuest()
{
	QuestName = FText::FromString("My New Quest");
	QuestDescription = FText::FromString("Enter a description for your quest here.");
}

class UNarrativeComponent* UQuest::GetOwningNarrativeComponent() const
{
	return OwningComp;
}

class APawn* UQuest::GetPawnOwner() const
{
	return OwningComp ? OwningComp->GetOwningPawn() : nullptr;
}

UWorld* UQuest::GetWorld() const
{
	return (HasAnyFlags(RF_ClassDefaultObject) ? nullptr : GetOwningNarrativeComponent()->GetWorld());
}

void UQuest::DuplicateAndInitializeFromQuest(UQuest* QuestTemplate)
{
	if (QuestTemplate)
	{
		//Duplicate the quest template, then steal all its states and branches
		UQuest* NewQuest = Cast<UQuest>(StaticDuplicateObject(QuestTemplate, this, NAME_None, RF_Transactional));
		NewQuest->SetFlags(RF_Transient | RF_DuplicateTransient);

		if (NewQuest)
		{
			QuestStartState = NewQuest->QuestStartState;
			States = NewQuest->States;
			Branches = NewQuest->Branches;

			//Change the outer of the duplicated states and branches to this quest instead of the template one.
			//TODO look into whether this is safe, should we possibly duplicating each state and branch individually or has StaticDuplicateObject made deep copies? 
			for (auto& State : States)
			{
				if (State)
				{
					State->Rename(nullptr, this);
				}
			}

			for (auto& Branch : Branches)
			{
				if (Branch)
				{
					Branch->Rename(nullptr, this);

					//Cull any extra tasks that were added if this branch only has a single task
					if (!Branch->bAddMultipleTasks)
					{
						Branch->Tasks.SetNum(1);
					}
				}
			}
		}
	}
}

bool UQuest::Initialize(class UNarrativeComponent* InitializingComp, const FName& QuestStartID /*= NAME_None*/)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		if (UQuestBlueprintGeneratedClass* BGClass = Cast<UQuestBlueprintGeneratedClass>(GetClass()))
		{
			BGClass->InitializeQuest(this);

			//At this point, we should have a valid quest assigned to us. Check if we have a valid start state
			if (QuestStartState)
			{
				OwningComp = InitializingComp;

				if (InitializingComp)
				{
					InitializingComp->OnQuestStarted.AddUniqueDynamic(this, &UQuest::OnQuestStarted);
					InitializingComp->OnQuestFailed.AddUniqueDynamic(this, &UQuest::OnQuestFailed);
					InitializingComp->OnQuestSucceeded.AddUniqueDynamic(this, &UQuest::OnQuestSucceeded);
					InitializingComp->OnQuestStepCompleted.AddUniqueDynamic(this, &UQuest::OnQuestStepCompleted);
					InitializingComp->OnQuestNewState.AddUniqueDynamic(this, &UQuest::OnQuestNewState);
					InitializingComp->OnQuestTaskProgressMade.AddUniqueDynamic(this, &UQuest::OnQuestTaskProgressMade);
					InitializingComp->OnBeginSave.AddUniqueDynamic(this, &UQuest::OnBeginSave);
					InitializingComp->OnBeginLoad.AddUniqueDynamic(this, &UQuest::OnBeginLoad);
					InitializingComp->OnSaveComplete.AddUniqueDynamic(this, &UQuest::OnSaveComplete);
					InitializingComp->OnLoadComplete.AddUniqueDynamic(this, &UQuest::OnLoadComplete);
				}

				QuestCompletion = EQuestCompletion::QC_Started;
				EnterState(QuestStartID.IsNone() ? QuestStartState : GetState(QuestStartID));
				return true;
			}
		}
	}
	return false;
}

EQuestProgress UQuest::UpdateQuest(UNarrativeComponent* NarrativeComp, const FString& EventString /*=""*/)
{

	if (!CurrentState)
	{
		check(false); // shouldnt happen
		return EQuestProgress::QP_NoChange;
	}

	//No need to update this quest 
	if (CanSkipUpdate(EventString))
	{
		return EQuestProgress::QP_NoChange;
	}

	if (QuestCompletion == EQuestCompletion::QC_Started)
	{
		FStateMachineResult QuestResult;
		QuestActivities.Add(EventString);

		bool bMadeProgress = false;
		bool bChangedState = false;

		QuestResult = CurrentState->RunState(bMadeProgress, EventString);

		while (QuestResult.FinalState != GetCurrentState())
		{
			//We're now on a new state!
			EnterState(QuestResult.FinalState);
			PreviousBranch = QuestResult.Branch;
			bChangedState = true;

			//Run the state again, we may have already completed a branch on it even though we just entered it 
			QuestResult = CurrentState->RunState(bMadeProgress, EventString);
		}

		//Check if our event took us to a new state 
		if (bChangedState)
		{
			switch (QuestResult.CompletionType)
			{
				//We reached the end state of a quest and succeeded it
				case EStateMachineCompletionType::Accepted:
				{
					return EQuestProgress::QP_Succeeded;
				}
				//We reached the end state of a quest and failed it
				case EStateMachineCompletionType::Rejected:
				{
					return EQuestProgress::QP_Failed;
				}
				//We reached a new state in the quest
				case EStateMachineCompletionType::NotAccepted:
				{
					return EQuestProgress::QP_Updated;
				}
			}
		}
		else if (bMadeProgress)//We didnt reach a new state on the quest, but we made progress on a step
		{
			return EQuestProgress::QP_MadeProgress;
		}

	}

	//The action didn't update any quests
	return EQuestProgress::QP_NoChange;
}

bool UQuest::CanSkipUpdate(const FString& EventString) const
{
	return false;
}

void UQuest::EnterState(UQuestState* NewState)
{
	if (NewState)
	{
		CurrentState = NewState;
		ReachedStates.Add(CurrentState);

		//Update the quests completion
		if (NewState->CompletionType == EStateMachineCompletionType::Accepted)
		{
			QuestCompletion = EQuestCompletion::QC_Succeded;
		}
		else if (NewState->CompletionType == EStateMachineCompletionType::Rejected)
		{
			QuestCompletion = EQuestCompletion::QC_Failed;
		}

		if (OwningComp)
		{
			OwningComp->OnQuestNewState.Broadcast(this, CurrentState);
		}

		//We're in a new state. Tell the branches they have been reached, they can then check if we've completed them. 
		for (auto& Branch : CurrentState->Branches)
		{
			if (Branch)
			{
				Branch->OnReached();
			}
		}
	}
}

class UQuestState* UQuest::GetState(const FName& ID) const
{
	for (auto& State : States)
	{
		if (State->ID == ID)
		{
			return State;
		}
	}
	return nullptr;
}

FText UQuest::GetQuestName() const
{
	return QuestName;
}

FText UQuest::GetQuestDescription() const
{
	return QuestDescription;
}

void UQuest::OnQuestStarted(const UQuest* Quest)
{
	BPOnQuestStarted(Quest);
}

void UQuest::OnQuestFailed(const UQuest* Quest, const FText& QuestFailedMessage)
{
	BPOnQuestFailed(Quest, QuestFailedMessage);
}

void UQuest::OnQuestSucceeded(const UQuest* Quest, const FText& QuestSucceededMessage)
{
	BPOnQuestSucceeded(Quest, QuestSucceededMessage);
}

void UQuest::OnQuestNewState( UQuest* Quest, const UQuestState* NewState)
{
	BPOnQuestNewState(Quest, NewState);
}

void UQuest::OnQuestTaskProgressMade(const UQuest* Quest, const FQuestTask& Task, const class UQuestBranch* Step, int32 CurrentProgress, int32 RequiredProgress)
{
	BPOnQuestTaskProgressMade(Quest, Task, Step, CurrentProgress, RequiredProgress);
}

void UQuest::OnQuestStepCompleted(const UQuest* Quest, const class UQuestBranch* Step)
{
	BPOnQuestStepCompleted(Quest, Step);
}

void UQuest::OnBeginSave(FString SaveName)
{
	BPOnBeginSave(SaveName);
}

void UQuest::OnBeginLoad(FString SaveName)
{
	BPOnBeginLoad(SaveName);
}

void UQuest::OnSaveComplete(FString SaveName)
{
	BPOnSaveComplete(SaveName);
}

void UQuest::OnLoadComplete(FString SaveName)
{
	BPOnLoadComplete(SaveName);
}
