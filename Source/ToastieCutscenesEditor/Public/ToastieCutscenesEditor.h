#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetTypeCategories.h"
#include "IAssetTools.h"

class FToastieCutscenesEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:

	TSharedPtr<IAssetTypeActions> ToastieCutsceneAssetTypeActions;
};
