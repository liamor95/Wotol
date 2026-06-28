#include "WOTOLGameMode_Hub.h"
#include "Core/WOTOLGameInstance.h"
#include "Kismet/GameplayStatics.h"

void AWOTOLGameMode_Hub::BeginPlay()
{
	Super::BeginPlay();
	ReadResultsFromGameInstance();
	OnHubReady(PlayerFaction, LastBattleResult);
}

void AWOTOLGameMode_Hub::ReadResultsFromGameInstance()
{
	if (UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		PlayerFaction     = GI->GetSelectedFaction();
		LastBattleResult  = GI->LastBattleResult;
	}
}

void AWOTOLGameMode_Hub::LaunchBattle(FName BattleLevelName)
{
	if (!BattleLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, BattleLevelName);
	}
}

void AWOTOLGameMode_Hub::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, TEXT("MainMenu"));
}
