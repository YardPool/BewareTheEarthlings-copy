// Copyright Narrative Tools 2022. 

#include "DialogueSM.h"
#include "Dialogue.h"
#include "DialogueAsset.h"
#include "NarrativeComponent.h"
#include "NarrativeCondition.h"
#include "Animation/AnimInstance.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "NarrativeDialogueSettings.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"

#define LOCTEXT_NAMESPACE "DialogueSM"

TArray<class UDialogueNode_NPC*> UDialogueNode::GetNPCReplies(APlayerController* OwningController, APawn* OwningPawn, class UNarrativeComponent* NarrativeComponent)
{
	TArray<class UDialogueNode_NPC*> ValidReplies;

	for (auto& NPCReply : NPCReplies)
	{
		if (NPCReply->AreConditionsMet(OwningPawn, OwningController, NarrativeComponent))
		{
			ValidReplies.Add(NPCReply);
		}
	}

	return ValidReplies;
}

TArray<class UDialogueNode_Player*> UDialogueNode::GetPlayerReplies(APlayerController* OwningController, APawn* OwningPawn, class UNarrativeComponent* NarrativeComponent)
{
	TArray<class UDialogueNode_Player*> ValidReplies;

	for (auto& PlayerReply : PlayerReplies)
	{
		if(PlayerReply && PlayerReply->AreConditionsMet(OwningPawn, OwningController, NarrativeComponent))
		{
			ValidReplies.Add(PlayerReply);
		}
	}

	return ValidReplies;
}


UWorld* UDialogueNode::GetWorld() const
{
	return OwningComponent ? OwningComponent->GetWorld() : nullptr;
}

void UDialogueNode::Play_Implementation(class UNarrativeComponent* Player)
{
	OwningComponent = Player;
}

void UDialogueNode::FinishPlay()
{
	OnDialogueFinished.Broadcast();
}

FText UDialogueNode_NPC::GetNodeTitleText() const
{
	if (UDialogue* DialogueOuter = CastChecked<UDialogue>(GetOuter()))
	{
		return DialogueOuter->DialogueName.IsEmpty() ? FText::FromString("NPC") : DialogueOuter->DialogueName;
	}

	return FText::FromString("NPC");
}

TArray<class UDialogueNode_NPC*> UDialogueNode_NPC::GetReplyChain(APlayerController* OwningController, APawn* OwningPawn, class UNarrativeComponent* NarrativeComponent)
{
	TArray<UDialogueNode_NPC*> NPCFollowUpReplies;
	UDialogueNode_NPC* CurrentNode = this;

	NPCFollowUpReplies.Add(CurrentNode);

	while (CurrentNode)
	{
		if (CurrentNode != this)
		{
			NPCFollowUpReplies.Add(CurrentNode);
		}

		TArray<UDialogueNode_NPC*> NPCRepliesToRet = CurrentNode->NPCReplies;

		//Sort the replies by their Y position in the graph TODO: this currently doesn't work because NodePos isn't being updated in the schema. figure out why
		NPCRepliesToRet.Sort([](const UDialogueNode_NPC& NodeA, const UDialogueNode_NPC& NodeB) {
			return  NodeA.NodePos.Y > NodeB.NodePos.Y;
			});

		//If we don't find another node after this the loop will exit
		CurrentNode = nullptr;

		//Find the next valid reply. We'll then repeat this cycle until we run out
		for (auto& Reply : NPCRepliesToRet)
		{
			if (Reply != this && Reply->AreConditionsMet(OwningPawn, OwningController, NarrativeComponent))
			{
				CurrentNode = Reply;
				break; // just use the first reply with valid conditions
			}
		}
	}

	return NPCFollowUpReplies;
}

