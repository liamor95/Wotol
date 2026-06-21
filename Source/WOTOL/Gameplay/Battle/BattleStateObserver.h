#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "BattleStateObserver.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBattleStateChanged, EBattlePhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVictoryConditionMet, EFactionID, Winner, EBattleResult, Result);

// Surveille la bataille et diffuse les transitions d'état
// S'abonne au FactionRegistrySubsystem — pas de scan du monde
UCLASS()
class WOTOL_API ABattleStateObserver : public AActor
{
	GENERATED_BODY()

public:
	ABattleStateObserver();

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void BeginObserving();

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void StopObserving();

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetBattlePhase(EBattlePhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Battle")
	EBattlePhase GetBattlePhase() const { return CurrentPhase; }

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FOnBattleStateChanged OnBattleStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FOnVictoryConditionMet OnVictoryConditionMet;

	// Factions en jeu pour cette bataille (renseigné par le GameMode)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Battle")
	TArray<EFactionID> ActiveFactions;

protected:
	virtual void BeginPlay() override;

private:
	void CheckVictoryConditions();

	UFUNCTION()
	void OnUnitUnregistered(class AUnitBase* Unit, EFactionID Faction);

	EBattlePhase CurrentPhase = EBattlePhase::Preparation;
};
