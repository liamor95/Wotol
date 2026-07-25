#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "BattleTimerManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBattleTimeExpired);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleTimeChanged, float, RemainingSeconds);

// Compte à rebours central de la bataille (60:00 affiché en haut au centre)
UCLASS()
class WOTOL_API UBattleTimerManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Lance le compte à rebours (appelé par GameMode au début de la phase Tactical)
	UFUNCTION(BlueprintCallable, Category = "BattleTimer")
	void StartTimer(float DurationSeconds = 3600.f);

	UFUNCTION(BlueprintCallable, Category = "BattleTimer")
	void PauseTimer();

	UFUNCTION(BlueprintCallable, Category = "BattleTimer")
	void ResumeTimer();

	UFUNCTION(BlueprintCallable, Category = "BattleTimer")
	void StopTimer();

	UFUNCTION(BlueprintPure, Category = "BattleTimer")
	float GetRemainingSeconds() const { return RemainingSeconds; }

	// Format "MM:SS" pour l'affichage HUD
	UFUNCTION(BlueprintPure, Category = "BattleTimer")
	FText GetFormattedTime() const;

	UFUNCTION(BlueprintPure, Category = "BattleTimer")
	bool IsRunning() const { return bRunning; }

	UPROPERTY(BlueprintAssignable, Category = "BattleTimer")
	FOnBattleTimeExpired OnTimeExpired;

	UPROPERTY(BlueprintAssignable, Category = "BattleTimer")
	FOnBattleTimeChanged OnTimeChanged;

	virtual void Deinitialize() override;

private:
	float RemainingSeconds = 0.f;
	bool bRunning = false;

	FTimerHandle TickHandle;

	void Tick();
};
