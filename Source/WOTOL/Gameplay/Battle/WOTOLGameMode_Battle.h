#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameMode_Battle.generated.h"

class ABattleStateObserver;
class UTacticalPhaseManager;
class UTerritoryStateManager;
class AWOTOLBattleCamera;
class AWOTOLUnitSpawner;
class UBattleConfigDataAsset;

UCLASS()
class WOTOL_API AWOTOLGameMode_Battle : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWOTOLGameMode_Battle();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// Config statique de la bataille — assignée dans le WorldSettings du niveau
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle|Config")
	TObjectPtr<UBattleConfigDataAsset> BattleConfig;

	// Distribue le profil joueur à tous les AIControllers ennemis
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void BroadcastPlayerProfileToAI();

	// Exposé pour que les spawners puissent se référencer
	UFUNCTION(BlueprintPure, Category = "Battle")
	UTacticalPhaseManager* GetPhaseManager() const { return PhaseManager; }

protected:
	UPROPERTY()
	TObjectPtr<ABattleStateObserver> StateObserver;

	UPROPERTY()
	TObjectPtr<UTacticalPhaseManager> PhaseManager;

	UPROPERTY()
	TObjectPtr<UTerritoryStateManager> TerritoryManager;

	UPROPERTY()
	TObjectPtr<AWOTOLBattleCamera> BattleCamera;

private:
	void SetupBattleFromGameInstance();
	void SpawnBattleCamera();
	void TriggerUnitSpawners();
	void StartBattle();

	UFUNCTION()
	void OnBattlePhaseChanged(EBattlePhase NewPhase);

	UFUNCTION()
	void OnVictoryConditionMet(EFactionID Winner, EBattleResult Result);

	UFUNCTION()
	void OnTacticalWindowOpened(EFactionID Faction);

	UFUNCTION()
	void OnTacticalWindowClosed(EFactionID Faction);

	UFUNCTION()
	void OnTerritoryCaptured(EFactionID Faction, FTerritoryGrade Grade);

	UFUNCTION()
	void OnCaptureProgress(EFactionID Faction, float Progress);
};
