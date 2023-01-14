// Copyright Narrative Tools 2022. 

#include "SDialogueGraphNode.h"
#include "DialogueGraphNode.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Editor.h"
#include "DialogueGraph.h"
#include "SGraphPanel.h"
#include "ScopedTransaction.h"
#include "DialogueSM.h"
#include "Dialogue.h"
#include "DialogueEditorStyle.h"
#include "NarrativeCondition.h"
#include "NarrativeEvent.h"

#define LOCTEXT_NAMESPACE "SDialogueGraphNode"

void SDialogueGraphNode::Construct(const FArguments& InArgs, UDialogueGraphNode* InNode)
{
	GraphNode = InNode;

	UpdateGraphNode();
}

void SDialogueGraphNode::CreatePinWidgets()
{
	UDialogueGraphNode* DialogueGraphNode = CastChecked<UDialogueGraphNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < DialogueGraphNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = DialogueGraphNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SGraphPin, MyPin);

			AddPin(NewPin.ToSharedRef());
		}
	}
}

void SDialogueGraphNode::UpdateGraphNode()
{

	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	auto SizeBox = SNew(SBox);
	SizeBox->SetMinDesiredWidth(250.f);
	SizeBox->SetMinDesiredHeight(60.f);

	this->GetOrAddSlot(ENodeZone::Center)
.HAlign(HAlign_Fill)
.VAlign(VAlign_Fill)
[
	SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
	.BorderBackgroundColor(this, &SDialogueGraphNode::GetBorderColor)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SizeBox
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(5.f)
			[
				
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredHeight(10.f)
					[
						SAssignNew(LeftNodeBox, SVerticalBox)
					]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.FillWidth(1.f)
				[

					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Top)
					.HAlign(HAlign_Fill)
					.AutoHeight()
					[
					SNew(STextBlock)
						.Text(this, &SDialogueGraphNode::GetNodeTitleText)
						.ColorAndOpacity(this, &SDialogueGraphNode::GetNodeTitleColor)
						.Justification(ETextJustify::Center)
						.Clipping(EWidgetClipping::Inherit)
						.TextStyle(FEditorStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
					]
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.FillHeight(1.f)
					[
						SNew(STextBlock)
						.Text(this, &SDialogueGraphNode::GetNodeText)
						.Justification(ETextJustify::Center)
						.WrapTextAt(200.f)
					]
					+ SVerticalBox::Slot()
					.VAlign(VAlign_Bottom)
					.HAlign(HAlign_Fill)
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(this, &SDialogueGraphNode::GetEventsText)
						.Justification(ETextJustify::Center)
						.AutoWrapText(true)
					]
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Right)
				.AutoWidth()
				[
					SNew(SBox)
					.MinDesiredHeight(10.f)
				[
					SAssignNew(RightNodeBox, SVerticalBox)
				]
				]
			]
			]
];

	CreatePinWidgets();
}

TSharedPtr<SToolTip> SDialogueGraphNode::GetComplexTooltip()
{
	return nullptr;
}

void SDialogueGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));


	const UEdGraphPin* PinObj = PinToAdd->GetPinObj();

	if (PinToAdd->GetDirection() == EEdGraphPinDirection::EGPD_Input)
	{
		LeftNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			.Padding(5.f, 0.f)
			[
				PinToAdd
			];
		InputPins.Add(PinToAdd);
	}
	else
	{
		RightNodeBox->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.FillHeight(1.0f)
			[
				PinToAdd
			];
		OutputPins.Add(PinToAdd);
	}

	

}

FReply SDialogueGraphNode::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FText SDialogueGraphNode::GetNodeText() const
{
	if (UDialogueGraphNode* Node = CastChecked<UDialogueGraphNode>(GraphNode))
	{
		if (Node->DialogueNode)
		{
			return Node->DialogueNode->Text;
		}
	}

	return FText::GetEmpty();
}

FText SDialogueGraphNode::GetEventsText() const
{
	FString EvtText = "";

	if (UDialogueGraphNode* DialogueGraphNode = Cast<UDialogueGraphNode>(GraphNode))
	{
		if (DialogueGraphNode->DialogueNode)
		{

			if (DialogueGraphNode->DialogueNode->Events.Num())
			{
				int32 Idx = 0;
				for (auto& Evt : DialogueGraphNode->DialogueNode->Events)
				{
					if (Evt)
					{
						//Only append newline if we have more than one Evtition
						if (Idx == 0)
						{
							EvtText += Evt->GetGraphDisplayText();
						}
						else
						{
							EvtText += LINE_TERMINATOR + Evt->GetGraphDisplayText();
						}

						Idx++;
					}
				}
			}
		}
	}

	return FText::FromString(EvtText);
}

FText SDialogueGraphNode::GetNodeTitleText() const
{
	if (UDialogueGraphNode* DialogueGraphNode = Cast<UDialogueGraphNode>(GraphNode))
	{
		if (DialogueGraphNode->DialogueNode)
		{
			FText CondText = GetConditionsText();

			return CondText.IsEmpty() ? DialogueGraphNode->DialogueNode->GetNodeTitleText() : CondText;
		}
	}

	return FText::GetEmpty();
}

FSlateColor SDialogueGraphNode::GetNodeTitleColor() const
{
	if (UDialogueGraphNode* DialogueGraphNode = Cast<UDialogueGraphNode>(GraphNode))
	{
		if (DialogueGraphNode->DialogueNode->Conditions.Num())
		{
			return FSlateColor(FColorList::Goldenrod);
		}
	}
	return  FSlateColor(FColor::White);
}

FText SDialogueGraphNode::GetConditionsText() const
{
	if (UDialogueGraphNode* DialogueGraphNode = Cast<UDialogueGraphNode>(GraphNode))
	{
		if (DialogueGraphNode->DialogueNode)
		{
			FString CondText = "";

			if (DialogueGraphNode->DialogueNode->Conditions.Num())
			{
				int32 Idx = 0;
				for (auto& Cond : DialogueGraphNode->DialogueNode->Conditions)
				{
					if (Cond)
					{
						//Only append newline if we have more than one condition
						if (Idx == 0)
						{
							CondText += Cond->GetGraphDisplayText();
						}
						else
						{
							CondText += LINE_TERMINATOR + Cond->GetGraphDisplayText();
						}
					}
					++Idx;
				}
			}

			return FText::FromString(CondText);
		}
	}
	return FText::GetEmpty();
}

FSlateColor SDialogueGraphNode::GetBorderColor() const
{
	if (UDialogueGraphNode* DialogueGraphNode = Cast<UDialogueGraphNode>(GraphNode))
	{
		return DialogueGraphNode->GetGraphNodeColor();
	}

	return FLinearColor::Gray;
}

void SDialogueGraphNodePin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	this->SetCursor(EMouseCursor::Default);

	bShowLabel = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SDialogueGraphNodePin::GetPinBorder)
		.BorderBackgroundColor(this, &SDialogueGraphNodePin::GetPinColor)
		.Cursor(EMouseCursor::Crosshairs)
		.Padding(FMargin(10.0f))
	);
}

FSlateColor SDialogueGraphNodePin::GetPinColor() const
{
	return FSlateColor(FLinearColor::White);
}

TSharedRef<SWidget> SDialogueGraphNodePin::GetDefaultValueWidget()
{
	//Todo find out why this is here
	return SNew(STextBlock);
}

const FSlateBrush* SDialogueGraphNodePin::GetPinBorder() const
{
	return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Body"));
}

#undef LOCTEXT_NAMESPACE