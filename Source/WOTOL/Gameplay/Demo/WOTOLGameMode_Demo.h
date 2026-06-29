#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameMode_Demo.generated.h"

class AWOTOLDemoDirector;
class AWOTOLBattleCamera;

// ─────────────────────────────────────────────────────────────────────────────
// GAMEMODE DE DÉMO (greybox bataille) — auto-assemble la scène pour limiter au
// maximum la config éditeur. Au Play :
//   - spawn un WOTOLDemoDirector (qui monte les armées et lance la bataille) ;
//   - spawn une caméra de bataille libre et la fait posséder par le joueur.
//
// ÉTAPES ÉDITEUR (minimales) :
//   1. Ouvre un niveau avec un SOL + un NavMeshBoundsVolume au-dessus (pour l'IA)
//   2. World Settings → GameMode Override = WOTOLGameMode_Demo
//   3. Play → la bataille greybox se joue toute seule
// (La faction joueur vient du menu ; sinon Aquiloris par défaut via le Director.)
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLGameMode_Demo : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWOTOLGameMode_Demo();

	virtual void BeginPlay() override;

	// Position de la caméra spectatrice au lancement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	FVector CameraSpawnLocation = FVector(0.f, 0.f, 200.f);

protected:
	UPROPERTY()
	TObjectPtr<AWOTOLDemoDirector> Director;

	UPROPERTY()
	TObjectPtr<AWOTOLBattleCamera> Camera;
};
