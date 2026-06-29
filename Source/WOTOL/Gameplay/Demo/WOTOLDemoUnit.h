#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Units/UnitBase.h"
#include "WOTOLDemoUnit.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

// ─────────────────────────────────────────────────────────────────────────────
// UNITÉ GREYBOX CONTEXTUALISÉE (sections 3-5 du cahier des charges).
// Forme symbolique selon la CATÉGORIE (rôle), TAILLE réelle du GDD, COULEUR de
// faction (bleu Aquiloris / vert Noxéen). Nom lisible. 100 % additif : hérite de
// AUnitBase, ne modifie rien. Remplaçable par un vrai mesh Meshy plus tard.
//
// Formes : Chef = cylindre haut · Infanterie = cylindre · Montée = bloc massif ·
//          Distance = cône orienté · Spéciale = cylindre fin · Mythique = grand bloc.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDemoUnit : public AUnitBase
{
	GENERATED_BODY()

public:
	AWOTOLDemoUnit();

	// Hauteur réelle approximative en mètres (valeurs du GDD/doc), pour l'échelle
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	static float GetUnitHeightMeters(FName UnitID);

	// Si vrai : comportement de créature/boss autonome (cherche, avance, attaque)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	bool bCreatureBrain = false;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UStaticMeshComponent> ShapeMesh;

	// Étiquette flottante : nom de l'unité + PV% (remplace une barre de vie UMG)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UTextRenderComponent> NameTag;

	// Construit la forme greybox (mesh + échelle + couleur) selon rôle/faction/taille
	void BuildGreyboxShape();

private:
	// Matériau dynamique de la forme (pour changer la couleur à la sélection)
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> ShapeMID;

	// Réagit à la sélection joueur : surligne l'unité
	UFUNCTION()
	void HandleSelected(bool bSel);

	// Affiche un chiffre de dégâts flottant quand l'unité perd des PV
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	// Cerveau autonome de créature/boss (cherche l'ennemi, avance, attaque)
	void CreatureBrainTick(float DeltaSeconds);

	float LastKnownHealth = -1.f;
	bool  bCreatureStyled = false;
};
