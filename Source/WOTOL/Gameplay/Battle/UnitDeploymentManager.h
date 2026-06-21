#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "UnitDeploymentManager.generated.h"

class AUnitBase;
class UHexGridManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeploymentReady);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitPlaced, AUnitBase*, Unit, FHexCoord, Cell);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitUnplaced, FHexCoord, Cell);

// Gère la phase de déploiement avant la bataille (placement des unités sur la grille hex)
UCLASS()
class WOTOL_API UUnitDeploymentManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Initialise le manager avec les unités à placer et la faction concernée
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void BeginDeployment(EFactionID Faction, const TArray<AUnitBase*>& UnitsToPlace);

	// Le joueur sélectionne une unité à placer (depuis l'UI)
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void SelectUnitForPlacement(AUnitBase* Unit);

	// Tente de placer l'unité sélectionnée sur une cellule
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	bool PlaceSelectedUnit(FHexCoord Cell);

	// Retire une unité déjà placée (retour en pool)
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	bool UnplaceUnit(FHexCoord Cell);

	// Toutes les unités sont-elles placées ?
	UFUNCTION(BlueprintPure, Category = "Deployment")
	bool IsDeploymentComplete() const;

	// Valide le déploiement et signale que la bataille peut commencer
	UFUNCTION(BlueprintCallable, Category = "Deployment")
	void ConfirmDeployment();

	UFUNCTION(BlueprintPure, Category = "Deployment")
	AUnitBase* GetSelectedUnit() const { return SelectedUnit.Get(); }

	UFUNCTION(BlueprintPure, Category = "Deployment")
	TArray<AUnitBase*> GetUnplacedUnits() const;

	UFUNCTION(BlueprintPure, Category = "Deployment")
	EFactionID GetDeploymentFaction() const { return DeploymentFaction; }

	UPROPERTY(BlueprintAssignable, Category = "Deployment")
	FOnDeploymentReady OnDeploymentConfirmed;

	UPROPERTY(BlueprintAssignable, Category = "Deployment")
	FOnUnitPlaced OnUnitPlaced;

	UPROPERTY(BlueprintAssignable, Category = "Deployment")
	FOnUnitUnplaced OnUnitUnplaced;

private:
	EFactionID DeploymentFaction = EFactionID::None;

	TWeakObjectPtr<AUnitBase> SelectedUnit;

	// Unités non encore placées
	TArray<TWeakObjectPtr<AUnitBase>> UnplacedUnits;

	// Cellule → unité placée
	TMap<FHexCoord, TWeakObjectPtr<AUnitBase>> PlacedUnits;

	UHexGridManager* GetHexGrid() const;
};
