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
	// Écran RÉGLAGES : barre de volume musique (clic = fixe le niveau à la position) et
	// bouton plein écran / fenêtré, entre le titre et les boutons Reprendre/Recommencer/Quitter.
	static FBox2D MusicVolumeBarRect(float W, float H);
	static FBox2D FullscreenToggleButtonRect(float W, float H);
	// Sous-écran "COMMANDES" (liste des touches, ouvert depuis le menu réglages) : bouton
	// RETOUR, ancré en bas (la liste est plus haute que le menu réglages standard).
	static FBox2D ControlsBackButtonRect(float W, float H);
	// Confirmation avant Recommencer/Quitter (actions destructives) : Oui / Annuler.
	static FBox2D ConfirmYesButtonRect(float W, float H);
	static FBox2D ConfirmNoButtonRect(float W, float H);

	// Flux d'écrans
	static FBox2D StartGameButtonRect(float W, float H);      // menu principal
	static FBox2D FactionButtonRect(int32 Index, float W, float H); // 0=Aquiloris 1=Noxeens
	static FBox2D FactionLaunchButtonRect(float W, float H); // validation explicite après tous les réglages
	static FBox2D LaunchBattleButtonRect(float W, float H);   // préparation

	// ─── Personnalisation du héros (entre le choix de faction et le lancement) ──
	// Aquiloris uniquement : choix Akis (0) / Aquira (1), stats identiques (Role Chef).
	static FBox2D HeroAquilorisVariantButtonRect(int32 Index, float W, float H);
	static FBox2D HeroHeritageButtonRect(int32 Index, float W, float H);  // 4 héritages
	static FBox2D HeroSpecialtyButtonRect(int32 Index, float W, float H); // 4 spécialités
	static FBox2D HeroPortraitPrevRect(float W, float H);
	static FBox2D HeroPortraitNextRect(float W, float H);
	static FBox2D HeroCustomizationConfirmRect(float W, float H);
	static FBox2D HeroCustomizationBackRect(float W, float H);
	// Récapitulatif avant lancement (UI_ResumePartie.png) : Retour (vers personnalisation) /
	// Lancer la partie (lance réellement la démo — StartDemoAfterSelection).
	static FBox2D PreGameSummaryBackRect(float W, float H);
	static FBox2D PreGameSummaryLaunchRect(float W, float H);

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

	// Bornes du monde (centre + étendue) de la minimap — source de vérité partagée entre le
	// dessin (DrawMinimap) et le clic (recentrage caméra). Renvoie faux si rien de vivant.
	static bool GetMinimapWorldFrame(class UWorld* World, FVector2D& OutCenter, float& OutSpan);
	// Convertit un clic sur le panneau minimap en position monde (plan horizontal, Z=0).
	// Renvoie faux si le clic est hors du panneau ou si la trame n'a pas pu être calculée.
	static bool MinimapScreenToWorld(const FVector2D& ScreenPos, float W, float H,
		class UWorld* World, FVector& OutWorldLoc);

