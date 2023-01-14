// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "QuestGraphNode.h"
#include "QuestGraphNode_Action.generated.h"

class UQuestBranch;

/**
 * 
 */
UCLASS()
class UQuestGraphNode_Action : public UQuestGraphNode
{
	GENERATED_BODY()
	
public:

	virtual void AllocateDefaultPins() override;
	virtual void DestroyNode() override;

	virtual FLinearColor GetGraphNodeColor() const override;

	UPROPERTY(EditAnywhere, Category = "Node")
	UQuestBranch* Branch;
};
