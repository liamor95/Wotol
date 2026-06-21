#include "WOTOLGameMode_MainMenu.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Core/MenuFlowSubsystem.h"
#include "Core/SaveGameSubsystem.h"

AWOTOLGameMode_MainMenu::AWOTOLGameMode_MainMenu()
{
	DefaultPawnClass = nullptr;
}

void AWOTOLGameMode_MainMenu::BeginPlay()
{
	Super::BeginPlay();

	if (MainMenuWidgetClass)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(GetWorld(), MainMenuWidgetClass);
		if (MainMenuWidget) MainMenuWidget->AddToViewport();
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	// Détecter si une sauvegarde existe
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveGameSubsystem* SGS = GI->GetSubsystem<USaveGameSubsystem>())
		{
			const bool bHasSave = SGS->HasSaveGame();
			OnSaveDetected(bHasSave);
		}
	}

	OnMenuOpened();
}

void AWOTOLGameMode_MainMenu::StartNewGame()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMenuFlowSubsystem* Flow = GI->GetSubsystem<UMenuFlowSubsystem>())
		{
			Flow->GoToStep(EMenuStep::GameModeSelection);
		}
	}
}

void AWOTOLGameMode_MainMenu::ContinueGame()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USaveGameSubsystem* SGS = GI->GetSubsystem<USaveGameSubsystem>())
		{
			SGS->LoadGame();
		}
		if (UMenuFlowSubsystem* Flow = GI->GetSubsystem<UMenuFlowSubsystem>())
		{
			Flow->LoadSavedGame(ExplorationLevel);
		}
	}
}

void AWOTOLGameMode_MainMenu::OpenOptions()
{
	// Blueprint UMG gère l'affichage des options
}

void AWOTOLGameMode_MainMenu::OpenCredits()
{
	// Blueprint UMG gère les crédits
}

void AWOTOLGameMode_MainMenu::QuitGame()
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UKismetSystemLibrary::QuitGame(GetWorld(), PC, EQuitPreference::Quit, false);
	}
}
