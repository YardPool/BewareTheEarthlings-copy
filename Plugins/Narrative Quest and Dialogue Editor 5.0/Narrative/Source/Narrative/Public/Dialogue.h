// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LevelSequencePlayer.h"
#include "Dialogue.generated.h"

//Created at runtime, but also used as a template, similar to UWidgetTrees in UWidgetBlueprints. 
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NARRATIVE_API UDialogue : public UObject
{
	GENERATED_BODY()

public:

	UDialogue();

	virtual void Initialize(class AActor* NPC, const FText& NPCName);

	UPROPERTY(EditAnywhere, Category = "Dialogue Details")
	FText DialogueName;

	//Optional name of the person that speaks this dialogue. Can be left empty if multiple different NPCs share this dialogue.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Details")
	FText OptionalNPCName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue Details", meta = (MultiLine = true))
	FText DialogueDescription;

	//Will automatically run this cinematic when your cutscene is playing. 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	class ULevelSequence* DialogueSequence;

	//Playback settings for the cutscene
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	struct FMovieSceneSequencePlaybackSettings DialogueSequenceSettings;

	//A sequence to play whilst the NPC is talking
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	class ULevelSequence* NPCTalkingShot;

	//Playback settings for when the NPC is talking
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	struct FMovieSceneSequencePlaybackSettings NPCTalkingSettings;

	//A sequence to play whilst the player is talking
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	class ULevelSequence* PlayerTalkingShot;

	//Playback settings for when the NPC is talking
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	struct FMovieSceneSequencePlaybackSettings PlayerTalkingSettings;

	//A sequence to play whilst the NPC is waiting for the player to select his reply 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	class ULevelSequence* SelectReplyShot;

	//Playback settings for when the NPC is Waiting
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	struct FMovieSceneSequencePlaybackSettings SelectReplySettings;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	class UNarrativeComponent* NarrativeComp;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	FText NPCName;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	class AActor* NPCActor;

	//The first thing the NPC says in the dialog
	UPROPERTY()
	class UDialogueNode_NPC* RootDialogue;

	//Holds all of the npc replies in the dialogue
	UPROPERTY()
	TArray<class UDialogueNode_NPC*> NPCReplies;

	//Holds all of the player replies in the dialogue
	UPROPERTY()
	TArray<class UDialogueNode_Player*> PlayerReplies;

};