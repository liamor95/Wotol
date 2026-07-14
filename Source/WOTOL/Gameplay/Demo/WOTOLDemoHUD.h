#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLDemoHUD.generated.h"

// HUD de la démo dessiné 100% en C++ (Canvas) — AUCUN widget UMG requis.
// Affiche : la boîte de sélection (rectangle de drag), le message d'objectif
// au centre-haut, et l'écran de fin de démo. Branché auto par WOTOLGameMode_Demo.
UCLASS()
class WOTOL_API AWOTOLDemoHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// ─── Zones cliquables (source de vérité partagée HUD ↔ PlayerController) ───
	// Bouton pause (deux barres) en haut à droite.
	static FBox2D PauseButtonRect(float W, float H);
	// Bouton RÉGLAGES (engrenage), à gauche du bouton pause.
	static FBox2D SettingsButtonRect(float W, float H);
	// Boutons du menu pause (0 = Reprendre, 1 = Recommencer, 2 = Quitter).
	static FBox2D MenuButtonRect(int32 Index, float W, float H);

	// Flux d'écrans
	static FBox2D StartGameButtonRect(float W, float H);      // menu principal
	static FBox2D FactionButtonRect(int32 Index, float W, float H); // 0=Aquiloris 1=Noxeens
	static FBox2D LaunchBattleButtonRect(float W, float H);   // préparation

	// Boutons de couche verticale (nage) — montent/descendent la sélection
	static FBox2D LayerUpButtonRect(float W, float H);
	static FBox2D LayerDownButtonRect(float W, float H);

	// Écran de RÉSUMÉ de bataille
	static FBox2D SummaryContinueButtonRect(float W, float H);      // phase 1 -> phase 2
	static FBox2D SummaryReplayButtonRect(float W, float H);        // final : rejouer
	static FBox2D SummaryChangeFactionButtonRect(float W, float H); // final : changer de faction
	static FBox2D SummaryMenuButtonRect(float W, float H);          // final : menu principal
	static FBox2D DifficultyButtonRect(int32 Index, float W, float H); // choix faction : 3 niveaux
	static FBox2D SummaryQuitButtonRect(float W, float H);          // final : quitter
	static FBox2D InterludeContinueButtonRect(float W, float H);    // transition -> phase 2

	// Carte de la barre de commandement (bas-gauche) pour l'index de groupe donné.
	// Sert au double-clic : sélectionner + zoomer sur ce groupe d'unités.
	static FBox2D CommandCardRect(int32 Index, float W, float H);
	static int32  CommandCardMaxFit(float W);

	// Construit la liste des GROUPES du roster : TOUTES les unités VIVANTES de la
	// faction joueur, agrégées par nom (dans l'ordre du registre). Partagé entre le
	// dessin du roster (HUD) et le clic (PlayerController) pour que les index de
	// cartes correspondent EXACTEMENT — le roster reste affiché en entier même quand
	// un seul groupe est actif (sélectionné).
	static void BuildRosterGroups(class UWorld* World, EFactionID Faction,
		TArray<FString>& OutOrder, TMap<FString, TArray<class AUnitBase*>>& OutByName);

private:
	void DrawCenteredText(const FString& Text, float Y, const FLinearColor& Color, float Scale);
	void DrawPauseButton(float W, float H);
	void DrawSettingsButton(float W, float H);
	void DrawPauseOverlay(float W, float H);

	// Écrans du flux (menu / faction / préparation)
	void DrawMainMenu(float W, float H);
	void DrawFactionSelect(float W, float H);
	void DrawPrepareBar(float W, float H);
	void DrawSummary(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawInterlude(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawBuildingBar(float W, float H, class AWOTOLCaptureObject* Building);
	// Boussole de courant océanique (sens relatif caméra + intensité).
	void DrawCurrentIndicator(float W, float H, class UWorld* World);
	void DrawButton(const FBox2D& R, const FString& Label, const FLinearColor& Tint, float TextScale = 1.3f);

	// Fond marin ANIMÉ (dégradé de profondeur + bulles qui montent + rais de lumière),
	// partagé par tous les écrans plein-écran pour un rendu vivant (pas une couleur plate).
	void DrawUnderwaterBackground(float W, float H);
	// Grand titre lumineux avec halo + pulsation (menu principal).
	void DrawGlowTitle(const FString& Text, float Y, float Scale, const FLinearColor& Color);
	void DrawLavaTitle(const FString& Text, float Y, float Scale);

	// Image d'accueil : chargée soit comme asset importé (/Game/UI/MainMenuBG), soit
	// directement depuis le PNG du disque (Content/UI/MainMenuBG.png) -> pas d'import
	// manuel nécessaire. Mise en cache (chargée une seule fois).
	class UTexture2D* GetMenuBackground();
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> MenuBgTexture = nullptr;
	bool bMenuBgTried = false;

	// Éléments style Total War
	void DrawTopBar(float W, float H, class UWorld* World, class UDemoFlowSubsystem* Demo);
	void DrawBossBar(float W, float H, class AWOTOLDemoUnit* Boss);
	void DrawCommandBar(float W, float H, class UWorld* World);
	// MARQUEURS DE GROUPE sur le champ de bataille (façon Total War) : un seul repère par
	// groupe (icône du type + effectif + petite barre de vie), projeté à l'écran -> remplace
	// les noms 3D par unité (perf + lisibilité).
	void DrawBattlefieldMarkers(float W, float H, class UWorld* World);
	void DrawRoleIcon(float CX, float CY, float R, EUnitRole Role, const FLinearColor& Fac);
	// Petite barre encadrée générique (fond + remplissage + cadre).
	void DrawBar(float X, float Y, float BarW, float BarH, float Pct,
		const FLinearColor& Fill, const FLinearColor& Back);
};
