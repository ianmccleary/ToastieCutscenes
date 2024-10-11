#include "ToastieCutsceneAssetFactory.h"
#include "ToastieCutsceneAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Lexer.h"
#include "Logging/StructuredLog.h"
#include "Misc/MessageDialog.h"
#include "ObjectTools.h"
#include "PackageTools.h"

DEFINE_LOG_CATEGORY(TCSImporter);

namespace Parser
{
	struct FScene
	{
		FString Name;
		TArray<FInstancedStruct> Commands;
		bool bDialogue;

		FScene()
			: Name()
			, Commands()
			, bDialogue(false)
		{
			Reset();
		}

		bool IsValid() const
		{
			return !Name.IsEmpty();
		}

		void Reset()
		{
			Name.Reset();
			Commands.Reset();
			bDialogue = false;
		}
	};

#define DEFINE_TCS_KEYWORD(Name) const static FString Keyword##Name(TEXT(#Name))

	DEFINE_TCS_KEYWORD(Scene);
	DEFINE_TCS_KEYWORD(EndScene);
	DEFINE_TCS_KEYWORD(Dialogue);
	DEFINE_TCS_KEYWORD(Block);
	DEFINE_TCS_KEYWORD(EndBlock);
	DEFINE_TCS_KEYWORD(Concurrent);
	DEFINE_TCS_KEYWORD(PlayerChoice);
	DEFINE_TCS_KEYWORD(EndPlayerChoice);
	DEFINE_TCS_KEYWORD(Define);

#define ADD_TCS_TYPE(Command) Command::StaticStruct()

