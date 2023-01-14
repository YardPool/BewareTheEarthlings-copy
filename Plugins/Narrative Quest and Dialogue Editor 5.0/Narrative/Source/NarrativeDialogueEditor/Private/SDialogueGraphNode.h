// Copyright Narrative Tools 2022. 

#pragma once

#include "CoreMinimal.h"
#include "Layout/Visibility.h"
#include "Styling/SlateColor.h"
#include "Input/DragAndDrop.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SWidget.h"
#include "DialogueGraphNode.h"
#include "SGraphNode.h"
#include "SGraphPin.h"

class NARRATIVEDIALOGUEEDITOR_API SDialogueGraphNode : public SGraphNode
{
public:

	SLATE_BEGIN_ARGS(SDialogueGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UDialogueGraphNode* InNode);

	//~ Begin SGraphNode Interface
	virtual void CreatePinWidgets() override;
	virtual void UpdateGraphNode() override;
	virtual TSharedPtr<SToolTip> GetComplexTooltip() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;
	//~ End SGraphNode Interface

	/** handle mouse down on the node */
	FReply OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent);

protected:

	FText GetNodeText() const;
	FText GetEventsText() const;
	FText GetNodeTitleText() const;
	FSlateColor GetNodeTitleColor() const;
	FText GetConditionsText() const;
	EVisibility GetNodeTextVisibility() const;
	EVisibility GetCurrentStatePointerVisibility() const;
	FSlateColor GetBorderColor() const;

};

class SDialogueGraphNodePin : public SGraphPin 
{

public:

	SLATE_BEGIN_ARGS(SDialogueGraphNodePin) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin);

protected:

	//~ Begin SGraphPin Interface
	virtual FSlateColor GetPinColor() const override;
	virtual TSharedRef<SWidget>	GetDefaultValueWidget() override;
	//~ End SGraphPin Interface

	const FSlateBrush* GetPinBorder() const;


};
