// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueAsset.generated.h"

class UDialogue;

/**
 * 
 */
UCLASS(BlueprintType)
class NARRATIVE_API UDialogueAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UDialogueAsset();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue")
	FText GetDialogueName() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Dialogue")
	FText GetDialogueDescription() const;

	UPROPERTY(EditAnywhere, Instanced, Category = "Dialogue")
	UDialogue* Dialogue;

#if WITH_EDITORONLY_DATA

		/** Graph for quest asset */
		UPROPERTY()
		class UEdGraph*	DialogueGraph;

	/** Info about the graphs we last edited */
	UPROPERTY()
		TArray<FEditedDocumentInfo> LastEditedDocuments;

#endif


};
