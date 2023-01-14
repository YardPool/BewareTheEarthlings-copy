// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "Quest.h"
#include "GameFramework/SaveGame.h"
#include "Templates/SubclassOf.h"
#include "NarrativeSaveGame.generated.h"

USTRUCT()
struct NARRATIVE_API FSavedQuestBranch
{
	GENERATED_BODY()

	FSavedQuestBranch(){};
	FSavedQuestBranch(const TArray<FQuestTask>& InTasks) : Tasks(InTasks){};

	//All we need to save a branch is remember what progress the tasks had 
	UPROPERTY(SaveGame)
	TArray<FQuestTask> Tasks;
};

USTRUCT()
struct NARRATIVE_API FNarrativeSavedQuest
{
	GENERATED_BODY()

public:

	UPROPERTY(SaveGame)
	TSubclassOf<class UQuest> QuestClass;

	UPROPERTY(SaveGame)
	FName CurrentStateID;

	//We also save all the branches tasks by mapping BranchID->Task and then restoring these 
	UPROPERTY(SaveGame)
	TMap<FName, FSavedQuestBranch> Branches;

	UPROPERTY(SaveGame)
	TArray<FName> ReachedStateNames;

};

/**
 * Narrative Savegame object 
 */
UCLASS()
class NARRATIVE_API UNarrativeSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, Category = "Save")
	TArray<FNarrativeSavedQuest> SavedQuests;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	TMap<FString, int32> MasterTaskList;

};
