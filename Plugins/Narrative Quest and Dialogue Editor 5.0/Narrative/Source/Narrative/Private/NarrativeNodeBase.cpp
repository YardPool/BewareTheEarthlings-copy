// SurvivalGame Project - The Unreal C++ Survival Game Course - Copyright Reuben Ward 2020


#include "NarrativeNodeBase.h"
#include "NarrativeCondition.h"
#include "NarrativeEvent.h"

UNarrativeNodeBase::UNarrativeNodeBase()
{
	//autofill the ID
	ID = GetFName();
}

void UNarrativeNodeBase::ProcessEvents(APawn* Pawn, APlayerController* Controller, class UNarrativeComponent* NarrativeComponent)
{
	for (auto& Event : Events)
	{
		if (Event)
		{
			Event->ExecuteEvent(Pawn, Controller, NarrativeComponent);
		}
	}
}

bool UNarrativeNodeBase::AreConditionsMet(APawn* Pawn, APlayerController* Controller, class UNarrativeComponent* NarrativeComponent)
{
	//Ensure all conditions are met
	for (auto& Cond : Conditions)
	{	
		if (Cond && Cond->CheckCondition(Pawn, Controller, NarrativeComponent) == Cond->bNot)
		{
			return false;
		}
	}

	return true;
}
