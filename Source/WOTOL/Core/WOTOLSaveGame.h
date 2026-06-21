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
	UPROPERTY(SaveGame, BlueprintReadWrite)
	EFactionID SelectedFaction = EFactionID::None;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FHeroLoadout HeroLoadout;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FPlayerBehaviorProfile BehaviorProfile;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 TerritoryGradeCaptured = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 TotalVictories = 0;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	int32 TotalDefeats = 0;

	static const FString SaveSlotName;
	static const int32 UserIndex;
};
