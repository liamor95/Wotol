#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "HexGridManager.generated.h"

class AUnitBase;

// Grille hexagonale pour la phase de déploiement
// Coordonnées axiales standard (q, r) — convertibles en world position
UCLASS()
class WOTOL_API UHexGridManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Taille d'une cellule hex en unités UE (rayon centre → sommet)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HexGrid")
	float HexSize = 150.f;

	// Position monde du centre de la grille
	UPROPERTY(BlueprintReadWrite, Category = "HexGrid")
	FVector GridOrigin = FVector::ZeroVector;

	// Conversion coordonnées hex ↔ world
	UFUNCTION(BlueprintPure, Category = "HexGrid")
	FVector HexToWorld(FHexCoord Hex) const;

	UFUNCTION(BlueprintPure, Category = "HexGrid")
	FHexCoord WorldToHex(FVector WorldPos) const;

	// Zones de déploiement par faction
	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	void SetDeploymentZone(EFactionID Faction, const TArray<FHexCoord>& Cells);

	UFUNCTION(BlueprintPure, Category = "HexGrid")
	bool IsInDeploymentZone(EFactionID Faction, FHexCoord Cell) const;

	UFUNCTION(BlueprintPure, Category = "HexGrid")
	TArray<FHexCoord> GetDeploymentZone(EFactionID Faction) const;

	// Placement d'unités sur la grille
	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	bool PlaceUnit(AUnitBase* Unit, FHexCoord Cell, EFactionID Faction);

	UFUNCTION(BlueprintCallable, Category = "HexGrid")
	void RemoveUnit(FHexCoord Cell);

	UFUNCTION(BlueprintPure, Category = "HexGrid")
	bool IsCellOccupied(FHexCoord Cell) const;

	UFUNCTION(BlueprintPure, Category = "HexGrid")
	AUnitBase* GetUnitAtCell(FHexCoord Cell) const;

	// Voisins d'une cellule hex
	UFUNCTION(BlueprintPure, Category = "HexGrid")
	TArray<FHexCoord> GetNeighbors(FHexCoord Cell) const;

	// Distance hex entre deux cellules
	UFUNCTION(BlueprintPure, Category = "HexGrid")
	int32 HexDistance(FHexCoord A, FHexCoord B) const;

private:
	TMap<EFactionID, TArray<FHexCoord>>            DeploymentZones;
	TMap<FHexCoord, TWeakObjectPtr<AUnitBase>>     OccupiedCells;

	static const FHexCoord HexDirections[6];
};
