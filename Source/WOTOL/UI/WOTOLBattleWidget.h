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

	// ---- Events pour animer le widget ----
	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnTurnChanged(EFactionID NewFaction);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnBattleEnded(EBattleResult Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnCaptureProgressChanged(float NewProgress);

	UFUNCTION(BlueprintImplementableEvent, Category = "Battle|UI")
	void OnSelectionChanged();

protected:
	virtual void NativeOnInitialized() override;

private:
	void SubscribeToGameState();
};
