#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "RTSBattleManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattlePhaseChanged, EBattlePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleTimerTick,    float, SecondsRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBattleEnded, EFactionID, Winner, EBattleResult, Result);

// Gestionnaire de bataille temps réel — remplace l'ex-TacticalPhaseManager tour par tour
// Inspire de Total War / Bannerlord :
//   Deployment → joueur place ses unités sur la grille hex
//   Tactical   → combat RTS temps réel, toutes les IA actives simultanément
//   Resolution → fin de bataille, calcul résultats
//
// PAS de tour par faction. PAS de fenêtre tactique. Tout tourne en continu.
UCLASS()
class WOTOL_API URTSBattleManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ─── Phases de bataille ───────────────────────────────────────────────────

	// Phase déploiement : joueur place ses unités sur la grille hex
	UFUNCTION(BlueprintCallable, Category = "Battle|RTS")
	void StartDeploymentPhase();

	// Démarre le combat temps réel — active toutes les IA ennemies simultanément
	UFUNCTION(BlueprintCallable, Category = "Battle|RTS")
	void StartBattlePhase(float BattleDurationSeconds = 3600.f);

	// Fin de bataille (victoire / défaite / temps écoulé)
	UFUNCTION(BlueprintCallable, Category = "Battle|RTS")
	void EndBattle(EFactionID Winner, EBattleResult Result);

	// ─── Lecture ──────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Battle|RTS")
	EBattlePhase GetCurrentPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintPure, Category = "Battle|RTS")
	float GetTimeRemaining() const { return TimeRemaining; }

	UFUNCTION(BlueprintPure, Category = "Battle|RTS")
	bool IsBattleActive() const { return CurrentPhase == EBattlePhase::Tactical; }

	UFUNCTION(BlueprintPure, Category = "Battle|RTS")
	FText GetFormattedTime() const;

	// ─── Délégués ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Battle|RTS")
	FOnBattlePhaseChanged OnBattlePhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "Battle|RTS")
	FOnBattleTimerTick OnBattleTimerTick;

	UPROPERTY(BlueprintAssignable, Category = "Battle|RTS")
	FOnBattleEnded OnBattleEnded;

private:
	EBattlePhase CurrentPhase   = EBattlePhase::Preparation;
	float        TimeRemaining  = 3600.f;
	bool         bBattleEnded   = false;

	// Un seul timer pour tout le batch RTS (1s interval)
	FTimerHandle BattleTickHandle;

	void BattleTick();
	void SetPhase(EBattlePhase NewPhase);
	void ActivateAllEnemyAI();
};
