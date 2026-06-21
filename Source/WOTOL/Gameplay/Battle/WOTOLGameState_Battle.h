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

	// Compte à rebours (secondes restantes, affiché 56:37 en haut au centre)
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	float BattleTimeRemaining = 3600.f;

	// Vrai pendant la phase Deployment (placement hex avant le Tactical)
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	bool bIsDeploymentPhase = false;

	// Ressources actuelles par faction (snapshot pour l'UI)
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TMap<EFactionID, int32> PlayerResourceSnapshot;
};
