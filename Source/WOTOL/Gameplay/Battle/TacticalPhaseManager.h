#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "TacticalPhaseManager.generated.h"

// Tick centralisé unique pour toutes les fenêtres tactiques
// Interdit les FTimerHandle par unité — toute la logique de tour passe ici
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalWindowOpened, EFactionID, Faction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalWindowClosed, EFactionID, Faction);

UCLASS()
class WOTOL_API UTacticalPhaseManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void StartPhase(const TArray<EFactionID>& TurnOrder, float WindowDurationSeconds);

	UFUNCTION(BlueprintCallable, Category = "Tactical")
	void StopPhase();

	UFUNCTION(BlueprintPure, Category = "Tactical")
	EFactionID GetActiveFaction() const { return ActiveFaction; }

	UFUNCTION(BlueprintPure, Category = "Tactical")
	bool IsPhaseRunning() const { return bRunning; }

	UPROPERTY(BlueprintAssignable, Category = "Tactical")
	FOnTacticalWindowOpened OnTacticalWindowOpened;

	UPROPERTY(BlueprintAssignable, Category = "Tactical")
	FOnTacticalWindowClosed OnTacticalWindowClosed;

private:
	void AdvanceTurn();

	FTimerHandle TurnTimer;   // un seul timer pour tout le système

	TArray<EFactionID> CurrentTurnOrder;
	int32              TurnIndex        = 0;
	EFactionID         ActiveFaction    = EFactionID::None;
	float              WindowDuration   = 30.f;
	bool               bRunning         = false;
};
