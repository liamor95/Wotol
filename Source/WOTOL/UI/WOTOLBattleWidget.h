#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLBattleWidget.generated.h"

// Classe de base C++ du widget de bataille
// Liamor crée le layout UMG en BP ; la logique est ici
// Pas besoin de recoder la logique en BP — tout est disponible via Getters
UCLASS(Abstract, Blueprintable)
class WOTOL_API UWOTOLBattleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- Données exposées au widget UMG ----

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	float GetCaptureProgress() const;

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	EFactionID GetCapturingFaction() const;

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	EFactionID GetActiveTurnFaction() const;

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	EBattlePhase GetBattlePhase() const;

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	int32 GetPlayerUnitCount() const;

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	int32 GetEnemyUnitCount() const;

	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	TArray<class AUnitBase*> GetSelectedUnits() const;

	// Temps restant en secondes (pour le compteur 56:37 en haut)
	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	float GetTimeRemaining() const;

	// Format "MM:SS" prêt à afficher
	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	FText GetFormattedTime() const;

	// Ressource d'une faction (pour la barre de ressources en haut)
	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	int32 GetResourceAmount(EFactionID Faction, EResourceType Resource) const;

	// Niveau du héros (affiché en bas à gauche)
	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	int32 GetHeroLevel() const;

	// Progression XP du héros (0.0 à 1.0)
	UFUNCTION(BlueprintPure, Category = "Battle|UI")
	float GetHeroXPProgress() const;

	// ---- Events pour animer le widget ----
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnTurnChanged(EFactionID NewFaction);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnBattleEnded(EBattleResult Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnCaptureProgressChanged(float NewProgress);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnSelectionChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnTimeWarning(float RemainingSeconds);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnObjectivePopup(const FText& Title, const FText& Description);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnDeploymentPhaseStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnHeroLevelUp(int32 NewLevel);

protected:
	virtual void NativeOnInitialized() override;

private:
	void SubscribeToGameState();
};
