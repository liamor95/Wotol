#pragma once

#include "CoreMinimal.h"
#include "AbilityBase.h"
#include "AbilityBase_Generic.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Sous-classe CONCRÈTE minimale de UAbilityBase (qui est Abstract, donc jamais
// instanciable seule). Aucune logique propre : le comportement de base de
// UAbilityBase::ExecuteAbility() (dégâts sur la cible + soin sur le lanceur)
// suffit à rendre une compétence FONCTIONNELLE.
//
// Sert de repli tant qu'aucune ability Blueprint/C++ dédiée n'existe pour une
// unité (AUnitBase::InitFromDataAsset configure alors Cooldown/Damage/Nom à
// partir du UnitDataAsset — voir Docs/DOCUMENT_MAITRE_WOTOL.md, aucune valeur
// inventée ici). Remplaçable plus tard par des ability spécifiques par unité
// sans casser ce repli (AbilityClasses du DataAsset garde toujours la priorité).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API UAbilityBase_Generic : public UAbilityBase
{
	GENERATED_BODY()
};
