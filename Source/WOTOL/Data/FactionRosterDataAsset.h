#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "FactionRosterDataAsset.generated.h"

// Roster d'unités d'une faction — instancié par faction dans l'éditeur
// Liamor remplit ce DataAsset avec les unités et leurs quotas
// Affiché dans l'écran "Unités Disponibles" (5 unités max dans l'escouade)
UCLASS(BlueprintType)
class WOTOL_API UFactionRosterDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	EFactionID Faction = EFactionID::None;

	// Toutes les unités disponibles pour cette faction avec leurs quotas
	// Ex: Aquistance (Distance, max 3), Léviaphénix (Mythique, max 1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roster")
	TArray<FUnitRosterEntry> AvailableUnits;

	// Nombre max d'unités dans l'escouade (= 5 pour la démo)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roster")
	int32 MaxSquadSize = 5;

	// Ressource primaire de la faction
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	EResourceType PrimaryResource = EResourceType::BiomasseMarine;

	UFUNCTION(BlueprintPure, Category = "Roster")
	int32 GetMaxCountForRole(EUnitRole Role) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("FactionRoster", GetFName());
	}
};
