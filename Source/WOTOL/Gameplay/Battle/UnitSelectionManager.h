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
	TArray<TObjectPtr<AUnitBase>> SelectedUnits;

	bool IsValidForSelection(AUnitBase* Unit, EFactionID PlayerFaction) const;
	void AddUnitInternal(AUnitBase* Unit);
};
