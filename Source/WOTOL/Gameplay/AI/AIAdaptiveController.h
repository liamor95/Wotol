#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/WOTOLTypes.h"
#include "AIAdaptiveController.generated.h"

class UFactionRegistrySubsystem;

// IA adaptative par faction
// S'abonne au TacticalPhaseManager pour agir uniquement pendant sa fenêtre
// Interroge le FactionRegistry — pas de GetAllActorsOfClass
// Exécution individuelle des unités = Behavior Tree/EQS standard
UCLASS()
class WOTOL_API UAIAdaptiveController : public AAIController
{
	GENERATED_BODY()

public:
	UAIAdaptiveController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetControlledFaction(EFactionID InFaction);

	// Injecte le profil joueur reçu du PlayerProfileSubsystem
	UFUNCTION(BlueprintCallable, Category = "AI")
	void UpdatePlayerProfile(const FPlayerBehaviorProfile& Profile);

	// Paramètres d'agressivité ajustés dynamiquement
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
	void ActivateAI();
	void DeactivateAI();

	bool bIsActive = false;
};
