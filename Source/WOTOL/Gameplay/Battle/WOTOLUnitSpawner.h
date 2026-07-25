#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLUnitSpawner.generated.h"

class UUnitDataAsset;

// Placé dans le niveau bataille par Liamor — configure et spawne les unités
// Liamor n'a qu'à drag-dropper cet actor, assigner la faction et les DataAssets
UCLASS()
class WOTOL_API AWOTOLUnitSpawner : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLUnitSpawner();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	EFactionID Faction = EFactionID::None;

	// Unités à spawner (liste ordonnée)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	TArray<TObjectPtr<UUnitDataAsset>> UnitsToSpawn;

	// Espacement entre les unités en formation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float FormationSpacing = 200.f;

	// Spawne toutes les unités configurées — appelé par le GameMode_Battle
	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void SpawnUnits();

	// Événement Blueprint optionnel pour animer l'apparition
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawn")
	void OnUnitSpawned(class AUnitBase* Unit);

protected:
	virtual void BeginPlay() override;

private:
	FVector GetFormationSlot(int32 Index, int32 Total) const;
};