private:
	void DrawCenteredText(const FString& Text, float Y, const FLinearColor& Color, float Scale);
	void DrawPauseButton(float W, float H);
	void DrawSettingsButton(float W, float H);
	void DrawPauseOverlay(float W, float H);
	// Sous-écran "COMMANDES" (liste des touches par contexte), ouvert depuis le menu réglages.
	void DrawControlsScreen(float W, float H);
	// Boîte de confirmation avant une action destructive (Recommencer/Quitter).
	void DrawConfirmDialog(float W, float H, const FString& Message, const FString& ConfirmLabel);
	// Fenêtre d'objectif modale (titre + corps + bouton Continuer), dessinée par-dessus tout.
	void DrawObjectiveWindow(float W, float H, class UDemoFlowSubsystem* Demo);

	// Écrans du flux (menu / faction / préparation)
	void DrawMainMenu(float W, float H);
	void DrawFactionSelect(float W, float H);
	// Écran de personnalisation du héros (héritage / spécialité / portrait), entre le choix
	// de faction et le lancement effectif de la démo (StartDemoAfterSelection).
	void DrawHeroCustomization(float W, float H, class UDemoFlowSubsystem* Demo);
	// Récapitulatif final avant lancement (nom/faction/difficulté/héritage/spécialité),
	// conforme à Content/UI/Reference/Maquettes/UI_ResumePartie.png.
	void DrawPreGameSummary(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawExplorationHUD(float W, float H, class UDemoFlowSubsystem* Demo);
	// Vue cité : chrome par-dessus la scène 3D isométrique (AWOTOLCityEnvironment/
	// AWOTOLCityCamera) — cristaux, cartes de production, fiche technique, bouton d'expédition.
	void DrawCityView(float W, float H, class UDemoFlowSubsystem* Demo);
	void DrawTerritoryView(float W, float H, class UDemoFlowSubsystem* Demo);
	// Onglet compétences : choix de l'axe (voie) de chaque type d'unité.
	void DrawSkillsView(float W, float H, class UDemoFlowSubsystem* Demo);
	// Écran de chargement (fond animé + logo + anneau + astuce).
	void DrawLoadingScreen(float W, float H, class UDemoFlowSubsystem* Demo);
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
	// Lavis translucide dans la teinte de la faction choisie (thème d'interface par faction,
	// décision Liamor du 22/07/2026) — à appeler après DrawUnderwaterBackground sur les
	// écrans qui suivent le choix de faction. Sans effet si EFactionID::None.
	void DrawFactionAmbientTint(float W, float H, EFactionID Faction);
	// Grand titre lumineux avec halo + pulsation (menu principal).
	void DrawGlowTitle(const FString& Text, float Y, float Scale, const FLinearColor& Color);
	void DrawLavaTitle(const FString& Text, float Y, float Scale);

	// Image d'accueil : chargée soit comme asset importé (/Game/UI/MainMenuBG), soit
	// directement depuis le PNG du disque (Content/UI/MainMenuBG.png) -> pas d'import
	// manuel nécessaire. Mise en cache (chargée une seule fois).
	class UTexture2D* GetMenuBackground();
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> MenuBgTexture = nullptr;
	bool bMenuBgTried = false;

	// Fonds de biome réels par faction (Content/UI/BackgroundAquiloris.png /
	// BackgroundNoxeens.png), même mécanisme que GetMenuBackground (PNG chargé directement
	// depuis le disque, pas d'import manuel requis). Utilisés par DrawFactionAmbientTint à la
	// place des silhouettes procédurales quand le fichier existe. Hors scope démo pour les
	// 3 autres factions (retourne toujours nullptr pour elles).
	class UTexture2D* GetFactionBackground(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> AquilorisBgTexture = nullptr;
	bool bAquilorisBgTried = false;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> NoxeensBgTexture = nullptr;
	bool bNoxeensBgTried = false;

	// Emblèmes officiels par faction (Content/UI/EmblemAquiloris.png / EmblemNoxeens.png),
	// utilisés sur l'écran de choix de faction. Même mécanisme de chargement.
	class UTexture2D* GetFactionEmblem(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> AquilorisEmblemTexture = nullptr;
	bool bAquilorisEmblemTried = false;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> NoxeensEmblemTexture = nullptr;
	bool bNoxeensEmblemTried = false;

	// Éléments style Total War
	void DrawTopBar(float W, float H, class UWorld* World, class UDemoFlowSubsystem* Demo);
	void DrawBossBar(float W, float H, class AWOTOLDemoUnit* Boss);
	void DrawCommandBar(float W, float H, class UWorld* World);
	// État de la compétence (R) de l'unité primaire sélectionnée : nom + prête/recharge.
	void DrawAbilityStatus(float W, float H, class UWorld* World);
	// Minimap schématique (coin haut-droit, sous les boutons pause/réglages) : positions
	// de toutes les unités vivantes (couleur = faction) + bâtiment de capture + caméra.
	void DrawMinimap(float W, float H, class UWorld* World);
	static FBox2D MinimapRect(float W, float H);
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
