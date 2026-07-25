#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "FactionDataAsset.generated.h"

UCLASS(BlueprintType)
class WOTOL_API UFactionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EFactionID FactionID = EFactionID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText LoreDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftObjectPtr<UTexture2D> FactionIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resources")
	FName PrimaryResource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Units")
	TArray<TSoftClassPtr<class UUnitDataAsset>> AvailableUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float BaseAggressionBias = 0.5f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("FactionData", GetFName());
	}
};
