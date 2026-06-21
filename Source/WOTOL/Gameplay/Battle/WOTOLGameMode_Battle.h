#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameMode_Battle.generated.h"

class ABattleStateObserver;
class UTacticalPhaseManager;
class UTerritoryStateManager;

UCLASS()
class WOTOL_API AWOTOLGameMode_Battle : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWOTOLGameMode_Battle();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// Durée d'une fenêtre tactique par faction (secondes)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle|Config")
	float TacticalWindowDuration = 30.f;

protected:
	UPROPERTY()
	TObjectPtr<ABattleStateObserver> StateObserver;

	UPROPERTY()
	TObjectPtr<UTacticalPhaseManager> PhaseManager;

	UPROPERTY()
	TObjectPtr<UTerritoryStateManager> TerritoryManager;

private:
	void SetupBattleFromGameInstance();
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
};
