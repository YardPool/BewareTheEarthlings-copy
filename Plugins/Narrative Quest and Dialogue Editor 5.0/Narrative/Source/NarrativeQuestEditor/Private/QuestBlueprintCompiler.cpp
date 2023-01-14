// SurvivalGame Project - The Unreal C++ Survival Game Course - Copyright Reuben Ward 2020


#include "QuestBlueprintCompiler.h"
#include "QuestBlueprintGeneratedClass.h"
#include "QuestGraphSchema.h"
#include "QuestSM.h"
#include "Quest.h"
#include "Kismet2/KismetReinstanceUtilities.h"

FQuestBlueprintCompilerContext::FQuestBlueprintCompilerContext(UQuestBlueprint* SourceQuestBP, FCompilerResultsLog& InMessageLog, const FKismetCompilerOptions& InCompilerOptions)
	: Super(SourceQuestBP, InMessageLog, InCompilerOptions)
	, NewQuestBlueprintClass(nullptr)
	, QuestSchema(nullptr)
{
}

FQuestBlueprintCompilerContext::~FQuestBlueprintCompilerContext()
{
}

void FQuestBlueprintCompilerContext::SpawnNewClass(const FString& NewClassName)
{

	NewQuestBlueprintClass = FindObject<UQuestBlueprintGeneratedClass>(Blueprint->GetOutermost(), *NewClassName);

	if (NewQuestBlueprintClass == nullptr)
	{
		NewQuestBlueprintClass = NewObject<UQuestBlueprintGeneratedClass>(Blueprint->GetOutermost(), FName(*NewClassName), RF_Public | RF_Transactional);
	}
	else
	{
		// Already existed, but wasn't linked in the Blueprint yet due to load ordering issues
		FBlueprintCompileReinstancer::Create(NewQuestBlueprintClass);
	}
	NewClass = NewQuestBlueprintClass;
}

void FQuestBlueprintCompilerContext::OnNewClassSet(UBlueprintGeneratedClass* ClassToUse)
{
	NewQuestBlueprintClass = Cast<UQuestBlueprintGeneratedClass>(ClassToUse);
}

void FQuestBlueprintCompilerContext::PrecompileFunction(FKismetFunctionContext& Context, EInternalCompilerFlags InternalFlags)
{
	Super::PrecompileFunction(Context, InternalFlags);
}

void FQuestBlueprintCompilerContext::CleanAndSanitizeClass(UBlueprintGeneratedClass* ClassToClean, UObject*& InOutOldCDO)
{
	UQuestBlueprint* QuestBP = GetQuestBlueprint();

	const bool bRecompilingOnLoad = Blueprint->bIsRegeneratingOnLoad;
	auto RenameObjectToTransientPackage = [bRecompilingOnLoad](UObject* ObjectToRename, const FName BaseName, bool bClearFlags)
	{
		const ERenameFlags RenFlags = REN_DontCreateRedirectors | (bRecompilingOnLoad ? REN_ForceNoResetLoaders : 0) | REN_NonTransactional | REN_DoNotDirty;

		if (BaseName.IsNone())
		{
			ObjectToRename->Rename(nullptr, GetTransientPackage(), RenFlags);
		}
		else
		{
			FName TransientArchetypeName = MakeUniqueObjectName(GetTransientPackage(), ObjectToRename->GetClass(), BaseName);
			ObjectToRename->Rename(*TransientArchetypeName.ToString(), GetTransientPackage(), RenFlags);
		}

		ObjectToRename->SetFlags(RF_Transient);

		if (bClearFlags)
		{
			ObjectToRename->ClearFlags(RF_Public | RF_Standalone | RF_ArchetypeObject);
		}
		FLinkerLoad::InvalidateExport(ObjectToRename);
	};

	if (!Blueprint->bIsRegeneratingOnLoad && bIsFullCompile)
	{
		if (UQuestBlueprintGeneratedClass* WBC_ToClean = Cast<UQuestBlueprintGeneratedClass>(ClassToClean))
		{
			if (UQuest* OldQuestTemplate = WBC_ToClean->GetQuestTemplate())
			{
				FString TransientArchetypeString = FString::Printf(TEXT("OLD_QUEST_TEMPLATE%s"), *OldQuestTemplate->GetName());
				RenameObjectToTransientPackage(OldQuestTemplate, *TransientArchetypeString, true);


				WBC_ToClean->SetQuestTemplate(nullptr);
			}
		}
	}

	//TODO maybe remove unused states and branches
	Super::CleanAndSanitizeClass(ClassToClean, InOutOldCDO);
}

void FQuestBlueprintCompilerContext::SaveSubObjectsFromCleanAndSanitizeClass(FSubobjectCollection& SubObjectsToSave, UBlueprintGeneratedClass* ClassToClean)
{
	Super::SaveSubObjectsFromCleanAndSanitizeClass(SubObjectsToSave, ClassToClean);

	check(ClassToClean == NewClass);
	NewQuestBlueprintClass = Cast<UQuestBlueprintGeneratedClass>((UObject*)NewClass);

	UQuestBlueprint* QuestBP = GetQuestBlueprint();
	SubObjectsToSave.AddObject(QuestBP->QuestTemplate);
}

