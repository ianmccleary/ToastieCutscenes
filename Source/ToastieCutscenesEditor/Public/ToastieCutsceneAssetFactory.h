#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "EditorReimportHandler.h"
#include "ToastieCutsceneAssetFactory.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(TCSImporter, Log, All);

/**
 * 
 */
UCLASS()
class TOASTIECUTSCENESEDITOR_API UToastieCutsceneAssetFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UToastieCutsceneAssetFactory();
	
protected:
	//~ Begin UFactory Interface
	virtual UObject* FactoryCreateText(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, const TCHAR* Type, const TCHAR*& Buffer, const TCHAR* BufferEnd, FFeedbackContext* Warn) override;
	//~ End UFactory Interface

	//~ Begin FReimportHandler
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames)  override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	//~ End FReimportHandler

private:
	TArray<FString> SceneFilter;
};
