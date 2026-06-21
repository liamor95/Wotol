#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLSaveGame.generated.h"

UCLASS()
class WOTOL_API UWOTOLSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// ─── Paramètres joueur ────────────────────────────────────────────────────
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EFactionID SelectedFaction = EFactionID::None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EGameMode GameMode = EGameMode::Campagne;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	EDifficultyLevel Difficulty = EDifficultyLevel::Guerrier;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bIronmanMode = false;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	bool bRandomMode = false;

	// ─── Héros & escouade ─────────────────────────────────────────────────────
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FHeroLoadout HeroLoadout;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FHeroAppearance HeroAppearance;

	// Chemins des DataAssets d'unités de l'escouade (soft refs sérialisables)
	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<FSoftObjectPath> SelectedSquadPaths;

	// ─── Progression ──────────────────────────────────────────────────────────
	UPROPERTY(SaveGame, BlueprintReadWrite)
	FPlayerBehaviorProfile BehaviorProfile;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 TerritoryGradeCaptured = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 TotalVictories = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 TotalDefeats = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 BattlesPlayed = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 HeroLevel = 1;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 HeroXP = 0;

	static const FString SaveSlotName;
	static const int32   UserIndex;
};
