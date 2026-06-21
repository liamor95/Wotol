#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/WOTOLTypes.h"
#include "AIAdaptiveController.generated.h"

class UUnitAIStateComponent;

// IA adaptative par faction
// Active/désactive la machine d'états UnitAIStateComponent selon la fenêtre tactique
// Reçoit les ordres joueur (move/attack) et les transmet à la state machine
UCLASS()
class WOTOL_API UAIAdaptiveController : public AAIController
{
	GENERATED_BODY()

public:
	UAIAdaptiveController();

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetControlledFaction(EFactionID InFaction);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdatePlayerProfile(const FPlayerBehaviorProfile& Profile);

	// Ordres reçus du PlayerController_Battle
	UFUNCTION(BlueprintCallable, Category = "AI")
	void IssueMoveCommand(FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void IssueAttackCommand(class AUnitBase* TargetUnit);

	// Paramètres d'agressivité calculés dynamiquement
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float ComputedAggressionLevel = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float ComputedCautionLevel = 0.5f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EFactionID ControlledFaction = EFactionID::None;

private:
	UFUNCTION()
	void OnTacticalWindowOpened(EFactionID Faction);

	UFUNCTION()
	void OnTacticalWindowClosed(EFactionID Faction);

	void AdaptToPlayerProfile(const FPlayerBehaviorProfile& Profile);
	void SetAIStateActive(bool bActive);

	UUnitAIStateComponent* GetStateComponent() const;

	bool bIsActive = false;
};
