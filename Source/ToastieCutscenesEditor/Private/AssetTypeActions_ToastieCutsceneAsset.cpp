#include "AssetTypeActions_ToastieCutsceneAsset.h"
#include "Internationalization/Internationalization.h"
#include "ToastieCutsceneAsset.h"

UClass* FAssetTypeActions_ToastieCutsceneAsset::GetSupportedClass() const
{
	return UToastieCutsceneAsset::StaticClass();
}

// IAssetTypeActions Implementation
FText FAssetTypeActions_ToastieCutsceneAsset::GetName() const
{
	return INVTEXT("Toastie Cutscene");
}

FColor FAssetTypeActions_ToastieCutsceneAsset::GetTypeColor() const
{
	return FColor::Purple;
}

bool FAssetTypeActions_ToastieCutsceneAsset::IsImportedAsset() const
{
	return true;
}
// End IAssetTypeActions
