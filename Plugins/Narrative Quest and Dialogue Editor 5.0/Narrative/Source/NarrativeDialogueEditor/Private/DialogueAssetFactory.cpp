// Copyright Narrative Tools 2022. 


#include "DialogueAssetFactory.h"
#include "DialogueAsset.h"

#define LOCTEXT_NAMESPACE "DialogueAssetFactory"

UDialogueAssetFactory::UDialogueAssetFactory()
{
	SupportedClass = UDialogueAsset::StaticClass();

	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDialogueAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UDialogueAsset>(InParent, Class, Name, Flags | RF_Transactional);
}

bool UDialogueAssetFactory::CanCreateNew() const
{
	return true;
}

FString UDialogueAssetFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("NewDialogue"));
}

FText UDialogueAssetFactory::GetDisplayName() const
{
	return LOCTEXT("DialogueText", "Dialogue");
}

#undef LOCTEXT_NAMESPACE