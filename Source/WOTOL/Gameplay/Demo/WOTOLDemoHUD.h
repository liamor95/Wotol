#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Data/WOTOLTypes.h"
#include "Gameplay/Demo/DemoFlowSubsystem.h"
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
	// Vitesse de jeu : 3 chips (0 = x1, 1 = x1.5, 2 = x2), sous le bouton plein écran.
	static FBox2D GameSpeedButtonRect(int32 Index, float W, float H);
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
	// Aquiloris uniquement : choix Aquis (0) / Aquira (1), stats identiques (Role Chef).
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
	// Onglets de la fenêtre de bâtiment (fiche technique) : 0=Résumé, 1=Recrutement,
	// 2=Statistiques, 3=Compétences, 4=Rôle (demande Liamor 29/07/2026).
	static FBox2D BuildingTabRect(int32 TabIndex, float W, float H);

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
	// Bouton "AMÉLIORER LE GRADE" de la ligne (catégorie d'unité).
	static FBox2D SkillsGradeButtonRect(int32 CatIndex, float W, float H);
	// Bouton "PALIER" (investissement complémentaire dans l'axe déjà choisi, une fois pris —
	// portée qui grandit progressivement pour les unités à distance), affiché seulement pour
	// les catégories concernées (cf. UDemoFlowSubsystem::DoesCategoryAxisScaleByTier).
	static FBox2D SkillsTierButtonRect(int32 CatIndex, float W, float H);
	// Catégories productibles en cité, dans l'ordre des cartes (hors Chef).
	static int32 CityCardCount();
	static EDemoUnitCategory CityCardCategory(int32 Index);
	// Alias de CityCardCount/CityCardCategory pour l'écran Compétences (hors Chef — il se gère
	// via son propre bâtiment cliquable dans la cité, cf. fenêtre RECHERCHE).
	static int32 SkillsCategoryCount();
	static EDemoUnitCategory SkillsCategoryAt(int32 Index);

	// ─── Fenêtre RECHERCHE (demande Liamor 29/07/2026) ─────────────────────────
	static FBox2D ResearchBackButtonRect(float W, float H);
	// Bouton d'amélioration de bâtiment (côté gauche, une ligne par catégorie).
	static FBox2D ResearchCityUpgradeRect(int32 CatIndex, float W, float H);
	// Boutons du Chef (côté droit) : Grade +1, puis Axe 1 / Axe 2.
	static FBox2D ResearchChefGradeRect(float W, float H);
	static FBox2D ResearchChefAxisRect(int32 AxisIndex, float W, float H);
	// Palier complémentaire du Chef (Noxar : portée laser), sous les deux noeuds d'axe.
	static FBox2D ResearchChefTierRect(float W, float H);
	// Bouton "RECHERCHE" ouvert depuis la fenêtre de bâtiment (visible surtout sur le Chef).
	static FBox2D BuildingResearchButtonRect(float W, float H);
	// Sélecteur de formation tactique (voir DrawFormationSelector).
	static FBox2D FormationButtonRect(int32 Index, float W, float H);

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
	// Comme DrawCenteredText, mais centré HORIZONTALEMENT DANS Box (pas sur tout l'écran) —
	// pour du texte à l'intérieur d'un panneau étroit (ex. fenêtre de bâtiment, décalée sur le
	// côté). Corrige un bug pré-existant : DrawCenteredText centrait sur Canvas->SizeX entier,
	// donc le texte du panneau de bâtiment (large ~360px, collé à droite) s'affichait en réalité
	// au milieu de l'écran, hors du panneau (découvert le 29/07/2026 en ajoutant les icônes).
	void DrawCenteredTextInBox(const FBox2D& Box, const FString& Text, float Y, const FLinearColor& Color, float Scale);
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
	// Fenêtre RECHERCHE scindée en deux (demande Liamor 29/07/2026) : bâtiments de la cité à
	// gauche, arbre Grade/Axe du Chef à droite (un seul écran au lieu de deux séparés).
	void DrawResearchView(float W, float H, class UDemoFlowSubsystem* Demo);
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
	// Fond de panneau ORNÉ (planche PanelFrame/PanelFrameWide/PanelFramePortrait fournie par
	// Liamor selon la forme du rectangle) + voile sombre semi-transparent par-dessus pour garder
	// le texte lisible. Remplace le simple DrawRect plat utilisé jusqu'ici sur la plupart des
	// écrans (retour de Liamor du 31/07/2026 : "les fenêtres cliquables ça fait des rectangles
	// mais... pas de fond"). Repli sur un DrawRect plus opaque si la planche est absente.
	void DrawFramedPanel(const FBox2D& R, EFactionID Faction, float OverlayOpacity = 0.55f);
	// Pictogramme vectoriel (traits Canvas, pas de texture) pour la thématique d'un axe de
	// compétence — épée (Offensif), bouclier (Défensif), chevron (Support), croix (Soins),
	// réticule (Contrôle). Remplace le glyphe texte ASCII provisoire (demande Liamor 29/07/2026).
	void DrawAxisGlyph(const FVector2D& Center, float Size, const FString& Category, const FLinearColor& Color);

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

	// Emblèmes officiels par faction, versions DÉTOURÉES (Content/UI/EmblemAquilorisIcon.png /
	// EmblemNoxeensIcon.png — dégradé alpha généré depuis les planches complètes fournies par
	// Liamor, cf. commentaire dans GetFactionEmblem), utilisés sur l'écran de choix de faction.
	// Même mécanisme de chargement.
	class UTexture2D* GetFactionEmblem(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> AquilorisEmblemTexture = nullptr;
	bool bAquilorisEmblemTried = false;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> NoxeensEmblemTexture = nullptr;
	bool bNoxeensEmblemTried = false;

	// Cadres ornés par faction (Content/UI/PanelFrameAquiloris.png / PanelFrameNoxeens.png,
	// 26/07/2026) : habillage des fenêtres MODALES (objectif, confirmation, réglages) qui
	// n'avaient jusqu'ici qu'un panneau de couleur unie. Même mécanisme de chargement.
	class UTexture2D* GetPanelFrame(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> AquilorisFrameTexture = nullptr;
	bool bAquilorisFrameTried = false;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> NoxeensFrameTexture = nullptr;
	bool bNoxeensFrameTried = false;

	// Variantes PORTRAIT (menu Réglages, contenu haut/étroit) et LARGE (écran Commandes,
	// texte de description qui déborderait d'un cadre standard), fournies par Liamor le
	// 26/07/2026 pour compléter les 2 écrans que le cadre standard (format paysage ~1.5:1)
	// aurait déformés.
	class UTexture2D* GetPanelFramePortrait(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> AquilorisFramePortraitTexture = nullptr;
	bool bAquilorisFramePortraitTried = false;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> NoxeensFramePortraitTexture = nullptr;
	bool bNoxeensFramePortraitTried = false;

	class UTexture2D* GetPanelFrameWide(EFactionID Faction);
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> AquilorisFrameWideTexture = nullptr;
	bool bAquilorisFrameWideTried = false;
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> NoxeensFrameWideTexture = nullptr;
	bool bNoxeensFrameWideTried = false;

	// Fond neutre de l'écran de choix de faction (Content/UI/BackgroundFactionSelect.png,
	// 26/07/2026) : seul écran qui ne pouvait pas utiliser DrawFactionAmbientTint (pas encore
	// de faction choisie à ce moment) — restait donc 100% procédural jusqu'ici.
	class UTexture2D* GetFactionSelectBackground();
	UPROPERTY(Transient) TObjectPtr<class UTexture2D> FactionSelectBgTexture = nullptr;
	bool bFactionSelectBgTried = false;

	// Éléments style Total War
	void DrawTopBar(float W, float H, class UWorld* World, class UDemoFlowSubsystem* Demo);
	void DrawBossBar(float W, float H, class AWOTOLDemoUnit* Boss);
	void DrawCommandBar(float W, float H, class UWorld* World);
	// État de la compétence (R) de l'unité primaire sélectionnée : nom + prête/recharge.
	void DrawAbilityStatus(float W, float H, class UWorld* World);
	// Sélecteur de FORMATION tactique (26/07/2026, UFormationComponent branché) : n'apparaît
	// que si >=2 unités sont sélectionnées (une formation n'a de sens qu'en groupe). 6 puces
	// (Aucune/Ligne/Coin/Carré/Lâche/Colonne), surbrillance sur le type actif du contrôleur.
	void DrawFormationSelector(float W, float H, class UWorld* World);
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
