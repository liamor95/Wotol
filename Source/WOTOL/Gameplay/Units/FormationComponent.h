#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FormationComponent.generated.h"

class AUnitBase;

UENUM(BlueprintType)
enum class EFormationType : uint8
{
	None,            // Pas de formation (mouvement individuel)
	Line,            // Ligne de bataille — Total War classic
	Wedge,           // Coin — percée de formation, Aquilances spécialité
	DefensiveSquare, // Carré défensif — bonus DEF, bonus contre charge
	Loose,           // Formation lâche — réduit dégâts AoE, Aquilombres
	Column           // Colonne de marche — vitesse max, DEF réduite
};

// Appliqué au commandant (chef d'escouade) — les unités suivantes maintiennent
// leurs positions relatives en formation
// Inspire Total War : les unités bougent ensemble, gardent le contact
UCLASS(ClassGroup = "WOTOL", meta = (BlueprintSpawnableComponent))
class WOTOL_API UFormationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFormationComponent();

	// ─── Configuration ────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void SetFormation(EFormationType NewFormation);

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void AddUnitToFormation(AUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void RemoveUnitFromFormation(AUnitBase* Unit);

	UFUNCTION(BlueprintCallable, Category = "Formation")
	void ClearFormation();

	// ─── Mise à jour ──────────────────────────────────────────────────────────

	// Appelé par le PlayerController quand le groupe se déplace
	UFUNCTION(BlueprintCallable, Category = "Formation")
	void UpdateFormationPositions(FVector LeaderDestination, FRotator LeaderFacing);

	// Retourne la destination calculée pour l'unité à l'index donné
	UFUNCTION(BlueprintPure, Category = "Formation")
	FVector GetSlotDestination(int32 UnitIndex) const;

	// ─── Bonus de formation ───────────────────────────────────────────────────

	// Bonus DEF additionnel si la formation est maintenue (unités en place)
	UFUNCTION(BlueprintPure, Category = "Formation")
	float GetFormationDefenseBonus() const;

	// Multiplicateur vitesse de la formation
	UFUNCTION(BlueprintPure, Category = "Formation")
	float GetFormationSpeedMultiplier() const;

	// Réduit les dégâts AoE reçus (formation lâche)
	UFUNCTION(BlueprintPure, Category = "Formation")
	float GetAoEDamageReduction() const;

	// Vrai si toutes les unités sont à moins de SlotTolerance de leur slot
	UFUNCTION(BlueprintPure, Category = "Formation")
	bool IsFormationIntact() const;

	// ─── Lecture ─────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Formation")
	EFormationType GetFormationType() const { return CurrentFormation; }

	UFUNCTION(BlueprintPure, Category = "Formation")
	int32 GetUnitCount() const { return FormationUnits.Num(); }

	// Tolérance en UE units pour considérer une unité "en position"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
	float SlotTolerance = 150.f;

	// Espacement entre unités (UE units)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Formation")
	float UnitSpacing = 200.f;

private:
	EFormationType CurrentFormation = EFormationType::Line;

	UPROPERTY()
	TArray<TWeakObjectPtr<AUnitBase>> FormationUnits;

	// Slots calculés lors du dernier UpdateFormationPositions
	TArray<FVector> CachedSlots;

	// Calcul des slots par type de formation
	TArray<FVector> ComputeLineSlots(FVector Origin, FRotator Facing, int32 Count) const;
	TArray<FVector> ComputeWedgeSlots(FVector Origin, FRotator Facing, int32 Count) const;
	TArray<FVector> ComputeSquareSlots(FVector Origin, FRotator Facing, int32 Count) const;
	TArray<FVector> ComputeLooseSlots(FVector Origin, FRotator Facing, int32 Count) const;
	TArray<FVector> ComputeColumnSlots(FVector Origin, FRotator Facing, int32 Count) const;
};
