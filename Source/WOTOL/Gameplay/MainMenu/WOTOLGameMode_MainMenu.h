#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WOTOLGameMode_MainMenu.generated.h"

// GameMode du menu principal
// Charge automatiquement le widget Blueprint UMG (à assigner dans l'éditeur)
UCLASS()
class WOTOL_API AWOTOLGameMode_MainMenu : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWOTOLGameMode_MainMenu();

	virtual void BeginPlay() override;

	// Widget Blueprint UMG principal du menu (à assigner dans l'éditeur)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TSubclassOf<class UUserWidget> MainMenuWidgetClass;

	// Niveau d'exploration à charger pour une nouvelle partie
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Menu")
	TSoftObjectPtr<UWorld> ExplorationLevel;

	// Actions exposées à Blueprint (boutons du menu)
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartNewGame();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void ContinueGame();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenOptions();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void OpenCredits();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

	// BlueprintImplementableEvent pour animer les transitions entre écrans
	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	void OnMenuOpened();

	UFUNCTION(BlueprintImplementableEvent, Category = "Menu")
	void OnSaveDetected(bool bHasSave);

private:
	UPROPERTY()
	class UUserWidget* MainMenuWidget = nullptr;
};
