#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "HeroLoadoutDataAsset.generated.h"

UCLASS(BlueprintType)
class WOTOL_API UHeroLoadoutDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FText HeroDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	EFactionID Faction = EFactionID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	float MaxHealth = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	float AttackDamage = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<FName> DefaultAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<FName> UnlockableAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<USkeletalMesh> HeroMesh;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("HeroLoadout", GetFName());
	}
};
