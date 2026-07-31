#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "UnitSelectionManager.generated.h"

class AUnitBase;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSelectionChanged);

UCLASS()
class WOTOL_API UUnitSelectionManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Sélectionne une unité (remplace la sélection courante)
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectUnit(AUnitBase* Unit, EFactionID PlayerFaction);

	// Ajoute une unité à la sélection courante
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void AddToSelection(AUnitBase* Unit, EFactionID PlayerFaction);

	// Sélection par boîte (coordonnées écran)
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void BoxSelect(APlayerController* PC, FVector2D ScreenStart,
		FVector2D ScreenEnd, EFactionID PlayerFaction);

	// Sélectionne toutes les unités d'un type pour une faction
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SelectAllOfFaction(EFactionID Faction);

	UFUNCTION(BlueprintCallable, Category = "Selection")
	void ClearSelection();

	UFUNCTION(BlueprintPure, Category = "Selection")
	const TArray<AUnitBase*>& GetSelectedUnits() const { return SelectedUnits; }

	UFUNCTION(BlueprintPure, Category = "Selection")
	bool HasSelection() const { return !SelectedUnits.IsEmpty(); }

	UFUNCTION(BlueprintPure, Category = "Selection")
	int32 GetSelectionCount() const { return SelectedUnits.Num(); }

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnSelectionChanged OnSelectionChanged;

private:
	// UPROPERTY obligatoire : sans elle, ce tableau n'est pas suivi par le garbage collector,
	// et une unite detruite (fin de bataille precedente, mort au combat...) laisse un pointeur
	// pendant au lieu d'etre mis a null -> crash EXCEPTION_ACCESS_VIOLATION au prochain
	// ClearSelection() qui essaie de le dereferencer (plante reel remonte par Liamor le
	// 31/07/2026, juste avant la Phase 3 : la selection de la bataille du Kraken restait dans
	// ce tableau, pointant vers des unites deja detruites, quand BeginPreparation() de la
	// bataille suivante appelait ClearSelection()).
	UPROPERTY()
	TArray<TObjectPtr<AUnitBase>> SelectedUnits;

	bool IsValidForSelection(AUnitBase* Unit, EFactionID PlayerFaction) const;
	void AddUnitInternal(AUnitBase* Unit);
};
