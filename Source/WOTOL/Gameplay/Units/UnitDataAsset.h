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

	// ─── Compétences ───────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AbilityDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisOneName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisOneDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisTwoName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText AxisTwoDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText PassiveDescription;

	// Classes de compétence Blueprint (à créer en BP héritant UAbilityBase)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TArray<TSoftClassPtr<UObject>> AbilityClasses;

	// ─── Recrutement ───────────────────────────────────────────────────────────

	// Quota max dans l'escouade (ex: 3 pour Aquistance, 1 pour Léviaphénix)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recruitment", meta = (ClampMin = "1"))
	int32 MaxCountInSquad = 1;

	// Bâtiment requis pour recruter
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recruitment")
	FText RequiredBuilding;
};
