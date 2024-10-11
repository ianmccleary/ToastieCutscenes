#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/**
 * 
 */
class FAssetTypeActions_ToastieCutsceneAsset : public FAssetTypeActions_Base
{
public:
	// IAssetTypeActions interface
	virtual UClass* GetSupportedClass() const override;
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Misc; }

	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual bool IsImportedAsset() const override;
	// End of IAssetTypeActions interface
};
