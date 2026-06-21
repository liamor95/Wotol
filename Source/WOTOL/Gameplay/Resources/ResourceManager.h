#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "ResourceManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnResourceChanged, EFactionID, Faction, EResourceType, Resource, int32, NewAmount);

// Gère les 8 ressources pour toutes les factions
// EnergieOceanique a un cap (56/66) — les autres sont illimitées
UCLASS()
class WOTOL_API UResourceManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Ajoute des ressources à une faction
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void AddResource(EFactionID Faction, EResourceType Resource, int32 Amount);

	// Dépense des ressources — retourne false si insuffisant
	UFUNCTION(BlueprintCallable, Category = "Resources")
	bool SpendResource(EFactionID Faction, EResourceType Resource, int32 Amount);

	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetResource(EFactionID Faction, EResourceType Resource) const;

	UFUNCTION(BlueprintPure, Category = "Resources")
	int32 GetResourceCap(EFactionID Faction, EResourceType Resource) const;

	// EnergieOceanique seulement a un cap configurable
	UFUNCTION(BlueprintCallable, Category = "Resources")
	void SetEnergyCap(EFactionID Faction, int32 Cap);

	UFUNCTION(BlueprintPure, Category = "Resources")
	bool HasEnough(EFactionID Faction, EResourceType Resource, int32 Amount) const;

	UPROPERTY(BlueprintAssignable, Category = "Resources")
	FOnResourceChanged OnResourceChanged;

private:
	// [FactionID][ResourceType] = Amount
	TMap<EFactionID, TMap<EResourceType, int32>> Resources;
	TMap<EFactionID, int32>                       EnergyCaps;

	static constexpr int32 DefaultEnergyCap = 100;

	void EnsureFactionExists(EFactionID Faction);
};
