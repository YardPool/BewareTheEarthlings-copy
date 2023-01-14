// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

/** Quest editor public interface */
class IDialogueEditor : public FWorkflowCentricApplication
{
public: 

	virtual uint32 GetSelectedNodesCount() const = 0;
	virtual bool GetBoundsForSelectedNodes(class FSlateRect& Rect, float Padding) const = 0;
};