void FQuestBlueprintCompilerContext::EnsureProperGeneratedClass(UClass*& TargetUClass)
{
	if (TargetUClass && !((UObject*)TargetUClass)->IsA(UQuestBlueprintGeneratedClass::StaticClass()))
	{
		FKismetCompilerUtilities::ConsignToOblivion(TargetUClass, Blueprint->bIsRegeneratingOnLoad);
		TargetUClass = nullptr;
	}
}


void FQuestBlueprintCompilerContext::CopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	FKismetCompilerContext::CopyTermDefaultsToDefaultObject(DefaultObject);
}

void FQuestBlueprintCompilerContext::FinishCompilingClass(UClass* Class)
{
	UQuestBlueprint* QuestBP = GetQuestBlueprint();
	UQuestBlueprintGeneratedClass* BPGClass = Cast<UQuestBlueprintGeneratedClass>(Class); // why is this not a quest blueprint generated class! 

	UClass* ParentClass = QuestBP->ParentClass;
	const bool bIsSkeletonOnly = CompileOptions.CompileType == EKismetCompileType::SkeletonOnly;

	if (!bIsSkeletonOnly)
	{
		if (!QuestBP->bHasBeenRegenerated)
		{
			UBlueprint::ForceLoadMembers(QuestBP->QuestTemplate);
		}

		FixAbandonedQuestTemplate(QuestBP);

		if (BPGClass && QuestBP->QuestTemplate)
		{
			// Need to clear archetype flag before duplication as we check during dup to see if we should postload
			EObjectFlags PreviousFlags = QuestBP->QuestTemplate->GetFlags();
			QuestBP->QuestTemplate->ClearFlags(RF_ArchetypeObject);

			UQuest* NewQuestTemplate = Cast<UQuest>(StaticDuplicateObject(QuestBP->QuestTemplate, BPGClass, NAME_None, RF_AllFlags & ~RF_DefaultSubObject));
			BPGClass->SetQuestTemplate(NewQuestTemplate);

			QuestBP->QuestTemplate->SetFlags(PreviousFlags);
		}
	}

	Super::FinishCompilingClass(Class);
}

bool FQuestBlueprintCompilerContext::ValidateGeneratedClass(UBlueprintGeneratedClass* Class)
{
	bool SuperResult = Super::ValidateGeneratedClass(Class);
	bool Result = UQuestBlueprint::ValidateGeneratedClass(Class);

	return SuperResult && Result;
}

void FQuestBlueprintCompilerContext::FixAbandonedQuestTemplate(UQuestBlueprint* QuestBP)
{
	UQuest* QuestTemplate = QuestBP->QuestTemplate;

	if (ensure(QuestTemplate))
	{
		if (QuestTemplate->GetName() != TEXT("QuestTemplate"))
		{
			if (UQuest* AbandonedQuestTemplate = static_cast<UQuest*>(FindObjectWithOuter(QuestBP, UQuest::StaticClass(), TEXT("QuestTemplate"))))
			{
				AbandonedQuestTemplate->ClearFlags(RF_DefaultSubObject);
				AbandonedQuestTemplate->SetFlags(RF_Transient);
				AbandonedQuestTemplate->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_ForceNoResetLoaders | REN_NonTransactional | REN_DoNotDirty);
			}

			QuestTemplate->Rename(TEXT("QuestTemplate"), nullptr, REN_DontCreateRedirectors | REN_ForceNoResetLoaders | REN_NonTransactional | REN_DoNotDirty);
			QuestTemplate->SetFlags(RF_DefaultSubObject);
		}
	}
}

FQuestBlueprintCompiler::FQuestBlueprintCompiler()
{

}

bool FQuestBlueprintCompiler::CanCompile(const UBlueprint* Blueprint)
{
	return Cast<UQuestBlueprint>(Blueprint) != nullptr;
}

void FQuestBlueprintCompiler::PreCompile(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions)
{

}

void FQuestBlueprintCompiler::Compile(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results)
{
	if (UQuestBlueprint* QuestBlueprint = CastChecked<UQuestBlueprint>(Blueprint))
	{
		FQuestBlueprintCompilerContext Compiler(QuestBlueprint, Results, CompileOptions);
		Compiler.Compile();
		check(Compiler.NewClass);
	}
}

void FQuestBlueprintCompiler::PostCompile(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions)
{

}

bool FQuestBlueprintCompiler::GetBlueprintTypesForClass(UClass* ParentClass, UClass*& OutBlueprintClass, UClass*& OutBlueprintGeneratedClass) const
{
	if (ParentClass == UQuest::StaticClass() || ParentClass->IsChildOf(UQuest::StaticClass()))
	{
		OutBlueprintClass = UQuestBlueprint::StaticClass();
		OutBlueprintGeneratedClass = UQuestBlueprintGeneratedClass::StaticClass();
		return true;
	}

	return false;
}
