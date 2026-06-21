#include "SaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"

void USaveGameSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadGame();
}

void USaveGameSubsystem::SaveGame()
{
	if (!CurrentSave)
	{
		CurrentSave = Cast<UWOTOLSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UWOTOLSaveGame::StaticClass()));
	}

	UGameplayStatics::SaveGameToSlot(
		CurrentSave, UWOTOLSaveGame::SaveSlotName, UWOTOLSaveGame::UserIndex);
}

void USaveGameSubsystem::LoadGame()
{
	if (UGameplayStatics::DoesSaveGameExist(
			UWOTOLSaveGame::SaveSlotName, UWOTOLSaveGame::UserIndex))
	{
		CurrentSave = Cast<UWOTOLSaveGame>(UGameplayStatics::LoadGameFromSlot(
			UWOTOLSaveGame::SaveSlotName, UWOTOLSaveGame::UserIndex));
	}
	else
	{
		CurrentSave = Cast<UWOTOLSaveGame>(
			UGameplayStatics::CreateSaveGameObject(UWOTOLSaveGame::StaticClass()));
	}

	OnSaveGameLoaded.Broadcast(CurrentSave);
}
