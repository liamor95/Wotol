#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameMode_Hub.generated.h"

// GameMode du Hub (cité post-bataille) — actif entre les batailles
// Liamor branche ici ses Blueprints de construction et de gestion de ressources
UCLASS()
class WOTOL_API AWOTOLGameMode_Hub : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;

	// ─── Navigation ───────────────────────────────────────────────────────────

	// Lance une bataille depuis le hub (charge le niveau de bataille)
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void LaunchBattle(FName BattleLevelName);

	// Retour au menu principal
	UFUNCTION(BlueprintCallable, Category = "Hub")
	void ReturnToMainMenu();

	// ─── Résultats de la bataille précédente ──────────────────────────────────

	// Rempli par le niveau de bataille avant le retour au hub
	UPROPERTY(BlueprintReadOnly, Category = "Hub")
	EBattleResult LastBattleResult = EBattleResult::None;

	UPROPERTY(BlueprintReadOnly, Category = "Hub")
	EFactionID PlayerFaction = EFactionID::None;

	// ─── Évènements Blueprint ─────────────────────────────────────────────────

	// Déclenché quand le hub est prêt — branche les BP de construction ici
	UFUNCTION(BlueprintImplementableEvent, Category = "Hub")
	void OnHubReady(EFactionID Faction, EBattleResult PreviousResult);

protected:
	void ReadResultsFromGameInstance();
};
