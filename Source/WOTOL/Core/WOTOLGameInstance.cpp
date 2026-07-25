#include "WOTOLGameInstance.h"
#include "SaveGameSubsystem.h"
#include "WOTOLSaveGame.h"

void UWOTOLGameInstance::Init()
{
	Super::Init();
}

void UWOTOLGameInstance::Shutdown()
{
	CommitSessionToSave();
	Super::Shutdown();
}

void UWOTOLGameInstance::CommitSessionToSave()
{
	USaveGameSubsystem* SaveSys = GetSubsystem<USaveGameSubsystem>();
	if (!SaveSys) return;

	UWOTOLSaveGame* Save = SaveSys->GetSaveGame();
	if (!Save) return;

	Save->PlayerName       = SessionConfig.PlayerName;
	Save->SelectedFaction  = SessionConfig.SelectedFaction;
	Save->GameMode         = SessionConfig.GameMode;
	Save->Difficulty       = SessionConfig.Difficulty;
	Save->bIronmanMode     = SessionConfig.bIronmanMode;
	Save->bRandomMode      = SessionConfig.bRandomMode;
	Save->HeroLoadout      = SessionConfig.HeroLoadout;
	Save->HeroAppearance   = SessionConfig.HeroAppearance;

	Save->SelectedSquadPaths.Empty();
	for (const TSoftObjectPtr<UUnitDataAsset>& Unit : SessionConfig.SelectedSquad)
	{
		Save->SelectedSquadPaths.Add(Unit.ToSoftObjectPath());
	}

	SaveSys->SaveGame();
}

void UWOTOLGameInstance::RestoreSessionFromSave()
{
	USaveGameSubsystem* SaveSys = GetSubsystem<USaveGameSubsystem>();
	if (!SaveSys) return;

	const UWOTOLSaveGame* Save = SaveSys->GetSaveGame();
	if (!Save) return;

	SessionConfig.PlayerName      = Save->PlayerName;
	SessionConfig.SelectedFaction = Save->SelectedFaction;
	SessionConfig.GameMode        = Save->GameMode;
	SessionConfig.Difficulty      = Save->Difficulty;
	SessionConfig.bIronmanMode    = Save->bIronmanMode;
	SessionConfig.bRandomMode     = Save->bRandomMode;
	SessionConfig.HeroLoadout     = Save->HeroLoadout;
	SessionConfig.HeroAppearance  = Save->HeroAppearance;

	SessionConfig.SelectedSquad.Empty();
	for (const FSoftObjectPath& Path : Save->SelectedSquadPaths)
	{
		SessionConfig.SelectedSquad.Add(TSoftObjectPtr<UUnitDataAsset>(Path));
	}
}
