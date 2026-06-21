#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameInstance.generated.h"

// GameInstance persistant entre tous les niveaux
// Porte la config de session complète (réglages → héros → unités → exploration → bataille)
UCLASS()
class WOTOL_API UWOTOLGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// ─── Config de session (remplie par les écrans de menu) ──────────────────

	UPROPERTY(BlueprintReadWrite, Category = "Session")
	FSessionConfig SessionConfig;

	// Raccourcis fréquents
	UFUNCTION(BlueprintPure, Category = "Session")
	EFactionID GetSelectedFaction() const { return SessionConfig.SelectedFaction; }

	UFUNCTION(BlueprintPure, Category = "Session")
	EDifficultyLevel GetDifficulty() const { return SessionConfig.Difficulty; }

	UFUNCTION(BlueprintPure, Category = "Session")
	bool IsIronmanMode() const { return SessionConfig.bIronmanMode; }

	// Écrit la session dans le SaveGame et sauvegarde
	UFUNCTION(BlueprintCallable, Category = "Session")
	void CommitSessionToSave();

	// Restaure la session depuis le SaveGame au démarrage
	UFUNCTION(BlueprintCallable, Category = "Session")
	void RestoreSessionFromSave();

	// ─── Compatibilité ancien code ────────────────────────────────────────────
	UFUNCTION(BlueprintPure, Category = "Session")
	EFactionID GetSelectedFactionLegacy() const { return SessionConfig.SelectedFaction; }

	UFUNCTION(BlueprintPure, Category = "Session")
	const FHeroLoadout& GetHeroLoadout() const { return SessionConfig.HeroLoadout; }
};
