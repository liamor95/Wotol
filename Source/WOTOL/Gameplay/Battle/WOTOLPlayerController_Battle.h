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

	// État de la boîte de sélection (lu par le HUD pour dessiner le rectangle)
	bool      IsBoxSelecting() const { return bIsBoxSelecting; }
	FVector2D GetBoxStart()    const { return BoxSelectStart; }
	FVector2D GetBoxCurrent()  const { return BoxSelectCurrent; }

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
	void OnRightMouseReleased();
	void OnSelectAll();

	// Pause (clavier + clics sur le menu)
	void TogglePause();
	// Traite un clic gauche sur l'UI (bouton pause / menu / écrans). Vrai = consommé.
	bool HandleUIClick();
	bool GetViewportSizeSafe(FVector2D& Out) const;

	// Flux d'écrans
	class AWOTOLDemoDirector* GetDemoDirector() const;
	void PickFactionAndPrepare(EFactionID Faction);

	// Verticalité : monte/descend la couche des unités sélectionnées
	void ChangeLayerForSelection(float DeltaZ);

	// Raycast helpers
	AUnitBase* GetUnitUnderCursor() const;
	bool       GetGroundLocationUnderCursor(FVector& OutLocation) const;

	// Box select state
	bool       bIsBoxSelecting   = false;
	FVector2D  BoxSelectStart;
	FVector2D  BoxSelectCurrent;

	// Clic droit : distingue tap (= ordre) de drag (= rotation caméra)
	bool       bRightDown        = false;
	FVector2D  RightPressPos;

	// Double-clic gauche (focus caméra sur une unité alliée)
	float      LastLeftClickTime = -10.f;
	FVector2D  LastLeftClickPos;

	// Commandes aux unités sélectionnées
	void IssueCommandToSelection(AUnitBase* TargetUnit, FVector TargetLocation);
};
