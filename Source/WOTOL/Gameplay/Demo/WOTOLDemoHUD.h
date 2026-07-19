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
	static FBox2D FactionLaunchButtonRect(float W, float H); // validation explicite après tous les réglages
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

	// Bouton « Continuer » de la fenêtre d'objectif MODALE (validation manuelle v0.8).
	static FBox2D ObjectiveContinueButtonRect(float W, float H);

	// Inventaire de bâtiments pendant la nage libre post-Kraken.
	static FBox2D ExplorationCrystalliserButtonRect(float W, float H);

	// ─── Vue CITÉ (phases 2 & 9 — production) ───────────────────────────────────
	// Cartes de production (bâtiments) le long du bas de l'écran.
	static FBox2D CityCardRect(int32 Index, float W, float H);
	// Bandeau HAUT de la carte = bouton « Améliorer le bâtiment » (niveau -> niveau des unités).
	static FBox2D CityCardUpgradeRect(int32 Index, float W, float H);
	// Bouton « Partir en expédition » (bas-droite).
	static FBox2D CityDepartButtonRect(float W, float H);
	// Bouton « Compétences » de la cité (ouvre l'onglet des axes).
	static FBox2D CitySkillsButtonRect(float W, float H);
	// Trois emplacements fixes et libres de la cité greybox. Le bâtiment à distance doit être
	// sélectionné dans sa carte puis posé explicitement sur l'un de ces emplacements.
	static FBox2D CityBuildPlotRect(int32 Index, float W, float H);
	// Croissance du mythique après sécurisation de la zone.
	static FBox2D CityFeedMythicButtonRect(float W, float H);

	// ─── Gestion du territoire après la défense ──────────────────────────────
	static FBox2D TerritoryRepairButtonRect(float W, float H);
	static FBox2D TerritoryDefenseButtonRect(float W, float H);
	static FBox2D TerritoryGarrisonMinusRect(int32 Index, float W, float H);
	static FBox2D TerritoryGarrisonPlusRect(int32 Index, float W, float H);
	static FBox2D TerritoryReturnCityButtonRect(float W, float H);

	// ─── Onglet COMPÉTENCES (axes par unité) ───────────────────────────────────
	// Bouton d'axe (ligne = catégorie d'unité, col 0=Base,1=Axe1,2=Axe2).
	static FBox2D SkillsAxisRect(int32 CatIndex, int32 AxisIndex, float W, float H);
	static FBox2D SkillsBackButtonRect(float W, float H);
	// Catégories productibles en cité, dans l'ordre des cartes (hors Chef).
	static int32 CityCardCount();
	static EDemoUnitCategory CityCardCategory(int32 Index);

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
	// Fenêtre d'objectif modale (titre + corps + bouton Continuer), dessinée par-dessus tout.
	void DrawObjectiveWindow(float W, float H, class UDemoFlowSubsystem* Demo);

	// Écrans du flux (menu / faction / préparation)
	void DrawMainMenu(float W, float H);
	void DrawFactionSelect(float W, float H);
	void DrawExplorationHUD(float W, float H, class UDemoFlowSubsystem* Demo);
	// Vue cité : fond + cristaux + cartes de production + bouton d'expédition.
	void DrawCityView(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawTerritoryView(float W, float H, class UDemoFlowSubsystem* Demo);
	// Onglet compétences : choix de l'axe (voie) de chaque type d'unité.
	void DrawSkillsView(float W, float H, class UDemoFlowSubsystem* Demo);
	// Écran de chargement (fond animé + logo + anneau + astuce).
	void DrawLoadingScreen(float W, float H, class UDemoFlowSubsystem* Demo);
	// Fond de cité chargé depuis le disque selon la faction (mis en cache).
	class UTexture2D* GetCityBackground(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> CityBgTexture = nullptr;
	EFactionID CityBgFaction = EFactionID::None;
	class UTexture2D* GetTransitionBackground();
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> TransitionBgTexture = nullptr;
	bool bTransitionBgTried = false;
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
	// État de la compétence (R) de l'unité primaire sélectionnée : nom + prête/recharge.
	void DrawAbilityStatus(float W, float H, class UWorld* World);
	// MARQUEURS DE GROUPE sur le champ de bataille (façon Total War) : un seul repère par
	// groupe (icône du type + effectif + petite barre de vie), projeté à l'écran -> remplace
	// les noms 3D par unité (perf + lisibilité).
	void DrawBattlefieldMarkers(float W, float H, class UWorld* World);
	// Jauge verticale SURFACE / MID / SOL (Homeworld) : indique la couche de la sélection.
	void DrawVerticalLayerGauge(float W, float H, class UWorld* World);
	void DrawRoleIcon(float CX, float CY, float R, EUnitRole IconRole, const FLinearColor& Fac);
	// Petite barre encadrée générique (fond + remplissage + cadre).
	void DrawBar(float X, float Y, float BarW, float BarH, float Pct,
		const FLinearColor& Fill, const FLinearColor& Back);
};
