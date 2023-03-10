// Copyright Narrative Tools 2022. 

#include "SQuestGraphNode.h"
#include "QuestGraphNode.h"
#include "QuestGraphNode_PersistentTasks.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Editor.h"
#include "QuestGraph.h"
#include "SGraphPanel.h"
#include "ScopedTransaction.h"
#include "QuestSM.h"
#include "QuestEditorStyle.h"
#include "Narrative/Public/Quest.h"

#define LOCTEXT_NAMESPACE "SQuestGraphNode"

void SQuestGraphNode::Construct(const FArguments& InArgs, UQuestGraphNode* InNode)
{
	GraphNode = InNode;

	UpdateGraphNode();

}

void SQuestGraphNode::CreatePinWidgets()
{
	UQuestGraphNode* QuestGraphNode = CastChecked<UQuestGraphNode>(GraphNode);

	for (int32 PinIdx = 0; PinIdx < QuestGraphNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* MyPin = QuestGraphNode->Pins[PinIdx];
		if (!MyPin->bHidden)
		{
			TSharedPtr<SGraphPin> NewPin = SNew(SGraphPin, MyPin);

			AddPin(NewPin.ToSharedRef());
		}
	}
}

void SQuestGraphNode::UpdateGraphNode()
{

	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	FSlateRenderTransform QuantityRender;
	QuantityRender.SetTranslation(FVector2D(0.f, -30.f));

	FSlateRenderTransform CurrentStateIconRender;
	CurrentStateIconRender.SetTranslation(FVector2D(0.f, -96.f));

	this->GetOrAddSlot(ENodeZone::Center)
.HAlign(HAlign_Fill)
.VAlign(VAlign_Fill)
[
	SNew(SBorder)
	.BorderImage(FEditorStyle::GetBrush("Graph.StateNode.Body"))
	.BorderBackgroundColor(this, &SQuestGraphNode::GetBorderColor)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SSpacer)
				.Size(FVector2D(250.f, 75.f))
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Top)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("Kismet.DebuggerOverlay.InstructionPointer"))
				.RenderTransform(CurrentStateIconRender)
				.Visibility(this, &SQuestGraphNode::GetCurrentStatePointerVisibility)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(5.f)
			[
				SNew(STextBlock)
				.Text(this, &SQuestGraphNode::GetNodeQuantityText)
				.Justification(ETextJustify::Center)
				.Clipping(EWidgetClipping::Inherit)
				.TextStyle(FEditorStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
				.RenderTransform(QuantityRender)
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(5.f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.VAlign(VAlign_Top)
				.HAlign(HAlign_Fill)
				[
					SNew(STextBlock)
					.Text(this, &SQuestGraphNode::GetNodeTitleText)
					.Justification(ETextJustify::Center)
					.Clipping(EWidgetClipping::Inherit)
					.TextStyle(FEditorStyle::Get(), TEXT("PhysicsAssetEditor.Tools.Font"))
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
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
					.AutoWidth()
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SSpacer)
							.Size(FVector2D(200.f, 50.f))
						]
						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(this, &SQuestGraphNode::GetNodeText)
							.Justification(ETextJustify::Center)
							.WrapTextAt(200.f)
							.Clipping(EWidgetClipping::Inherit)
						]
					]
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Right)
					[
						SNew(SBox)
						.MinDesiredHeight(10.f)
					[
						SAssignNew(RightNodeBox, SVerticalBox)
					]
				]
			]
		]
	]
];

	CreatePinWidgets();
}

TSharedPtr<SToolTip> SQuestGraphNode::GetComplexTooltip()
{
	return nullptr;
}

void SQuestGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
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

FReply SQuestGraphNode::OnMouseDown(const FGeometry& SenderGeometry, const FPointerEvent& MouseEvent)
{
	return FReply::Handled();
}

FText SQuestGraphNode::GetNodeText() const
{
	UQuestGraphNode* Node = CastChecked<UQuestGraphNode>(GraphNode);

	if (Node->IsA<UQuestGraphNode_PersistentTasks>())
	{
		return FText::FromString("Tasks connected to here can be done from anywhere in the Quest.");
	}

	return (Node && Node->QuestNode) ? Node->QuestNode->Description : FText::GetEmpty();
}

FText SQuestGraphNode::GetNodeTitleText() const
{
	UQuestGraphNode* Node = CastChecked<UQuestGraphNode>(GraphNode);

	if (Node->IsA<UQuestGraphNode_PersistentTasks>())
	{
		return FText::FromString("Persistent Tasks");
	}

	return (Node && Node->QuestNode) ? Node->QuestNode->GetNodeTitle() : FText::GetEmpty();
}

FText SQuestGraphNode::GetNodeQuantityText() const
{
	//UQuestGraphNode* Node = CastChecked<UQuestGraphNode>(GraphNode);

	//if (Node->IsA<UQuestGraphNode_State>())
	//{
	//	if (UQuestState* StateNode = Cast<UQuestState>(Node->QuestNode))
	//	{
	//		if (UQuest* OwningQuest = StateNode->GetOwningQuest())
	//		{
	//			if (OwningQuest->GetCurrentState() == StateNode)
	//			{
	//				return LOCTEXT("StateCurrentNode", "CURRENT STATE");
	//			}
	//		}
	//	}
	//}
	return FText::GetEmpty();
}

EVisibility SQuestGraphNode::GetNodeTextVisibility() const
{
	TSharedPtr<SGraphPanel> MyOwnerPanel = GetOwnerPanel();
	return (!MyOwnerPanel.IsValid() || MyOwnerPanel->GetCurrentLOD() > EGraphRenderingLOD::LowDetail) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SQuestGraphNode::GetCurrentStatePointerVisibility() const
{
	UQuestGraphNode* Node = CastChecked<UQuestGraphNode>(GraphNode);

	if (Node->IsA<UQuestGraphNode_State>())
	{
		if (UQuestState* StateNode = Cast<UQuestState>(Node->QuestNode))
		{
			if (UQuest* OwningQuest = StateNode->GetOwningQuest())
			{
				if (OwningQuest->GetCurrentState() == StateNode)
				{
					return EVisibility::HitTestInvisible;
				}
			}
		}
	}
	return EVisibility::Hidden;
}

FSlateColor SQuestGraphNode::GetBorderColor() const
{

	if (UQuestGraphNode* QuestGraphNode = Cast<UQuestGraphNode>(GraphNode))
	{
		return QuestGraphNode->GetGraphNodeColor();
	}

	return FLinearColor::Gray;
}

void SQuestGraphNodePin::Construct(const FArguments& InArgs, UEdGraphPin* InPin)
{
	this->SetCursor(EMouseCursor::Default);

	bShowLabel = true;

	GraphPinObj = InPin;
	check(GraphPinObj != NULL);

	const UEdGraphSchema* Schema = GraphPinObj->GetSchema();
	check(Schema);

	SBorder::Construct(SBorder::FArguments()
		.BorderImage(this, &SQuestGraphNodePin::GetPinBorder)
		.BorderBackgroundColor(this, &SQuestGraphNodePin::GetPinColor)
		.Cursor(EMouseCursor::Crosshairs)
		.Padding(FMargin(10.0f))
	);
}

FSlateColor SQuestGraphNodePin::GetPinColor() const
{
	return FSlateColor(FLinearColor::White);
}

TSharedRef<SWidget> SQuestGraphNodePin::GetDefaultValueWidget()
{
	//Todo find out why this is here
	return SNew(STextBlock);
}

const FSlateBrush* SQuestGraphNodePin::GetPinBorder() const
{
	return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Body"));
}

#undef LOCTEXT_NAMESPACE