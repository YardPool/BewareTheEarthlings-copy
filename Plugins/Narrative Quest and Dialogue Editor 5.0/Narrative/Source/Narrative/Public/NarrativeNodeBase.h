// SurvivalGame Project - The Unreal C++ Survival Game Course - Copyright Reuben Ward 2020

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "NarrativeNodeBase.generated.h"

/**
 * The base class for all narrative nodes in eiher a quest state machine, or a dialogue tree 
 */
UCLASS()
class NARRATIVE_API UNarrativeNodeBase : public UObject
{
	GENERATED_BODY()

public:

	UNarrativeNodeBase();

	/**An optional ID for this node, can be left empty*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Details")
	FName ID;

	//Execute all the events on this quest/dialogue node - this is blueprint callable so dialogue UI can call this
	//when a piece of dialogue is spoken
	UFUNCTION(BlueprintCallable, Category = "Events & Conditions")
	void ProcessEvents(APawn* Pawn, APlayerController* Controller, class UNarrativeComponent* NarrativeComponent);

	//Check if all the conditions are met on this quest/dialogue node
	UFUNCTION(BlueprintCallable, Category = "Events & Conditions")
	bool AreConditionsMet(APawn* Pawn, APlayerController* Controller, class UNarrativeComponent* NarrativeComponent);

	/**This only appears if the following conditions are met*/
	UPROPERTY(EditAnywhere, Instanced, Category = "Events & Conditions")
	TArray<class UNarrativeCondition*> Conditions;

	/**Events that should fire when this is reached*/
	UPROPERTY(EditAnywhere, Instanced, Category = "Events & Conditions")
	TArray<class UNarrativeEvent*> Events;
	
};
