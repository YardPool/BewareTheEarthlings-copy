// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NarrativeCondition.generated.h"

/**
 * Subclass this in blueprint to conditionally include or exclude dialogue/quest options 
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class NARRATIVE_API UNarrativeCondition : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	bool CheckCondition(APawn* Pawn, APlayerController* Controller, class UNarrativeComponent* NarrativeComponent);

	UFUNCTION(BlueprintImplementableEvent)
	FString GetGraphDisplayText();

	//Set this to true to flip the result of this condition
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
	bool bNot;

};
