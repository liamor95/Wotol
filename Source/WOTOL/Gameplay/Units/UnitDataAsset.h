#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "UnitDataAsset.generated.h"

// Data Asset pour une unité — source de vérité de TOUTES les stats
// Une instance par type d'unité (ex: DA_Aquiloryons, DA_Noxeflare...)
// JAMAIS hardcoder les stats dans le code
UCLASS(BlueprintType)
class WOTOL_API UUnitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ─── Identité ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EFactionID Faction = EFactionID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EUnitRole Role = EUnitRole::Infanterie;

	// ─── Stats de combat (correspond à S_UnitData du GDD) ─────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FUnitStats Stats;

	// ─── Classe à instancier en jeu ──────────────────────────────────────────

	// Blueprint de l'unité (hérite de BP_UnitBase / AUnitBase) — spawné par le UnitSpawner
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSoftClassPtr<class AUnitBase> UnitClass;

	// ─── Mesh & VFX (à assigner dans l'éditeur, jamais en code) ──────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<class USkeletalMesh> UnitMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<class UAnimBlueprint> AnimBP;

	// Classe du projectile (si AttackType == Ranged)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSoftClassPtr<AActor> ProjectileClass;

	// Portrait unité pour le HUD (WBP_BattleHUD)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<class UTexture2D> Portrait;

	// Icône unité (roster bar)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<class UTexture2D> Icon;

	// ─── EFFETS SPÉCIAUX (Niagara — ex. assets Fab) ────────────────────────────────
	// Glisse ici le système Niagara importé (depuis Content/Factions/<Faction>/<Unite>/VFX).
	// Le C++ le joue AUTOMATIQUEMENT au bon moment — AUCUN Blueprint requis, juste l'assigner.

	// Joué SUR LA CIBLE quand l'unité porte un COUP DE BASE (impact d'attaque).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TSoftObjectPtr<class UNiagaraSystem> AttackImpactVFX;

	// Joué SUR L'UNITÉ quand elle attaque (bouche/canon : flash de tir, éclat de lame…).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TSoftObjectPtr<class UNiagaraSystem> MuzzleVFX;

	// Joué SUR L'UNITÉ quand elle lance sa COMPÉTENCE spéciale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TSoftObjectPtr<class UNiagaraSystem> AbilityVFX;

	// ─── Compétences ───────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AbilityDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisOneName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisOneDescription;

	// Thématique de l'Axe 1 (Offensif / Défensif / Support / Support Dégâts / Support Soin /
	// Soins / Contrôle...) — affichée dans l'onglet Compétences à la place d'un générique
	// "Axe 1" (clarification Liamor du 29/07/2026, revue unité par unité des 12 axes).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisOneCategory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisTwoName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisTwoDescription;

	// Thématique de l'Axe 2 — voir AxisOneCategory.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisTwoCategory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText PassiveDescription;

	// Classes de compétence Blueprint (à créer en BP héritant UAbilityBase)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TArray<TSoftClassPtr<UObject>> AbilityClasses;

	// ── Télégraphie visuelle (Grade/Axe, demande Liamor 29/07/2026) ────────────────────────
	// Vrai = un aperçu holographique de la zone d'effet (AWOTOLZoneTelegraph) est affiché
	// AVANT l'activation réelle de la compétence. Utilisé par Noxeflare (le joueur doit voir la
	// zone impactée avant de subir/déclencher le flash).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	bool bAbilityHasTelegraph = false;

	// Vrai = quand l'Axe 2 est choisi (Grade 1+), l'exécution de zone spawn en plus un voile
	// sombre persistant (AWOTOLZoneTelegraph, bAppliesBlindToEnemies) façon jet d'encre du
	// Kraken, ~3x la taille de l'unité — Aquilombres "Ombres Projetées" uniquement.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	bool bAxisTwoSpawnsShadowVeil = false;

	// Vrai = une fois l'Axe choisi (Grade 1+), la PORTÉE D'ATTAQUE effective (pas l'ability)
	// change réellement selon l'axe (voir AUnitBase::GetEffectiveAttackRange) — Aquisphères
	// Hydrosniper (+portée) / Hydropompe (-portée) uniquement.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	bool bAxisAffectsAttackRange = false;

	// ─── Recrutement ───────────────────────────────────────────────────────────

	// Quota max dans l'escouade (ex: 3 pour Aquisphères, 1 pour Léviaphénix)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recruitment", meta = (ClampMin = "1"))
	int32 MaxCountInSquad = 1;

	// Bâtiment requis pour recruter
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recruitment")
	FText RequiredBuilding;
};
