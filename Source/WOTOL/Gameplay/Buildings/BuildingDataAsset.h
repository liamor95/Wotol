#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "BuildingDataAsset.generated.h"

UENUM(BlueprintType)
enum class EBuildingCategory : uint8
{
	Production   UMETA(DisplayName = "Production (ressources)"),
	Recrutement  UMETA(DisplayName = "Recrutement"),
	Defense      UMETA(DisplayName = "Défense"),
	Commandement UMETA(DisplayName = "Commandement"),
	Recherche    UMETA(DisplayName = "Recherche")
};

// Coût de construction en ressources
USTRUCT(BlueprintType)
struct FBuildingCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EResourceType ResourceType = EResourceType::BiomasseMarine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 Amount = 0;
};

// Modificateur de stat appliqué aux unités recrutées dans ce bâtiment
USTRUCT(BlueprintType)
struct FBuildingUnitBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUnitRole TargetRole = EUnitRole::Infanterie;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealthBonus = 0.f;      // +PV%

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackBonus = 0.f;      // +ATK%

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefenseBonus = 0.f;     // +DEF pts
};

// DataAsset source de vérité pour un type de bâtiment
// Scope démo : Aquiloris uniquement (10 bâtiments)
UCLASS(BlueprintType)
class WOTOL_API UBuildingDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ─── Identité ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EFactionID Faction = EFactionID::Aquiloris;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EBuildingCategory Category = EBuildingCategory::Production;

	// ─── Construction ──────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction")
	TArray<FBuildingCost> ConstructionCost;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction",
		meta = (ClampMin = "0"))
	float ConstructionTime = 30.f;

	// PV du bâtiment
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Construction",
		meta = (ClampMin = "1"))
	int32 MaxHealth = 500;

	// ─── Production ────────────────────────────────────────────────────────────

	// Ressource générée chaque tick (si Category == Production)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production")
	EResourceType ProducedResource = EResourceType::Cristaux;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production",
		meta = (ClampMin = "0"))
	int32 ProductionPerTick = 0;

	// Intervalle entre deux ticks de production (secondes)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Production")
	float ProductionInterval = 10.f;

	// ─── Recrutement ───────────────────────────────────────────────────────────

	// Rôles d'unité que ce bâtiment peut recruter
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recruitment")
	TArray<EUnitRole> RecruitableRoles;

	// Bonus appliqués aux unités recrutées ici
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recruitment")
	TArray<FBuildingUnitBonus> UnitBonuses;

	// ─── Visuels ───────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<class UStaticMesh> BuildingMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals")
	TSoftObjectPtr<class UTexture2D> Icon;

	// ─── Prérequis ─────────────────────────────────────────────────────────────

	// Bâtiments qui doivent exister avant de pouvoir construire celui-ci
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prerequisites")
	TArray<TSoftObjectPtr<UBuildingDataAsset>> RequiredBuildings;

	// Grade de territoire minimum requis
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Prerequisites",
		meta = (ClampMin = "0", ClampMax = "4"))
	int32 RequiredTerritoryGrade = 0;
};
