#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLHUD.generated.h"

// Classe de base C++ du HUD — le widget UMG est géré en Blueprint
UCLASS()
class WOTOL_API AWOTOLHUD : public AHUD
{
	GENERATED_BODY()

public:
	// Appelé par le GameMode quand une fenêtre tactique s'ouvre
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnTacticalWindowOpened(EFactionID Faction, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnTacticalWindowClosed(EFactionID Faction);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCaptureProgressUpdated(EFactionID Faction, float Progress);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnBattleResultDisplayed(EBattleResult Result);
};
