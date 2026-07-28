#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Data/WOTOLTypes.h"
#include "Gameplay/Units/FormationComponent.h" // EFormationType
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

	// Exposé pour que le HUD dessine la position de la caméra sur la minimap.
	UFUNCTION(BlueprintPure, Category = "Battle")
	AWOTOLBattleCamera* GetBattleCamera() const { return BattleCamera.Get(); }

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

	// ── PAUSE (gel de l'action) & RÉGLAGES (menu de l'engrenage) — indépendants ──
	// bFrozen : le bouton pause GÈLE la bataille (unités, décor, animaux figés) MAIS la caméra
	//   reste libre de se déplacer. L'icône bascule pause <-> play. La reprise garde la position
	//   caméra où le joueur l'a laissée.
	// bSettingsOpen : l'engrenage ouvre le MENU (Reprendre / Recommencer / Quitter) — ce qui
	//   apparaissait avant sur la pause. Il met aussi le jeu en pause tant qu'il est ouvert.
	bool bFrozen = false;
	bool bSettingsOpen = false;
	// Sous-écran "COMMANDES" (liste des touches), accessible depuis le menu réglages -> ne
	// se ferme jamais tout seul en même temps que bSettingsOpen : ToggleSettings() le remet à
	// faux à la fermeture pour que la réouverture retombe toujours sur le menu principal.
	bool bControlsOpen = false;
	// Confirmation avant une action DESTRUCTIVE (perte de progression) : Recommencer/Quitter
	// n'exécutent plus directement au clic, ils ouvrent cette boîte de dialogue. Un seul des
	// deux actif à la fois (0=aucun, 1=recommencer, 2=quitter) -> pas besoin de 2 booléens.
	uint8 PendingConfirmAction = 0;
	// Suivi de l'état de la fenêtre d'objectif modale (gèle l'action tant qu'ouverte).
	bool bObjectivePausedLast = false;
	void ApplyPauseState(); // pause moteur = (bFrozen || bSettingsOpen)
	void TogglePause();     // bascule bFrozen
	void ToggleSettings();  // ouvre/ferme le menu réglages
public:
	bool IsBattleFrozen() const { return bFrozen; }
	bool IsSettingsOpen() const { return bSettingsOpen; }
	bool IsControlsOpen() const { return bControlsOpen; }
	// 0 = aucune confirmation en attente, 1 = confirmer "Recommencer", 2 = confirmer "Quitter".
	uint8 GetPendingConfirmAction() const { return PendingConfirmAction; }

	// ── Écran RÉGLAGES : volume musique (réel, s'applique à la piste en cours) + plein
	// écran (UGameUserSettings, aucun asset requis). Absents jusqu'ici — l'écran ne
	// contenait que Reprendre/Recommencer/Quitter, pas de vrais réglages audio/affichage.
	UFUNCTION(BlueprintPure, Category = "Battle")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetMusicVolume(float NewVolume);

	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsFullscreen() const;

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void ToggleFullscreen();

	// ── Vitesse de jeu (x1 / x1.5 / x2), demande de Liamor du 26/07/2026 ──
	// Dilate le temps moteur (unites, animations, production) ; l'UI (Canvas, Slate) reste en
	// temps reel puisqu'elle ne depend pas de TimeDilation. Persiste tant que la partie tourne.
	UFUNCTION(BlueprintPure, Category = "Battle")
	float GetGameSpeed() const { return CurrentGameSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetGameSpeed(float NewSpeed);

	// ── Formation tactique (26/07/2026, UFormationComponent branché) ──
	// Type appliqué au PROCHAIN ordre de déplacement de groupe (>=2 unités sélectionnées).
	// EFormationType::None = comportement EXISTANT inchangé (grille compacte orientée vers le
	// point cliqué, déjà validée) ; les autres types utilisent la géométrie de
	// UFormationComponent à la place, SANS toucher au reste du pipeline d'ordre (calage de
	// couche Z, clamp de zone de préparation, distinction déplacement/attack-move, tout reste
	// identique — seul le calcul des emplacements change).
	UFUNCTION(BlueprintPure, Category = "Battle")
	EFormationType GetFormationType() const { return CurrentFormationType; }

	UFUNCTION(BlueprintCallable, Category = "Battle")
	void SetFormationType(EFormationType NewType);

private:
	float CurrentGameSpeed = 1.f;
	EFormationType CurrentFormationType = EFormationType::None;

	UPROPERTY()
	TObjectPtr<UFormationComponent> FormationHelper = nullptr; // géométrie pure, jamais peuplé

private:
	// Traite un clic gauche sur l'UI (bouton pause / menu / écrans). Vrai = consommé.
	bool HandleUIClick();
	bool GetViewportSizeSafe(FVector2D& Out) const;

	// Clic sur une carte de la barre de commandement (bas-gauche).
	// Simple clic = consommé (ne désélectionne pas). Double clic = sélectionne
	// ce groupe d'unités + zoom caméra dessus (comme un double-clic sur l'unité).
	// Renvoie vrai si le clic a été traité (à consommer).
	bool HandleCommandBarClick(bool bDoubleClick);

	// Flux d'écrans
	class AWOTOLDemoDirector* GetDemoDirector() const;
	void SelectFaction(EFactionID Faction);

	// Verticalité : monte/descend la couche des unités sélectionnées
	void ChangeLayerForSelection(float DeltaZ);

	// Raycast helpers
	AUnitBase* GetUnitUnderCursor() const;
	bool       GetGroundLocationUnderCursor(FVector& OutLocation) const;
	bool       GetWorldLocationOnHorizontalPlane(const FVector2D& ScreenPosition,
		float PlaneZ, FVector& OutLocation) const;

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
	// bAttackMoveToGround : ordre vers un POINT au sol où les unités ENGAGENT tout ennemi
	// rencontré en chemin (attack-move classique, Shift+clic droit). Faux = déplacement direct.
	void IssueCommandToSelection(AUnitBase* TargetUnit, FVector TargetLocation, bool bAttackMoveToGround = false);

	// Touche R : active la compétence (index 0) de chaque unité sélectionnée sur l'ennemi
	// vivant le plus proche à portée. Ignore silencieusement si en recharge ou hors de portée
	// (pas de gaspillage d'activation sans effet).
	void ActivateSelectionAbility();

public:
	// Lu par le HUD pour afficher l'état de la compétence de l'unité PRIMAIRE sélectionnée
	// (première du groupe). Renvoie faux si rien n'est sélectionné ou si l'unité n'a pas
	// de compétence (repli greybox non assigné, cf. AUnitBase::InitFromDataAsset).
	UFUNCTION(BlueprintPure, Category = "Battle")
	bool GetPrimarySelectionAbilityStatus(FText& OutName, float& OutCooldownRemaining, float& OutCooldownMax) const;
};
