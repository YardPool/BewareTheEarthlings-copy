// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraphNode.h"
#include "DialogueGraphNode_Player.generated.h"

/**
 * 
 */
UCLASS()
class UDialogueGraphNode_Player : public UDialogueGraphNode
{
	GENERATED_BODY()
	
public:

	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetGraphNodeColor() const { return FLinearColor(0.65f, 0.28f, 0.f); };

};