void UDialogueNode_NPC::Play_Implementation(class UNarrativeComponent* OwningComp)
{
	
	Super::Play_Implementation(OwningComp);

	if (!OwningComp)
	{
		FinishPlay();
		return;
	}

	if (UDialogue* Dialogue = OwningComp->CurrentDialogue.Dialogue)
	{
		if (OwningComp->GetNetMode() == NM_Standalone)
		{
			ProcessEvents(OwningComp->GetOwningPawn(), OwningComp->GetOwningController(), OwningComp);
		}
		else
		{
			UE_LOG(LogNarrative, Warning, TEXT("NPC dialogue node %s has events added to it, but these won't be processed in a networked game. Add your events to a player dialogue node instead."), *GetNameSafe(this));
		}

		//Empty NPC nodes are just used for dialogue flow and should immedediately be skipped over
		if (Text.IsEmptyOrWhitespace())
		{
			FinishPlay();
			return;
		}

		if (DialogueSound)
		{

			if (OwningComp->CurrentDialogue.NPC)
			{
				//NPCs should spawn their dialogue clip on the actor, then finish their dialogue once that clip is over.
				//Games might want to override this function and play a facial animation or do something more custom, they can override this function
				if (UAudioComponent* DialogueAudio = UGameplayStatics::SpawnSoundAtLocation(OwningComp, DialogueSound, OwningComp->CurrentDialogue.NPC->GetActorLocation(), OwningComp->CurrentDialogue.NPC->GetActorForwardVector().Rotation()))
				{
					DialogueAudio->OnAudioFinished.AddUniqueDynamic(this, &UDialogueNode::FinishPlay);
				}
			}
			else //If dialogue wasnt passed an NPC just play 2D audio
			{
				if (UAudioComponent* DialogueAudio = UGameplayStatics::SpawnSound2D(OwningComp, DialogueSound))
				{
					DialogueAudio->OnAudioFinished.AddUniqueDynamic(this, &UDialogueNode::FinishPlay);
				}
			}
		}
		else// if there isn't any dialogue sound just delay for long enough for player to be able to read the dialogue text
		{
			//Empty NPC nodes are just used for dialogue flow and should immedediately be skipped over
			float LettersPerMinute = 25.f;
			float MinDialogueTextDisplayTime = 2.f;

			if (const UNarrativeDialogueSettings* DialogueSettings = GetDefault<UNarrativeDialogueSettings>())
			{
				LettersPerMinute = DialogueSettings->LettersPerMinute;
				MinDialogueTextDisplayTime = DialogueSettings->MinDialogueTextDisplayTime; 
			}

			FTimerHandle DummyHandle;
			const float DisplayTime = FMath::Max(Text.ToString().Len() / LettersPerMinute, MinDialogueTextDisplayTime); // 25 letters per second is a good average reading time, use 2 seconds as a min TODO make 2s configurable
			OwningComp->GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &UDialogueNode::FinishPlay, DisplayTime, false);
		}

		if (DialogueMontage)
		{
			//Use characters anim montage player otherwise use generic
			if (ACharacter* NPCChar = Cast<ACharacter>(OwningComp->CurrentDialogue.NPC))
			{
				if (NPCChar)
				{
					NPCChar->PlayAnimMontage(DialogueMontage);
				}
			}
			else if (OwningComp->CurrentDialogue.NPC)
			{
				if (USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(OwningComp->CurrentDialogue.NPC->GetComponentByClass(USkeletalMeshComponent::StaticClass())))
				{
					if (SkelMeshComp->GetAnimInstance())
					{
						SkelMeshComp->GetAnimInstance()->Montage_Play(DialogueMontage);
					}
				}
			}
		}

		//Ask narratives dialogue player to play the NPCs talking shot for us 
		if (Shot || Dialogue->NPCTalkingShot)
		{
			OwningComp->PlayDialogueShot(Shot ? Shot : Dialogue->NPCTalkingShot, Dialogue->NPCTalkingSettings);
		}
	}
	else
	{
		FinishPlay();
	}

}

void UDialogueNode_NPC::FinishPlay()
{
	Super::FinishPlay();

	//Dont go to reply shot if this is just a control node 
	if (!Text.IsEmptyOrWhitespace())
	{
		if (UDialogue* Dialogue = OwningComponent->CurrentDialogue.Dialogue)
		{
			//Once the NPC is finished talking, play the select reply cinematic 
			if (Dialogue->SelectReplyShot)
			{
				OwningComponent->PlayDialogueShot(Dialogue->SelectReplyShot, Dialogue->SelectReplySettings);
			}
	}
	}
}

void UDialogueNode_Player::Play_Implementation(class UNarrativeComponent* OwningComp)
{
	Super::Play_Implementation(OwningComp);

	if (!OwningComp)
	{
		FinishPlay();
	}

	if (OwningComp)
	{
		if (UDialogue* Dialogue = OwningComp->CurrentDialogue.Dialogue)
		{
			//If the player has some audio to say wait till thats finished, otherwise just wait 1s. TODO make 1s config option 
			if (DialogueSound)
			{
				if (UAudioComponent* DialogueAudio = UGameplayStatics::SpawnSound2D(OwningComp, DialogueSound))
				{
					DialogueAudio->OnAudioFinished.AddUniqueDynamic(this, &UDialogueNode::FinishPlay);
				}
			}
			else
			{
				float MinDialogueTextDisplayTime = 2.f;

				if (const UNarrativeDialogueSettings* DialogueSettings = GetDefault<UNarrativeDialogueSettings>())
				{
					MinDialogueTextDisplayTime = DialogueSettings->MinDialogueTextDisplayTime;
				}

				FTimerHandle DummyHandle;
				OwningComp->GetWorld()->GetTimerManager().SetTimer(DummyHandle, this, &UDialogueNode::FinishPlay, 1.f, false);
			}

			//Use characters anim montage player otherwise use generic
			if (DialogueMontage)
			{
				ACharacter* PlayerChar = Cast<ACharacter>(OwningComp->GetOwningPawn());

				if (PlayerChar)
				{
					PlayerChar->PlayAnimMontage(DialogueMontage);
				}
				else if (APawn* OwningPawn = OwningComp->GetOwningPawn())
				{
					if (USkeletalMeshComponent* SkelMeshComp = Cast<USkeletalMeshComponent>(OwningPawn->GetComponentByClass(USkeletalMeshComponent::StaticClass())))
					{
						if (SkelMeshComp->GetAnimInstance())
						{
							SkelMeshComp->GetAnimInstance()->Montage_Play(DialogueMontage);
						}
					}
				}
			}

			//Play the cinematic for while the player talks
		    if (Shot || Dialogue->PlayerTalkingShot)
			{
				OwningComponent->PlayDialogueShot(Shot ? Shot : Dialogue->PlayerTalkingShot, Shot ? ShotSettings : Dialogue->PlayerTalkingSettings);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE