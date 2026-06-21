#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/WOTOLTypes.h"
#include "TacticalPhaseManager.generated.h"

// Tick centralisé unique pour toutes les fenêtres tactiques
// Interdit les FTimerHandle par unité — toute la logique de tour passe ici
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalWindowOpened, EFactionID, Faction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalWindowClosed, EFactionID, Faction);

UCLASS()
class WOTOL_API UTacticalPhaseManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* InWorld);
	void Shutdown();

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

	TWeakObjectPtr<UWorld> WorldRef;
	FTimerHandle           TurnTimer;   // un seul timer pour tout le système

	TArray<EFactionID> CurrentTurnOrder;
	int32              TurnIndex        = 0;
	EFactionID         ActiveFaction    = EFactionID::None;
	float              WindowDuration   = 30.f;
	bool               bRunning         = false;
};
