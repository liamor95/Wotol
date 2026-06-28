#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UnitDataRegistrySubsystem.generated.h"

class UUnitDataAsset;

// Subsystem qui charge toutes les stats d'unités depuis UnitDataLibrary au démarrage.
// Liamor n'a pas besoin de créer des DataAssets dans l'éditeur — tout est dans le code.
// Accès : GetGameInstance()->GetSubsystem<UUnitDataRegistrySubsystem>()->GetUnitData("Aquis")
UCLASS()
class WOTOL_API UUnitDataRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Retourne les stats d'une unité par son ID canonique
	UFUNCTION(BlueprintPure, Category = "Units")
	UUnitDataAsset* GetUnitData(FName UnitID) const;

	// Toutes les unités d'une faction
	UFUNCTION(BlueprintPure, Category = "Units")
	TArray<UUnitDataAsset*> GetUnitsForFaction(EFactionID Faction) const;

	// Toutes les unités de la démo (12 unités)
	UFUNCTION(BlueprintPure, Category = "Units")
	TArray<UUnitDataAsset*> GetAllDemoUnits() const;

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<UUnitDataAsset>> Registry;
};
