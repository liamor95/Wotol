#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameState_Battle.generated.h"

// La logique de state est ici (pas dans GameMode) pour rester compatible multijoueur futur
UCLASS()
class WOTOL_API AWOTOLGameState_Battle : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EBattlePhase CurrentPhase = EBattlePhase::Preparation;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID PlayerFaction = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID EnemyFaction = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID ActiveTurnFaction = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	float CaptureProgress = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EBattleResult BattleResult = EBattleResult::None;
};
