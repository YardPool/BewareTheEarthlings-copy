// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NarrativeEvent.generated.h"

/**
 * Subclass this in BP to execute some logic when a particular quest step or dialogue option is reached  */
UCLASS(Abstract, Blueprintable, EditInlineNew, AutoExpandCategories = ("Default"))
class NARRATIVE_API UNarrativeEvent : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintImplementableEvent)
	bool ExecuteEvent(APawn* Pawn, APlayerController* Controller, class UNarrativeComponent* NarrativeComponent);

	UFUNCTION(BlueprintImplementableEvent)
	FString GetGraphDisplayText();
};
