#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGreyboxArena.generated.h"

class AWOTOLGreyboxUnit;

// ─────────────────────────────────────────────────────────────────────────────
// ARÈNE DE TEST "GREYBOX" — auto-bataille de formes simples.
//
// UTILISATION (côté éditeur, ~5 min) :
//   1. Ouvre un niveau avec un SOL (le niveau par défaut convient)
//   2. Pose un NavMeshBoundsVolume au-dessus du sol (sinon l'IA ne se déplace pas)
//   3. Glisse CET acteur (WOTOLGreyboxArena) dans le niveau
//   4. Appuie sur Play
//
// → Deux armées de formes colorées apparaissent et se battent toutes seules,
//   pilotées par tout le code RTS existant (IA, combat, moral…).
//   Aucun modèle 3D requis. 100 % additif, ne modifie aucun fichier de base.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLGreyboxArena : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLGreyboxArena();

	// Nombre d'unités par armée
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	int32 UnitsPerFaction = 4;

	// Espacement latéral entre unités (UE units)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	float UnitSpacing = 200.f;

	// Distance entre les deux armées
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	float ArmySeparation = 1500.f;

	// Délai avant le lancement de la bataille (laisse le temps au spawn/navmesh)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	float BattleStartDelay = 1.5f;

	// Classe d'unité greybox à spawner (par défaut AWOTOLGreyboxUnit)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	TSubclassOf<AWOTOLGreyboxUnit> GreyboxUnitClass;

protected:
	virtual void BeginPlay() override;

private:
	void SpawnArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing);
	void LaunchBattle();
};
