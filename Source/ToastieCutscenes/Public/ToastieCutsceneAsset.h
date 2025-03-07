#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "InstancedStruct.h"
#include "ToastieCutsceneAsset.generated.h"

/*
* 
* Commands Meta
* When parsing the TCS file, each line is separated into tokens. The tokens are parsed into commands
* by way of the meta tag in UPROPERTY. The following is a list of all metas that are used when parsing TCS tokens
* 
* Index=X
*	The value of this property is the Xth token
* 
* Property="X"
*	The value of this property is the value after the token named X. If no token X is found, the default value is kept
* 
* Ex:
* WalkTo "Bobby" "Marker12"
* 
* This command has 3 tokens. A UPROPERTY with meta=(Index="1") would assign the value of token at index 1 to that UPROPERTY
* So, it would assign the string value "Bobby" to that UPROPERTY
* 
*/

UENUM(BlueprintType)
enum class EToastieCutsceneOperator : uint8
{
	LessThan			UMETA(DisplayName = "Less Than"),
	LessThanOrEqual		UMETA(DisplayName = "Less Than or Equal"),
	Equal				UMETA(DisplayName = "Equal"),
	GreaterThan			UMETA(DisplayName = "Greater Than"),
	GreaterThanOrEqual	UMETA(DisplayName = "Greater Than or Equal"),
	NotEqual			UMETA(DisplayName = "Not Equal")
};

UENUM(BlueprintType)
enum class EToastieCutsceneBlockType : uint8
{
	Sequential			UMETA(DisplayName = "Sequential"),
	Concurrent			UMETA(DisplayName = "Concurrent"),
	PlayerChoice		UMETA(DisplayName = "Player Choice")
};

USTRUCT()
struct TOASTIECUTSCENES_API FToastieCutsceneDataBase
{
	GENERATED_BODY()
};

USTRUCT()
struct TOASTIECUTSCENES_API FToastieCutsceneDefine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Index = "1")) FString InputName;
	UPROPERTY(EditAnywhere, meta = (Property = "As")) FString OutputName;
};

/// <summary> Requirement </summary>
USTRUCT(BlueprintType, meta = (TCS = "Requirement"))
struct TOASTIECUTSCENES_API FToastieCutsceneReq : public FToastieCutsceneDataBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "1")) FString Key;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "2")) EToastieCutsceneOperator Op;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "3")) int32 Value;
};

USTRUCT(BlueprintType)
struct TOASTIECUTSCENES_API FToastieCutsceneCommandBase : public FToastieCutsceneDataBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere) TArray<FToastieCutsceneReq> Requirements;
	UPROPERTY(EditAnywhere, meta = (Property = "Delay")) double Delay;
	UPROPERTY(EditAnywhere, meta = (Property = "DoNotBlock")) bool bDoNotBlock;
};

/// <summary> Block </summary>
USTRUCT(BlueprintType)
struct TOASTIECUTSCENES_API FToastieCutsceneBlock : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) EToastieCutsceneBlockType Type;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 CommandCount;
};

/// <summary> Say </summary>
USTRUCT(BlueprintType)
struct TOASTIECUTSCENES_API FToastieCutsceneSay : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "0")) FString Who;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "1")) FText Line;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Property = "Timed")) double Time;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Property = "NoAnimation")) bool bNoAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Property = "Think")) bool bThink;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bKeepSpeechBubbleForNextLine;
};

/// <summary> EnablePlayerControl </summary>
USTRUCT(BlueprintType, meta = (TCS = "EnablePlayerControl"))
struct TOASTIECUTSCENES_API FToastieCutsceneEnablePlayerControl : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()
};

/// <summary> DisablePlayerControl </summary>
USTRUCT(BlueprintType, meta = (TCS = "DisablePlayerControl"))
struct TOASTIECUTSCENES_API FToastieCutsceneDisablePlayerControl : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()
};

/// <summary> Wait </summary>
USTRUCT(BlueprintType, meta = (TCS = "Wait"))
struct TOASTIECUTSCENES_API FToastieCutsceneWait : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "1")) double Time;
};

/// <summary> LookAt </summary>
USTRUCT(BlueprintType, meta = (TCS = "LookAt"))
struct TOASTIECUTSCENES_API FToastieCutsceneLookAt : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "1")) FString Who;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "2")) FString Target;
};

/// <summary> Exit </summary>
USTRUCT(BlueprintType, meta = (TCS = "Exit"))
struct TOASTIECUTSCENES_API FToastieCutsceneExit : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()
};

/// <summary> Goto </summary>
USTRUCT(BlueprintType, meta = (TCS = "Goto"))
struct TOASTIECUTSCENES_API FToastieCutsceneGoto : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "1")) FString Label;
};

/// <summary> Label </summary>
USTRUCT(BlueprintType, meta = (TCS = "Label"))
struct TOASTIECUTSCENES_API FToastieCutsceneLabel : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FString Label;
};

/// <summary> Option </summary>
USTRUCT(BlueprintType, meta = (TCS = "Option"))
struct TOASTIECUTSCENES_API FToastieCutsceneOption : public FToastieCutsceneCommandBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "1")) FString Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Index = "2")) FText DisplayText;
};

/**
 * 
 */
UCLASS(BlueprintType)
class TOASTIECUTSCENES_API UToastieCutsceneAsset : public UObject
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (BaseStruct = "ToastieCutsceneDataBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> Commands;

	UPROPERTY(BlueprintReadOnly)
	bool bDialogue;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = ImportSettings)
	TObjectPtr<UAssetImportData> AssetImportData;
#endif

	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
};
