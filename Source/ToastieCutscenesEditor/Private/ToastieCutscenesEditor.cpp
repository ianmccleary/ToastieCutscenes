#include "ToastieCutscenesEditor.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_ToastieCutsceneAsset.h"

namespace
{
	const static FName AssetToolsModuleName(TEXT("AssetTools"));
}

void FToastieCutscenesEditorModule::StartupModule()
{
	// Register asset types
	ToastieCutsceneAssetTypeActions = MakeShareable(new FAssetTypeActions_ToastieCutsceneAsset());
	FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(ToastieCutsceneAssetTypeActions.ToSharedRef());
}

void FToastieCutscenesEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(ToastieCutsceneAssetTypeActions.ToSharedRef());
	}
}
	
IMPLEMENT_MODULE(FToastieCutscenesEditorModule, ToastieCutscenes)