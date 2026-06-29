#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLDemoDirector.generated.h"

class AWOTOLDemoUnit;
class UUnitDataAsset;

// ─────────────────────────────────────────────────────────────────────────────
// DIRECTOR DE DÉMO (Stage 2) — assemble une bataille greybox jouable selon la
// phase courante et le roster débloqué, puis lance la bataille RTS existante.
//
// UTILISATION ÉDITEUR (minimal) :
//   1. Ouvre un niveau avec un sol + un NavMeshBoundsVolume au-dessus
//   2. Glisse cet acteur (WOTOLDemoDirector) dans le niveau
//   3. Choisis la faction via le menu (ou DefaultPlayerFaction ci-dessous)
//   4. Play → deux camps de formes contextualisées apparaissent et se battent
//
// Respecte le roster du cahier des charges : chef + infanterie + montée, et
// distance UNIQUEMENT si débloquée. Spéciale + mythique restent verrouillées.
// CreatureEncounter = 1 créature massive (réutilise les stats du mythique rival).
// RivalDefense       = petite escouade de la faction rivale.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLDemoDirector();

	// Faction joueur par défaut si le GameInstance n'en a pas (test direct)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	EFactionID DefaultPlayerFaction = EFactionID::Aquiloris;

	// Quantités du roster (valeurs du cahier des charges)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 InfantryCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 MountedCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 RangedCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float UnitSpacing = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float ArmySeparation = 2500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float BattleStartDelay = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TSubclassOf<AWOTOLDemoUnit> DemoUnitClass;

	// Lance/relance la bataille de la phase courante
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartCurrentBattle();

protected:
	virtual void BeginPlay() override;

private:
	EFactionID ResolvePlayerFaction() const;
	EFactionID RivalOf(EFactionID Faction) const;

	void SpawnPlayerArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing);
	void SpawnEnemyForCreature(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing);
	void SpawnRivalSquad(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing);

	AWOTOLDemoUnit* SpawnUnit(FName UnitID, const FVector& Loc, const FRotator& Facing, float ScaleBoost);
	void LaunchBattle();
};