	static const TArray<const UScriptStruct*> CommandTypes =
	{
		ADD_TCS_TYPE(FToastieCutsceneReq),
		ADD_TCS_TYPE(FToastieCutsceneEnablePlayerControl),
		ADD_TCS_TYPE(FToastieCutsceneDisablePlayerControl),
		ADD_TCS_TYPE(FToastieCutsceneWait),
		ADD_TCS_TYPE(FToastieCutsceneLookAt),
		ADD_TCS_TYPE(FToastieCutsceneExit),
		ADD_TCS_TYPE(FToastieCutsceneGoto),
		ADD_TCS_TYPE(FToastieCutsceneOption)
	};

#define SET_VALUE_IF_TYPE_IS(Field, FieldType, ValueType, Value)	\
if (FieldType == TEXT(#ValueType))									\
{																	\
	ValueType Temp = Value;											\
	Field->SetSingleValue_InContainer(StructPtr, (void*)&Temp, 0);	\
}

	const UScriptStruct* TryFindCommandType(const Lexer::FSentence& Sentence)
	{
		if (Sentence.IsKeywordSay())
		{
			return FToastieCutsceneSay::StaticStruct();
		}
		for (auto CommandType : CommandTypes)
		{
			auto CommandTypeKeywordPtr = CommandType->FindMetaData("TCS");
			if (CommandTypeKeywordPtr && Sentence.KeywordIs(*CommandTypeKeywordPtr))
			{
				return CommandType;
			}
		}
		return nullptr;
	}

	FString SanitizeString(
		const FString& Input,
		const Lexer::FSentence& Sentence,
		const TMap<FString, FString>* Defines = nullptr)
	{
		FString Output;

		if (Sentence.IsKeywordSay() && Input.EndsWith(":"))
		{
			Output = Input.LeftChop(1);
		}
		else
		{
			Output = Input.TrimQuotes();
		}

		if (Defines)
		{
			auto Define = Defines->Find(Output);
			if (Define)
			{
				return *Define;
			}
		}

		return Output;
	}

	bool DeserializeStruct(
		const UScriptStruct* CommandType,
		const Lexer::FSentence& Sentence,
		FInstancedStruct& OutStruct,
		const TMap<FString, FString>* Defines = nullptr)
	{
		OutStruct.InitializeAs(CommandType);

		auto StructPtr = (void*)OutStruct.GetMemory();

		for (TFieldIterator<FProperty> It(CommandType); It; ++It)
		{
			auto Field = *It;

			FString FieldExtendedType;
			FString FieldType = Field->GetCPPType(&FieldExtendedType);

			bool bKeyFound = false;
			bool bValueFound = false;
			FString ValueStr;

			auto MetaIndex = Field->FindMetaData("Index");
			if (MetaIndex)
			{
				// The value for this field can be found at the specified index in the sentence
				int32 SentenceIndex = FCString::Atoi(**MetaIndex);
				if (!Sentence.TryGetStringAtIndex(SentenceIndex, ValueStr))
				{
					UE_LOGFMT(TCSImporter, Error, "Synatx Error: Could not find Value at Index {0} for Command {1} at Line {2}", **MetaIndex, CommandType->GetName(), Sentence.GetLineNumber());
					OutStruct.Reset();
					return false;
				}

				bKeyFound = true;
				bValueFound = true;
			}

			auto MetaPropertyName = Field->FindMetaData("Property");
			if (MetaPropertyName)
			{
				bKeyFound = Sentence.Contains(*MetaPropertyName);
				bValueFound = Sentence.TryGetStringProperty(*MetaPropertyName, ValueStr);
			}

			SET_VALUE_IF_TYPE_IS(Field, FieldType, bool, bKeyFound);
			SET_VALUE_IF_TYPE_IS(Field, FieldType, int32, FCString::Atoi(*ValueStr));
			SET_VALUE_IF_TYPE_IS(Field, FieldType, float, FCString::Atof(*ValueStr));
			SET_VALUE_IF_TYPE_IS(Field, FieldType, double, FCString::Atod(*ValueStr));
			SET_VALUE_IF_TYPE_IS(Field, FieldType, FString, SanitizeString(ValueStr, Sentence, Defines));
			SET_VALUE_IF_TYPE_IS(Field, FieldType, FText, FText::FromString(SanitizeString(ValueStr, Sentence, Defines)));
		}

		return true;
	}

	bool TryParse(
		const TArray<Lexer::FSentence>& ASentences,
		UObject* AInParent,
		EObjectFlags AFlags,
		TArray<UObject*>& AObjectsOutput,
		TArray<FString>& ASceneFilter)
	{
		TArray<FInstancedStruct> Reqs;
		TArray<int32> BlockIndices;
		TMap<FString, FString> Defines;
		FScene CurrentScene;

		CurrentScene.Reset();

		for (int32 I = 0; I < ASentences.Num(); ++I)
		{
			auto& Sentence = ASentences[I];

			if (!Sentence.IsValid() || Sentence.IsComment())
				continue;

			/// <summary>
			/// Define
			/// </summary>
			if (Sentence.KeywordIs(KeywordDefine))
			{
				FInstancedStruct Define;
				if (DeserializeStruct(FToastieCutsceneDefine::StaticStruct(), Sentence, Define))
				{
					auto DefineData = Define.Get<FToastieCutsceneDefine>();
					if (!DefineData.InputName.IsEmpty() && !DefineData.OutputName.IsEmpty())
					{
						Defines.Add(DefineData.InputName, DefineData.OutputName);
					}
				}
			}

			/// <summary>
			/// Scene
			/// </summary>
			else if (Sentence.KeywordIs(KeywordScene))
			{
				if (CurrentScene.IsValid())
				{
					UE_LOGFMT(TCSImporter, Error, "Syntax Error: Attempting to start new Scene before ending previous Scene in Line {0}", Sentence.GetLineNumber());
					return false;
				}

				if (!Sentence.TryGetStringAtIndex(1, CurrentScene.Name))
				{
					UE_LOGFMT(TCSImporter, Error, "Syntax Error: Expected Name after Scene in Line {0}", Sentence.GetLineNumber());
					return false;
				}

				CurrentScene.bDialogue = Sentence.Contains(KeywordDialogue);
			}

			/// <summary>
			/// EndScene
			/// </summary>
			else if (Sentence.KeywordIs(KeywordEndScene))
			{
				if (!CurrentScene.IsValid())
				{
					UE_LOGFMT(TCSImporter, Error, "EndScene found with no valid Scene. Line {0}", Sentence.GetLineNumber());
					return false;
				}

				auto bPassesFilter = ASceneFilter.IsEmpty() || ASceneFilter.Contains(CurrentScene.Name);

				if (!CurrentScene.Commands.IsEmpty() && bPassesFilter)
				{
					if (AInParent == nullptr || AInParent->GetOutermost() == nullptr)
					{
						UE_LOG(TCSImporter, Error, TEXT("Import Error: Invalid Parent Object"));
						return false;
					}

					// Calculate "bKeepSpeechBubbleForNextLine" for each Say command
					for (int i = 0; i < CurrentScene.Commands.Num() - 2; ++i)
					{
						auto SayCurrent = CurrentScene.Commands[i + 0].GetMutablePtr<FToastieCutsceneSay>();
						auto SayNext = CurrentScene.Commands[i + 1].GetPtr<FToastieCutsceneSay>();
						if (SayCurrent)
						{
							if (SayNext)
							{
								const auto bSameSpeaker = SayCurrent->Who.Equals(SayNext->Who);
								const auto bSameThink = SayCurrent->bThink == SayNext->bThink;
								SayCurrent->bKeepSpeechBubbleForNextLine = bSameSpeaker && bSameThink;
							}
							else
							{
								SayCurrent->bKeepSpeechBubbleForNextLine = false;
							}
						}
					}
					if (auto SayLast = CurrentScene.Commands.Last().GetMutablePtr<FToastieCutsceneSay>();
						SayLast)
					{
						SayLast->bKeepSpeechBubbleForNextLine = false;
					}

					auto SceneAssetName = ObjectTools::SanitizeObjectName(CurrentScene.Name);

					auto NewPackageName = FPackageName::GetLongPackagePath(AInParent->GetOutermost()->GetName()) + TEXT("/") + SceneAssetName;
					NewPackageName = UPackageTools::SanitizePackageName(NewPackageName);

					auto Package = CreatePackage(*NewPackageName);
					if (Package == nullptr)
					{
						UE_LOG(TCSImporter, Error, TEXT("Import Error: Unable to create Package for TCS Asset"));
						return false;
					}

					Package->FullyLoad();

					auto SceneAsset = NewObject<UToastieCutsceneAsset>(Package, FName(SceneAssetName), AFlags);
					if (SceneAsset)
					{
						SceneAsset->Commands.Append(CurrentScene.Commands);
						SceneAsset->bDialogue = CurrentScene.bDialogue;
						AObjectsOutput.Add(SceneAsset);
					}
				}

				CurrentScene.Reset();
			}

			/// <summary>
			/// Blocks
			/// </summary>
			else if (Sentence.KeywordIs(KeywordBlock) || Sentence.KeywordIs(KeywordPlayerChoice))
			{
				auto Struct = FInstancedStruct::Make<FToastieCutsceneBlock>();
				auto BlockPtr = Struct.GetMutablePtr<FToastieCutsceneBlock>();

				if (Sentence.KeywordIs(KeywordPlayerChoice))
				{
					BlockPtr->Type = EToastieCutsceneBlockType::PlayerChoice;
				}
				else
				{
					BlockPtr->Type = Sentence.Contains(KeywordConcurrent)
						? EToastieCutsceneBlockType::Concurrent
						: EToastieCutsceneBlockType::Sequential;
				}
				BlockPtr->CommandCount = 0;

				BlockIndices.Add(CurrentScene.Commands.Num());
				CurrentScene.Commands.Add(MoveTemp(Struct));
			}

			/// <summary>
			/// EndBlocks
			/// </summary>
			else if (Sentence.KeywordIs(KeywordEndBlock) || Sentence.KeywordIs(KeywordEndPlayerChoice))
			{
				if (BlockIndices.IsEmpty())
				{
					UE_LOGFMT(TCSImporter, Error, "Synatx Error: EndBlock found with no corresponding Block. Line {0}", Sentence.GetLineNumber());
					return false;
				}

				auto Index = BlockIndices.Pop();				
				auto& Struct = CurrentScene.Commands[Index];
				auto BlockPtr = Struct.GetMutablePtr<FToastieCutsceneBlock>();
				if (!BlockPtr)
				{
					UE_LOGFMT(TCSImporter, Error, "Synatx Error: Unable to find corresponding Block for EndBlock in Line {0}", Sentence.GetLineNumber());
					return false;
				}
				
				BlockPtr->CommandCount = CurrentScene.Commands.Num() - Index - 1;
			}

			/// <summary>
			/// Label
			/// </summary>
			else if (Sentence.IsLabel())
			{
				auto Struct = FInstancedStruct::Make<FToastieCutsceneLabel>();
				auto BlockPtr = Struct.GetMutablePtr<FToastieCutsceneLabel>();
				Sentence.TryGetStringAtIndex(0, BlockPtr->Label);
				CurrentScene.Commands.Add(MoveTemp(Struct));
			}
			
			else
			{
				// Determine type of command
				auto CommandType = TryFindCommandType(Sentence);
				if (CommandType == nullptr)
				{
					UE_LOGFMT(TCSImporter, Warning, "Skipping unknown TCS Command \"{0}\" found in Line {1}", Sentence.GetKeyword(), Sentence.GetLineNumber());
					continue;
				}

				// Ensure type is valid
				if (CommandType->IsChildOf(FToastieCutsceneCommandBase::StaticStruct()) == false)
				{
					UE_LOGFMT(TCSImporter, Warning, "Skipping TCS Command \"{0}\" is incorrect Type. Found in Line {1}", Sentence.GetKeyword(), Sentence.GetLineNumber());
					continue;
				}

				FInstancedStruct Struct;
				if (DeserializeStruct(CommandType, Sentence, Struct, &Defines))
				{
					CurrentScene.Commands.Add(MoveTemp(Struct));
				}
			}
		}
		return true;
	}
}

UToastieCutsceneAssetFactory::UToastieCutsceneAssetFactory()
{
	SupportedClass = UToastieCutsceneAsset::StaticClass();

	bCreateNew = false;
	bEditorImport = true;
	bText = true;

	Formats.Add(TEXT("tcs;Toastie Cutscene File"));
}

UObject* UToastieCutsceneAssetFactory::FactoryCreateText(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	const TCHAR* Type,
	const TCHAR*& Buffer,
	const TCHAR* BufferEnd,
	FFeedbackContext* Warn)
{
	auto ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>();
	if (!ImportSubsystem)
	{
		return nullptr;
	}

	ImportSubsystem->BroadcastAssetPreImport(this, InClass, InParent, InName, TEXT("TCS"));

	TArray<Lexer::FSentence> Sentences;
	if (!Lexer::TryTokenize(FString(BufferEnd - Buffer, Buffer), Sentences))
	{
		// ERROR: Invalid token was found
		ImportSubsystem->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	TArray<UObject*> OutputObjects;
	if (!Parser::TryParse(Sentences, InParent, Flags, OutputObjects, SceneFilter))
	{
		// ERROR: Unable to parse tokens
		ImportSubsystem->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}

	if (OutputObjects.Num() > 0)
	{
		AdditionalImportedObjects.Reserve(OutputObjects.Num());
		for (auto Object : OutputObjects)
		{
			auto Cutscene = Cast<UToastieCutsceneAsset>(Object);
			if (Cutscene)
			{
				Cutscene->AssetImportData->Update(UFactory::GetCurrentFilename());
				FAssetRegistryModule::AssetCreated(Cutscene);
				ImportSubsystem->BroadcastAssetPostImport(this, Cutscene);
				Cutscene->MarkPackageDirty();
				Cutscene->PostEditChange();
				AdditionalImportedObjects.Add(Cutscene);
			}
		}
	}

	return OutputObjects.Num() > 0 ? OutputObjects[0] : nullptr;
}

bool UToastieCutsceneAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	auto ToastieCutsceneAsset = Cast<UToastieCutsceneAsset>(Obj);
	if (ToastieCutsceneAsset && ToastieCutsceneAsset->AssetImportData)
	{
		ToastieCutsceneAsset->AssetImportData->ExtractFilenames(OutFilenames);
		return true;
	}
	return false;
}

void UToastieCutsceneAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	auto ToastieCutsceneAsset = Cast<UToastieCutsceneAsset>(Obj);
	if (ToastieCutsceneAsset && ToastieCutsceneAsset->AssetImportData && ensure(NewReimportPaths.Num() == 1))
	{
		ToastieCutsceneAsset->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UToastieCutsceneAssetFactory::Reimport(UObject* Obj)
{
	// Make sure the original object is the correct type
	auto ToastieCutsceneAsset = Cast<UToastieCutsceneAsset>(Obj);
	if (!ToastieCutsceneAsset)
	{
		return EReimportResult::Failed;
	}

	// Make sure file is valid and exists
	const FString Filename = ToastieCutsceneAsset->AssetImportData->GetFirstFilename();
	if (!Filename.Len() || IFileManager::Get().FileSize(*Filename) == INDEX_NONE)
	{
		return EReimportResult::Failed;
	}

	SceneFilter.Empty();
	SceneFilter.Add(Obj->GetName());

	auto Result = EReimportResult::Failed;

	bool bOutCanceled = false;
	if (ImportObject(ToastieCutsceneAsset->GetClass(), ToastieCutsceneAsset->GetOuter(), *ToastieCutsceneAsset->GetName(), RF_Public | RF_Standalone, Filename, nullptr, bOutCanceled) != nullptr)
	{
		ToastieCutsceneAsset->AssetImportData->Update(Filename);

		if (ToastieCutsceneAsset->GetOuter())
		{
			ToastieCutsceneAsset->GetOuter()->MarkPackageDirty();
		}
		else
		{
			ToastieCutsceneAsset->MarkPackageDirty();
		}

		Result = EReimportResult::Succeeded;
	}
	else
	{
		Result = bOutCanceled
			? EReimportResult::Cancelled
			: EReimportResult::Failed;
	}

	SceneFilter.Empty();

	return Result;
}