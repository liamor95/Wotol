#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLPlayerController_Battle.generated.h"

class UUnitSelectionManager;
class AUnitBase;
class AWOTOLBattleCamera;

// Gère toutes les inputs de la bataille :
// - Clic gauche = sélection / drag = boîte de sélection
// - Clic droit = ordre (déplacement ou attaque selon la cible)
// - WASD/flèches = pan caméra (délégué à AWOTOLBattleCamera)
// - Molette = zoom caméra
UCLASS()
class WOTOL_API AWOTOLPlayerController_Battle : public APlayerController
{
	GENERATED_BODY()

public:
	AWOTOLPlayerController_Battle();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetPlayerFaction(EFactionID Faction);

	// Appelé par le GameMode après le spawn de la caméra
	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetBattleCamera(AWOTOLBattleCamera* Camera);

	// Exposé pour que le HUD affiche l'état de sélection
	UFUNCTION(BlueprintPure, Category = "Battle")
	UUnitSelectionManager* GetSelectionManager() const;

	// Seuil en pixels pour décider si un clic devient un drag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float BoxSelectDragThreshold = 10.f;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	EFactionID PlayerFaction = EFactionID::None;

	UPROPERTY(BlueprintReadOnly, Category = "Battle")
	TWeakObjectPtr<AWOTOLBattleCamera> BattleCamera;

private:
	// Inputs
	void OnLeftMousePressed();
	void OnLeftMouseReleased();
	void OnRightMousePressed();
	void OnSelectAll();

	// Raycast helpers
	AUnitBase* GetUnitUnderCursor() const;
	bool       GetGroundLocationUnderCursor(FVector& OutLocation) const;

	// Box select state
	bool       bIsBoxSelecting   = false;
	FVector2D  BoxSelectStart;
	FVector2D  BoxSelectCurrent;

	// Commandes aux unités sélectionnées
	void IssueCommandToSelection(AUnitBase* TargetUnit, FVector TargetLocation);
};
