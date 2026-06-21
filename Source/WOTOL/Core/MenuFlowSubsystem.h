#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "MenuFlowSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMenuStepChanged, EMenuStep, NewStep, EMenuStep, PreviousStep);

// Gère la navigation entre les étapes de menu (1-MainMenu → ... → 11-Introduction → Partie)
// Blueprint UMG s'abonne à OnMenuStepChanged pour afficher/masquer les écrans
UCLASS()
class WOTOL_API UMenuFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "MenuFlow")
	void GoToStep(EMenuStep Step);

	UFUNCTION(BlueprintCallable, Category = "MenuFlow")
	void GoNext();

	UFUNCTION(BlueprintCallable, Category = "MenuFlow")
	void GoBack();

	UFUNCTION(BlueprintPure, Category = "MenuFlow")
	EMenuStep GetCurrentStep() const { return CurrentStep; }

	UFUNCTION(BlueprintPure, Category = "MenuFlow")
	bool CanGoBack() const;

	// Démarre la partie (charge le niveau d'exploration)
	UFUNCTION(BlueprintCallable, Category = "MenuFlow")
	void StartGame(TSoftObjectPtr<UWorld> ExplorationLevel);

	// Charge une partie sauvegardée et va directement en exploration
	UFUNCTION(BlueprintCallable, Category = "MenuFlow")
	void LoadSavedGame(TSoftObjectPtr<UWorld> ExplorationLevel);

	UPROPERTY(BlueprintAssignable, Category = "MenuFlow")
	FOnMenuStepChanged OnMenuStepChanged;

private:
	EMenuStep CurrentStep = EMenuStep::MainMenu;

	// Ordre linéaire des étapes (nouvelle partie)
	static const TArray<EMenuStep> NewGameFlow;

	int32 GetFlowIndex(EMenuStep Step) const;
};
