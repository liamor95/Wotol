#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLHUD.generated.h"

class UWOTOLBattleWidget;

UCLASS()
class WOTOL_API AWOTOLHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	// Classe du widget de bataille — assignée dans le BP héritant
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UWOTOLBattleWidget> BattleWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "HUD")
	UWOTOLBattleWidget* GetBattleWidget() const { return BattleWidget; }

	// ---- Events pour les widgets ----
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnTacticalWindowOpened(EFactionID Faction, float Duration);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnTacticalWindowClosed(EFactionID Faction);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCaptureProgressUpdated(EFactionID Faction, float Progress);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnBattleResultDisplayed(EBattleResult Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnSelectionChanged(const TArray<class AUnitBase*>& SelectedUnits);

	// Dessin de la boîte de sélection (rectangle de drag en cours)
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetBoxSelectState(bool bActive, FVector2D Start, FVector2D Current);

protected:
	UPROPERTY()
	TObjectPtr<UWOTOLBattleWidget> BattleWidget;

	bool      bDrawBoxSelect = false;
	FVector2D BoxStart;
	FVector2D BoxCurrent;
};
