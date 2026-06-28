#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Units/UnitBase.h"
#include "WOTOLGreyboxUnit.generated.h"

class UStaticMeshComponent;

// ─────────────────────────────────────────────────────────────────────────────
// UNITÉ "GREYBOX" — forme simple colorée pour tester la logique SANS modèle 3D.
// Aquiloris = cube bleu, Noxéens = cône vert (couleurs officielles FFactionColors).
//
// 100 % ADDITIF : hérite de AUnitBase, ne modifie RIEN du code de base.
// Toute la logique (IA, combat, moral, verticalité) vient de AUnitBase tel quel.
// Remplaçable plus tard par un BP avec un vrai mesh Meshy, sans toucher au code.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLGreyboxUnit : public AUnitBase
{
	GENERATED_BODY()

public:
	AWOTOLGreyboxUnit();

protected:
	virtual void BeginPlay() override;

	// Forme visuelle (cube / cône) — purement cosmétique, pas de collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Greybox")
	TObjectPtr<UStaticMeshComponent> ShapeMesh;
};
