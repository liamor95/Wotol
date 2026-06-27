#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameState_Battle.generated.h"

// GameState de bataille RTS — PAS de ActiveTurnFaction (système tour par tour supprimé)
// Compatible réseau futur (multijoueur Couche 2)
UCLASS()
class WOTOL_API AWOTOLGameState_Battle : public AGameStateBase
{
	GENERATED_BODY()

public:
	// ─── Phase & résultat ──────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EBattlePhase CurrentPhase = EBattlePhase::Preparation;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EBattleResult BattleResult = EBattleResult::None;

	// ─── Factions ──────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID PlayerFaction = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID EnemyFaction = EFactionID::None;

	// ─── Timer RTS (affiché 56:37 dans le HUD) ────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	float BattleTimeRemaining = 3600.f;

	// ─── Déploiement ───────────────────────────────────────────────────────────

	// Vrai pendant la phase hex de déploiement (avant le Tactical)
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	bool bIsDeploymentPhase = false;

	// ─── Territoire (snapshot UI) ──────────────────────────────────────────────

	// Progression de capture de la zone principale (0–100, pour la barre UI)
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	float CaptureProgress = 0.f;

	// ─── Ressources (snapshot pour le HUD) ────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TMap<EFactionID, int32> PlayerResourceSnapshot;
};
