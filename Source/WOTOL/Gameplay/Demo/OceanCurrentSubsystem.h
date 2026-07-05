#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OceanCurrentSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// COURANT OCÉANIQUE — sens + intensité ALÉATOIRES, régénérés à chaque bataille.
// N'agit que sur les COUCHES HAUTES de verticalité (plus on monte, plus il pousse).
// Applique une dérive horizontale aux unités qui s'y trouvent (joueur, ennemi, Kraken) :
//   - dans le sens du déplacement -> l'unité va plus vite ;
//   - à contre-sens -> elle est freinée/déportée.
// L'IA interroge ce sous-système pour décider si monter est avantageux ou risqué.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API UOceanCurrentSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Régénère un courant aléatoire (nouveau sens + nouvelle intensité).
	UFUNCTION(BlueprintCallable, Category = "Ocean")
	void Regenerate();

	// Vecteur de dérive (unités/s) à une hauteur de couche visuelle donnée.
	// 0 en dessous de LowZ ; monte linéairement jusqu'à l'intensité pleine au sommet.
	FVector GetDriftAt(float LayerZ) const;

	UFUNCTION(BlueprintPure, Category = "Ocean")
	FVector GetDirection() const { return Direction; }

	UFUNCTION(BlueprintPure, Category = "Ocean")
	float GetStrength() const { return Strength; }

	UFUNCTION(BlueprintPure, Category = "Ocean")
	bool IsActive() const { return Strength > 1.f; }

	// Facteur d'intensité [0..1] à une hauteur donnée (pour l'IA et l'affichage).
	float GetFactorAt(float LayerZ) const;

private:
	FVector Direction = FVector(1.f, 0.f, 0.f); // horizontal, normalisé
	float   Strength  = 0.f;                    // unités/s de dérive AU SOMMET

	static constexpr float LowZ = 1000.f;       // en dessous : aucun courant
	static constexpr float TopZ = 2400.f;       // au sommet : courant maximal
};
