#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WOTOLSaveGame.h"
#include "SaveGameSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameLoaded, UWOTOLSaveGame*, SaveGame);

UCLASS()
class WOTOL_API USaveGameSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveGame();

	UFUNCTION(BlueprintCallable, Category = "Save")
	void LoadGame();

	UFUNCTION(BlueprintCallable, Category = "Save")
	UWOTOLSaveGame* GetSaveGame() const { return CurrentSave; }

	UPROPERTY(BlueprintAssignable, Category = "Save")
	FOnSaveGameLoaded OnSaveGameLoaded;

private:
	UPROPERTY()
	UWOTOLSaveGame* CurrentSave = nullptr;
};
