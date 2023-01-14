// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "NarrativeNodeBase.h"
#include <MovieSceneSequencePlayer.h>
#include "DialogueSM.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueNodeFinishedPlaying);

/**Base class for states and branches in the Dialogues state machine*/
 UCLASS(BlueprintType, Blueprintable)
 class NARRATIVE_API UDialogueNode : public UNarrativeNodeBase
 {

	 GENERATED_BODY()

 public:

	//A shot to play whilst this dialogue option is playing. Will override whatever NPC/player shot was selected in the dialogue 
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	class ULevelSequence* Shot;

	//Playback settings for the cutscene
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cinematics")
	struct FMovieSceneSequencePlaybackSettings ShotSettings;

	/**The text associated with this dialogue node ie "Hello adventurer, what can I do for you?"*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Details", meta = (MultiLine = true))
	FText Text;

	/**The sound the NPC/Player makes when this dialogue is selected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Details")
	class USoundBase* DialogueSound;

	/**An optional anim montage for the character to play whilst speaking their dialogue. "*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Details")
	class UAnimMontage* DialogueMontage;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue")
	FOnDialogueNodeFinishedPlaying OnDialogueFinished;

	/**We cache the graph node position so we can process conditions in order of the graph nodes Y position*/
	UPROPERTY()
	FVector2D NodePos;

	UPROPERTY()
	TArray<class UDialogueNode_NPC*> NPCReplies;

	UPROPERTY()
	TArray<class UDialogueNode_Player*> PlayerReplies;
	
	UPROPERTY()
	class UDialogue* OwningDialogue;

	UPROPERTY()
	class UNarrativeComponent* OwningComponent;

	TArray<class UDialogueNode_NPC*> GetNPCReplies(APlayerController* OwningController, APawn* OwningPawn, class UNarrativeComponent* NarrativeComponent);
	TArray<class UDialogueNode_Player*> GetPlayerReplies(APlayerController* OwningController, APawn* OwningPawn, class UNarrativeComponent* NarrativeComponent);

	virtual UWorld* GetWorld() const;

	//The text this dialogue should display on its Graph Node
	virtual FText GetNodeTitleText() const { return FText::FromString("Node"); };
	virtual FText GetNodeText() const { return Text; };

	/**Play this dialogue node. NPC and player dialogue nodes both override this
	as NPC and player dialogues should be handled different. Make sure to call FinishPlay() when dialogue is done if overridding */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue")
	void Play(class UNarrativeComponent* OwningComp);
	virtual void Play_Implementation(class UNarrativeComponent* OwningComp);
	
	//Functions overriding play must call this when the dialogue is finished playing! 
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	virtual void FinishPlay();

 };

UCLASS(BlueprintType)
class NARRATIVE_API UDialogueNode_NPC : public UDialogueNode
{
	GENERATED_BODY()

public:

	virtual FText GetNodeTitleText() const;

	/**Grab this NPC node, appending all follow up responses to that node. Since multiple NPC replies can be linked together, 
	we need to grab the chain of replies the NPC has to say. */
	TArray<class UDialogueNode_NPC*> GetReplyChain(APlayerController* OwningController, APawn* OwningPawn, class UNarrativeComponent* NarrativeComponent);

	virtual void Play_Implementation(class UNarrativeComponent* OwningComp) override;
	virtual void FinishPlay() override;

};

UCLASS(BlueprintType)
class NARRATIVE_API UDialogueNode_Player : public UDialogueNode
{
	GENERATED_BODY()

public:

	virtual FText GetNodeTitleText() const { return FText::FromString("Player"); };

	virtual void Play_Implementation(class UNarrativeComponent* OwningComp) override;

};