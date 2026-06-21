#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "BattleConfigDataAsset.generated.h"

// Résout la question ouverte #3 du CLAUDE.md
// Config statique de la bataille — instancié par niveau, transmis via GameInstance
UCLASS(BlueprintType)
class WOTOL_API UBattleConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// Durée d'une fenêtre tactique par faction (secondes)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing")
	float TacticalWindowDuration = 30.f;

	// Vitesse de capture du territoire par seconde
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	float CaptureRatePerSecond = 10.f;

	// Grade de territoire cible pour cette bataille (démo = 1)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory", meta = (ClampMin = "1", ClampMax = "4"))
	int32 TargetTerritoryGrade = 1;

	// Progression requise pour capturer le territoire
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	float CaptureProgressRequired = 100.f;

	// Rayon autour du territoire où les unités contribuent à la capture
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	float CaptureZoneRadius = 800.f;

	// Nombre d'unités max par faction en démo
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Units")
	int32 MaxUnitsPerFaction = 6;

	// Porcentage de PV pour déclencher la retraite de l'IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AIRetreatHealthThreshold = 0.25f;

	// Distance de détection des ennemis par l'IA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	float AIEnemySightRange = 1500.f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("BattleConfig", GetFName());
	}
};
