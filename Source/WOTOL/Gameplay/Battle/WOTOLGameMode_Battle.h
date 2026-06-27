#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameMode_Battle.generated.h"

class ABattleStateObserver;
class AWOTOLBattleCamera;
class AWOTOLUnitSpawner;
class UBattleConfigDataAsset;

// GameMode de bataille RTS temps réel — inspire Total War / Bannerlord
// PAS de tours. Toutes les IA actives en continu dès StartBattlePhase().
// Flux : Preparation → Deployment → Tactical (RTS) → Resolution
UCLASS()
class WOTOL_API AWOTOLGameMode_Battle : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWOTOLGameMode_Battle();

	virtual void BeginPlay() override;

	// Config assignée dans WorldSettings du niveau de bataille
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Battle|Config")
	TObjectPtr<UBattleConfigDataAsset> BattleConfig;

	// Distribue le profil joueur à tous les AIControllers ennemis
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void BroadcastPlayerProfileToAI();

	// Appelé par le Blueprint de déploiement quand le joueur confirme son placement
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void OnDeploymentConfirmed();

protected:
	UPROPERTY()
	TObjectPtr<ABattleStateObserver> StateObserver;

	UPROPERTY()
	TObjectPtr<AWOTOLBattleCamera> BattleCamera;

private:
	void SetupBattleFromGameInstance();
	void SpawnBattleCamera();
	void TriggerUnitSpawners();
	void StartDeploymentPhase();

	UFUNCTION()
	void OnBattlePhaseChanged(EBattlePhase NewPhase);

	UFUNCTION()
	void OnBattleEnded(EFactionID Winner, EBattleResult Result);

	UFUNCTION()
	void OnVictoryConditionMet(EFactionID Winner, EBattleResult Result);
};
