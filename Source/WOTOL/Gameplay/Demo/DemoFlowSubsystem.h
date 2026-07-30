#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "DemoFlowSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// MACHINE D'ÉTATS DE LA DÉMO WOTOL — colonne vertébrale de la boucle jouable.
// Temps réel (PAS de tour par tour). Persiste dans le GameInstance entre phases.
// Pilote : cité → exploration → bataille créature → capture → mythique →
//          retour cité/déblocages → défense rivale → réparation → fin.
// ─────────────────────────────────────────────────────────────────────────────

// Phases majeures de la démo (regroupent les 41 étapes du cahier des charges)
UENUM(BlueprintType)
enum class EDemoPhase : uint8
{
	None                  UMETA(DisplayName = "Aucune"),
	CityIntro             UMETA(DisplayName = "Cité — intro + 1ère mission"),
	Exploration_Creature  UMETA(DisplayName = "Exploration — vers la créature"),
	Battle_Creature       UMETA(DisplayName = "Bataille — créature"),
	Capture_Zone          UMETA(DisplayName = "Capture de zone (Grade 1)"),
	Mythic_Discovery      UMETA(DisplayName = "Découverte du mythique juvénile"),
	City_Unlock           UMETA(DisplayName = "Cité — déblocages"),
	Exploration_Rival     UMETA(DisplayName = "Alerte — zone attaquée"),
	Battle_Rival          UMETA(DisplayName = "Bataille — défense rivale"),
	Repair_Zone           UMETA(DisplayName = "Réparation de la zone"),
	Territory_Management  UMETA(DisplayName = "Gestion du territoire"),
	Battle_Grand          UMETA(DisplayName = "Bataille — grande (phase 3, zone neutre)"),
	DemoEnd               UMETA(DisplayName = "Fin de démo")
};

// Niveau de difficulté (choisi sur l'écran de faction). Ajuste la puissance ennemie et la
// robustesse du joueur, en plus de la compensation d'identité de faction.
UENUM(BlueprintType)
enum class EDemoDifficulty : uint8
{
	Facile   UMETA(DisplayName = "Facile"),
	Normal   UMETA(DisplayName = "Normal"),
	Difficile UMETA(DisplayName = "Difficile")
};

// Écran d'interface courant (menu → faction → préparation → jeu → résumé)
UENUM(BlueprintType)
enum class EDemoScreen : uint8
{
	MainMenu      UMETA(DisplayName = "Menu principal"),
	FactionSelect UMETA(DisplayName = "Choix de faction"),
	Prepare       UMETA(DisplayName = "Préparation (placement)"),
	Playing       UMETA(DisplayName = "En jeu"),
	Summary       UMETA(DisplayName = "Résumé de bataille"),
	Interlude     UMETA(DisplayName = "Transition narrative (hors-champ)"),
	City          UMETA(DisplayName = "Cité (production Aquiloris)"),
	WorldMap      UMETA(DisplayName = "Monde ouvert / carte"),
	Loading       UMETA(DisplayName = "Écran de chargement"),
	Skills        UMETA(DisplayName = "Compétences (arbre / axes)"),
	Exploration   UMETA(DisplayName = "Exploration — nage libre 3D"),
	Territory     UMETA(DisplayName = "Gestion du territoire"),
	HeroCustomization UMETA(DisplayName = "Personnalisation du héros"),
	// Récapitulatif final avant lancement (nom, faction, difficulté, héritage, spécialité) —
	// conforme à Content/UI/Reference/Maquettes/UI_ResumePartie.png. À NE PAS confondre avec
	// EDemoScreen::Summary (résumé de bataille, après une bataille — sens totalement différent).
	PreGameSummary UMETA(DisplayName = "Recapitulatif avant lancement"),
	// Fenêtre "RECHERCHE" scindée en deux (demande Liamor 29/07/2026) : à GAUCHE les
	// améliorations de bâtiments de la cité (les 5 catégories), à DROITE l'arbre Grade/Axe
	// du Chef (combat/PV) — un seul arbre de recherche au lieu de deux écrans séparés.
	Research UMETA(DisplayName = "Recherche (cite + chef)")
};

// Ligne de résumé : pertes d'un type d'unité (nom + perdus / total) pour une faction.
USTRUCT(BlueprintType)
struct FUnitLossEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString UnitName;
	UPROPERTY(BlueprintReadOnly) int32   Lost  = 0;
	UPROPERTY(BlueprintReadOnly) int32   Total = 0;
	UPROPERTY(BlueprintReadOnly) EFactionID Faction = EFactionID::None;
	UPROPERTY(BlueprintReadOnly) float   DamageDealt = 0.f; // dégâts infligés par le groupe
	UPROPERTY(BlueprintReadOnly) int32   DefPct   = 0;      // taux (représentatif) du type
	UPROPERTY(BlueprintReadOnly) int32   BlockPct = 0;
	UPROPERTY(BlueprintReadOnly) int32   DodgePct = 0;
};

// Type de bataille (la même arène sert pour les deux)
UENUM(BlueprintType)
enum class EBattleType : uint8
{
	CreatureEncounter UMETA(DisplayName = "Rencontre de créature"),
	RivalDefense      UMETA(DisplayName = "Défense contre la faction rivale")
};

// Catégorie d'unité (pilote forme greybox + règles de déblocage)
UENUM(BlueprintType)
enum class EDemoUnitCategory : uint8
{
	Chef       UMETA(DisplayName = "Chef / Héros"),
	Infanterie UMETA(DisplayName = "Infanterie"),
	Montee     UMETA(DisplayName = "Montée"),
	Distance   UMETA(DisplayName = "Distance"),
	Speciale   UMETA(DisplayName = "Spéciale"),
	Mythique   UMETA(DisplayName = "Mythique")
};

// Progression / déblocages de la démo
USTRUCT(BlueprintType)
struct FDemoProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bRangedUnlocked        = false; // Aquisphères / Noxeblast
	UPROPERTY(BlueprintReadOnly) bool bRangedBuildingConstructed = false;
	UPROPERTY(BlueprintReadOnly) bool bMythicDiscovered      = false; // juvénile découvert
	UPROPERTY(BlueprintReadOnly) bool bMythicBuildingUnlocked = false;
	UPROPERTY(BlueprintReadOnly) bool bRecruitBuildingActive = false;
	UPROPERTY(BlueprintReadOnly) bool bZoneCaptured          = false; // Grade 1 posé
	UPROPERTY(BlueprintReadOnly) bool bZoneDamaged           = false; // attaquée par la rivale
	UPROPERTY(BlueprintReadOnly) bool bZoneRepaired          = false;
	UPROPERTY(BlueprintReadOnly) bool bAllUnlocked           = false; // phase 3 : spéciale + mythique débloquées
	UPROPERTY(BlueprintReadOnly) bool bRivalAlertShown       = false;
	UPROPERTY(BlueprintReadOnly) bool bDefenseMissionReady   = false;
	UPROPERTY(BlueprintReadOnly) bool bZoneThreatened        = false;
	UPROPERTY(BlueprintReadOnly) bool bZoneLost              = false;
	UPROPERTY(BlueprintReadOnly) bool bDefenseSystemInstalled = false;
	UPROPERTY(BlueprintReadOnly) bool bBiomassGoalReached    = false;
	UPROPERTY(BlueprintReadOnly) bool bMythicPlayable        = false;
	UPROPERTY(BlueprintReadOnly) bool bMythicGiftPending     = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemoPhaseChanged,
	EDemoPhase, NewPhase, EDemoPhase, PreviousPhase);

// Diffusé quand le joueur valide une fenêtre d'objectif modale (paramètre = ID d'étape).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveConfirmed, FName, StepId);

// Diffusé à CHAQUE changement d'écran (SetScreen). Sert de point d'accroche unique pour la
// possession de caméra (ex. bascule vers la caméra isométrique de la cité) sans avoir à
// modifier tous les appels existants à SetScreen(EDemoScreen::City) dispersés dans le Director.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDemoScreenChanged, EDemoScreen, NewScreen);

UCLASS()
class WOTOL_API UDemoFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ─── Phase courante ────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetPhase(EDemoPhase NewPhase);

	// Avance à la phase suivante dans l'ordre linéaire de la démo
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void AdvancePhase();

	UFUNCTION(BlueprintPure, Category = "Demo")
	EDemoPhase GetPhase() const { return CurrentPhase; }

	// Type de bataille correspondant à la phase courante (pour l'arène)
	UFUNCTION(BlueprintPure, Category = "Demo")
	EBattleType GetCurrentBattleType() const;

	// ─── Faction & roster ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Demo")
	EFactionID GetPlayerFaction() const;

	// ID canonique d'une unité de la faction joueur pour une catégorie donnée
	UFUNCTION(BlueprintPure, Category = "Demo")
	FName GetUnitID(EFactionID Faction, EDemoUnitCategory Category) const;

	// Vrai si l'unité est jouable/déployable à ce stade de la démo
	UFUNCTION(BlueprintPure, Category = "Demo")
	bool IsCategoryUnlocked(EDemoUnitCategory Category) const;

	// ─── Déblocages (appelés par le Director aux bons moments) ──────────────────

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void UnlockRangedUnit();

	// Phase 3 : débloque TOUT le roster (spéciale + mythique compris).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void UnlockAll();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void DiscoverMythic();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ActivateRecruitBuilding();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void MarkZoneCaptured();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void MarkZoneDamaged();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void MarkZoneRepaired();

	UFUNCTION(BlueprintPure, Category = "Demo")
	const FDemoProgress& GetProgress() const { return Progress; }

	// ─── Statique : mapping catégorie → ID d'unité (noms du GDD/code) ───────────
	UFUNCTION(BlueprintPure, Category = "Demo")
	static EDemoUnitCategory GetCategoryForUnit(FName UnitID);

	UPROPERTY(BlueprintAssignable, Category = "Demo")
	FOnDemoPhaseChanged OnDemoPhaseChanged;

	// Message courant affiché par le HUD (objectif / narration)
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	FString CurrentMessage;

	// Issue de la démo (pour l'écran de fin : victoire ou défaite)
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	bool bDemoVictory = false;

	// Faction choisie par le joueur (source fiable, indépendante du GameInstance).
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	EFactionID SelectedFaction = EFactionID::None;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetSelectedFaction(EFactionID F) { SelectedFaction = F; RefreshHeroName(); }

	// ─── Personnalisation du héros (écran avant le lancement, après choix de faction) ──
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Hero")
	FHeroLoadout HeroLoadout;

	UFUNCTION(BlueprintPure, Category = "Demo|Hero")
	const FHeroLoadout& GetHeroLoadout() const { return HeroLoadout; }

	UFUNCTION(BlueprintCallable, Category = "Demo|Hero")
	void SetHeroHeritage(EHeroHeritage H) { HeroLoadout.Heritage = H; }

	UFUNCTION(BlueprintCallable, Category = "Demo|Hero")
	void SetHeroSpecialty(EHeroSpecialty S) { HeroLoadout.Specialty = S; }

	UFUNCTION(BlueprintCallable, Category = "Demo|Hero")
	void CycleHeroPortrait(int32 Delta) { HeroLoadout.PortraitIndex = (HeroLoadout.PortraitIndex + Delta + 5) % 5; }

	// Aquiloris uniquement : bascule le heros joue entre Aquis et Aquira (stats identiques,
	// Role Chef — voir FHeroLoadout::bPlayAsAquira). Sans effet pour les Noxeens.
	UFUNCTION(BlueprintCallable, Category = "Demo|Hero")
	void SetHeroPlayAsAquira(bool bAquira) { HeroLoadout.bPlayAsAquira = bAquira; RefreshHeroName(); }

private:
	// Nom du heros affiche (recap avant lancement) derive de la faction + du choix Aquis/
	// Aquira. Remplace le placeholder generique "Aquilian" qui ne correspondait a aucun nom
	// etabli du GDD.
	void RefreshHeroName()
	{
		if (SelectedFaction == EFactionID::Noxeens) HeroLoadout.HeroName = TEXT("Noxar");
		else if (SelectedFaction == EFactionID::Aquiloris)
			HeroLoadout.HeroName = HeroLoadout.bPlayAsAquira ? TEXT("Aquira") : TEXT("Aquis");
	}

public:

	// ─── Résumé de bataille (fin de phase) ─────────────────────────────────────
	// Pertes détaillées, remplies par le Director à la fin de chaque bataille.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	TArray<FUnitLossEntry> PlayerLosses;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	TArray<FUnitLossEntry> EnemyLosses;

	// Titre du résumé (ex: "KRAKEN VAINCU", "VICTOIRE", "DEFAITE").
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	FString SummaryTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	bool bSummaryVictory = true;

	// Durée totale de la bataille (secondes) — affichée dans le résumé.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	float SummaryDurationSeconds = 0.f;

	// Vrai = résumé FINAL de démo (boutons Rejouer / Changer de faction) ;
	// Faux = résumé intermédiaire phase 1 (bouton Continuer vers la phase 2).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	bool bSummaryIsFinal = false;

	// Une défaite de défense propose une issue utile : rejouer immédiatement ou revenir à
	// la cité. Le bâtiment encore debout reste contesté pendant la fenêtre de réaction.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	bool bSummaryCanReturnToCity = false;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	bool bSummaryBuildingDestroyed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	FString SummaryContinueLabel;

	// Réinitialise la progression (déblocages) pour rejouer la démo depuis le début.
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ResetProgress()
	{
		Progress = FDemoProgress();
		CurrentPhase = EDemoPhase::None;
		PlayerCrystals = 0;
		PlayerAbyssalMaterials = 0;
		PlayerBiomass = 0;
		PlayerOceanicEnergy = 0;
		LastRewardCrystals = 0;
		LastRewardAbyssalMaterials = 0;
		LastRewardBiomass = 0;
		LastRewardOceanicEnergy = 0;
		ReserveUnits.Empty();
		BuildingLevels.Empty();
		UnitAxes.Empty();
		TotalProducedUnits = 0;
		RangedUnitsProducedForObjective = 0;
		RangedBuildingPlotIndex = INDEX_NONE;
		bCityBuildingPlacementArmed = false;
		TerritoryBuildingCurrentHealth = TerritoryBuildingMaxHealth;
		TerritoryGrade = 1;
		InstalledDefenseCount = 0;
		DefenseTechnologyLevel = 1;
		GarrisonUnits = 0;
		InstalledDefenseSlots.Empty();
		GarrisonByUnit.Empty();
		ZoneDefenseWindowRemainingSeconds = 0.f;
		HeroLevel = 1;
		HeroXP = 0;
		CityLevel = 1;
		CityXP = 0;
		bSummaryCanReturnToCity = false;
		bSummaryBuildingDestroyed = false;
		SummaryContinueLabel.Empty();
		bCitySelectionValid = false;
		// Remis a zero : ReturnToCityForGrandBattleReveal() (phase 3) modifie ces trois champs
		// a l'execution (plafond de recrutement 60/100, base reduite) - sans ce reset, un
		// "Recommencer" apres avoir atteint la grande bataille repartirait avec le mauvais
		// plafond/effectif des la phase 1 (bug introduit le 26/07/2026, corrige ici).
		bReadyForGrandBattleDeparture = false;
		MaxArmyUnits = 35;
		InitialArmyUnits = 25;
	}

	// Difficulté choisie (défaut Normal = l'équilibrage de référence).
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	EDemoDifficulty Difficulty = EDemoDifficulty::Normal;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetDifficulty(EDemoDifficulty D) { Difficulty = D; }

	UFUNCTION(BlueprintPure, Category = "Demo")
	EDemoDifficulty GetDifficulty() const { return Difficulty; }

	// Écran d'interface courant (menu / faction / préparation / jeu)
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	EDemoScreen Screen = EDemoScreen::MainMenu;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetScreen(EDemoScreen S) { Screen = S; OnDemoScreenChanged.Broadcast(S); }

	UFUNCTION(BlueprintPure, Category = "Demo")
	EDemoScreen GetScreen() const { return Screen; }

	UPROPERTY(BlueprintAssignable, Category = "Demo")
	FOnDemoScreenChanged OnDemoScreenChanged;

	// ─── Sélection d'un bâtiment de la cité (clic 3D sur la maquette OU sur une carte) ──
	// Pilote la mise en évidence du bâtiment en 3D (AWOTOLCityBuildingProp) ET la fiche
	// technique affichée par le HUD (DrawCityView).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	EDemoUnitCategory SelectedCityCategory = EDemoUnitCategory::Infanterie;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	bool bCitySelectionValid = false;

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void SetSelectedCityCategory(EDemoUnitCategory Cat)
	{
		if (SelectedCityCategory != Cat) SelectedBuildingTab = 0; // nouvelle selection -> onglet Resume
		SelectedCityCategory = Cat;
		bCitySelectionValid = true;
	}

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool HasCitySelection() const { return bCitySelectionValid; }

	// Onglet actif de la fenêtre de bâtiment (0=Résumé, 1=Recrutement, 2=Statistiques,
	// 3=Compétences, 4=Rôle). Remis à 0 à chaque nouvelle sélection pour repartir du Résumé
	// (demande Liamor 29/07/2026 : fenêtre multi-onglets sur clic bâtiment).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 SelectedBuildingTab = 0;

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetSelectedBuildingTab() const { return SelectedBuildingTab; }

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void SetSelectedBuildingTab(int32 Tab) { SelectedBuildingTab = FMath::Clamp(Tab, 0, 4); }

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetMessage(const FString& Msg) { CurrentMessage = Msg; }

	// Objectif courant affiché en permanence dans le HUD
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	FString ObjectiveText;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetObjective(const FString& Text) { ObjectiveText = Text; }

	// Vrai entre la fin de l'interlude de croissance (phase 2 -> 3) et l'embarquement pour la
	// grande bataille : force un retour obligatoire a la cite pour que ses nouveaux batiments
	// (Speciale/Mythique, debloques par UnlockAll) soient VUS, pas seulement racontes par texte
	// (demande de Liamor du 26/07/2026). Remis a false des que la bataille demarre.
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	bool bReadyForGrandBattleDeparture = false;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetReadyForGrandBattleDeparture(bool bReady) { bReadyForGrandBattleDeparture = bReady; }

	// Boss courant (créature) — pour la barre de vie du HUD
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetBoss(AActor* InBoss) { BossActor = InBoss; }

	UFUNCTION(BlueprintPure, Category = "Demo")
	AActor* GetBoss() const { return BossActor.Get(); }

	// Objet de capture à défendre en phase 2 (pour la barre de vie du bâtiment au HUD)
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetCaptureObject(AActor* InObj) { CaptureObjectActor = InObj; }

	UFUNCTION(BlueprintPure, Category = "Demo")
	AActor* GetCaptureObject() const { return CaptureObjectActor.Get(); }

	// Texte narratif de l'écran de transition (déblocages hors-champ entre phase 1 et 2)
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	FString InterludeText;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetInterludeText(const FString& Text) { InterludeText = Text; }

	// ─── ÉCONOMIE DE LA CITÉ (phases 2 & 9 — production Aquiloris) ──────────────
	// Ressource de faction (Cristaux d'énergie Aquiloris / Biolumens Noxéens). Sert à
	// produire des unités dans la cité entre deux batailles et à réparer le Cristalliseur.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 PlayerCrystals = 0;

	// Ressources de mission distinctes. Les valeurs d'équilibrage restent réglables dans le
	// Director ; le subsystem ne fait que conserver le solde entre exploration, bataille et cité.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 PlayerAbyssalMaterials = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 PlayerBiomass = 0;

	// Énergie Océanique — 4e ressource COMMUNE (pas "Nourriture", qui n'existe pas dans les
	// visuels/GDD de Liamor : Content/UI/Reference/Ressources fournit exactement Cristaux
	// (Aquiloris), Minéraux Abyssaux, Biomasse Marine et Énergie Océanique comme les 4
	// ressources jouées en démo). SEULE ressource à capacité de STOCKAGE LIMITÉE — alimente
	// bâtiments actifs, recherches, améliorations avancées.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 PlayerOceanicEnergy = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|City", meta = (ClampMin = "1"))
	int32 MaxOceanicEnergy = 200;

	// Dernier lot gagné : affiché sur le résumé de bataille, sans inventer de conversion entre
	// les ressources. Remis à zéro au début d'une nouvelle récompense.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardCrystals = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardAbyssalMaterials = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardBiomass = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardOceanicEnergy = 0;

	// Unités produites en cité, en attente de déploiement à la bataille suivante
	// (clé = ID d'unité canonique, valeur = nombre en réserve).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	TMap<FName, int32> ReserveUnits;

	// La phase 2 commence avec chef + 16 fantassins + 8 montés = 25 unités. Les 10 unités
	// à distance demandées remplissent donc exactement le plafond de 35. Les places requises
	// par cet objectif sont toujours réservées pour éviter un blocage de progression.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|City", meta = (ClampMin = "1"))
	int32 MaxArmyUnits = 35;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|City", meta = (ClampMin = "0"))
	int32 InitialArmyUnits = 25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|City", meta = (ClampMin = "1"))
	int32 RangedProductionTarget = 10;

	// Coûts PROVISOIRES et éditables du bâtiment Aquisphères / Noxeblast.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|City", meta = (ClampMin = "0"))
	int32 RangedBuildingCrystalCost = 300;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|City", meta = (ClampMin = "0"))
	int32 RangedBuildingAbyssalMaterialCost = 25;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 TotalProducedUnits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 RangedUnitsProducedForObjective = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 RangedBuildingPlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	bool bCityBuildingPlacementArmed = false;

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetCrystals() const { return PlayerCrystals; }

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void AddCrystals(int32 Amount) { PlayerCrystals = FMath::Max(0, PlayerCrystals + Amount); }

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void GrantMissionRewards(int32 Crystals, int32 AbyssalMaterials, int32 Biomass, int32 OceanicEnergy);

	// ─── TERRITOIRE : réparation, défenses, garnison, alerte ──────────────────
	// La réparation maximale coûte seulement une fraction du prix de construction ; le coût
	// réel est proportionnel aux PV manquants et arrondi au supérieur.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Territory", meta = (ClampMin = "0"))
	int32 FullRepairCrystalCost = 13;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Territory", meta = (ClampMin = "0"))
	int32 FullRepairAbyssalMaterialCost = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Territory", meta = (ClampMin = "0"))
	int32 DefenseInstallCrystalCost = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Territory", meta = (ClampMin = "0"))
	int32 DefenseInstallAbyssalMaterialCost = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Territory", meta = (ClampMin = "1", ClampMax = "5"))
	int32 MaxTerritoryGrade = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Territory", meta = (ClampMin = "60.0"))
	float ZoneDefenseReactionWindowSeconds = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Mythic", meta = (ClampMin = "1"))
	int32 MythicGrowthBiomassGoal = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	float TerritoryBuildingMaxHealth = 16000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	float TerritoryBuildingCurrentHealth = 16000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	int32 TerritoryGrade = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	int32 InstalledDefenseCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	int32 DefenseTechnologyLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	int32 GarrisonUnits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	TArray<int32> InstalledDefenseSlots;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	TMap<FName, int32> GarrisonByUnit;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Territory")
	float ZoneDefenseWindowRemainingSeconds = 0.f;

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void SnapshotTerritoryBuilding(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	float GetTerritoryHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	int32 GetRepairCrystalCost() const;

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	int32 GetRepairAbyssalMaterialCost() const;

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	bool CanRepairTerritory() const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool RepairTerritory();

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	int32 GetDefenseCapacity() const { return FMath::Clamp(TerritoryGrade, 1, MaxTerritoryGrade); }

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	bool CanInstallNextDefense() const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool InstallNextDefense();

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	bool CanInstallDefenseAtSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool InstallDefenseAtSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	int32 GetGarrisonCapacity() const { return FMath::Clamp(TerritoryGrade * 2, 0, 10); }

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	bool CanAssignGarrisonUnit() const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool AssignGarrisonUnit();

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	int32 GetGarrisonCount(FName UnitID) const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool AssignGarrisonUnitByID(FName UnitID);

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool RemoveGarrisonUnitByID(FName UnitID);

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void StartZoneThreat();

	// Renvoie vrai si le délai vient d'expirer.
	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool TickZoneThreat(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void MarkZoneLost();

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void ResolveZoneThreat();

	UFUNCTION(BlueprintPure, Category = "Demo|Mythic")
	bool HasEnoughBiomassForMythic() const { return PlayerBiomass >= MythicGrowthBiomassGoal; }

	UFUNCTION(BlueprintCallable, Category = "Demo|Mythic")
	void RefreshBiomassGoal();

	// ─── PROGRESSION HÉROS / CITÉ / MYTHIQUE ─────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Progression", meta = (ClampMin = "1"))
	int32 HeroXPPerLevel = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Progression", meta = (ClampMin = "1"))
	int32 CityXPPerLevel = 100;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Progression")
	int32 HeroLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Progression")
	int32 HeroXP = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Progression")
	int32 CityLevel = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Progression")
	int32 CityXP = 0;

	UFUNCTION(BlueprintCallable, Category = "Demo|Progression")
	void GrantProgressionXP(int32 HeroAmount, int32 CityAmount);

	UFUNCTION(BlueprintPure, Category = "Demo|Progression")
	float GetHeroXPPercent() const;

	UFUNCTION(BlueprintPure, Category = "Demo|Progression")
	float GetCityXPPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Mythic")
	bool FeedMythicForGrowth();

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool CanAffordTerritoryBuilding(int32 CrystalCost, int32 AbyssalMaterialCost) const;

	// Dépense atomique : aucune ressource n'est retirée si l'un des deux soldes est insuffisant.
	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	bool SpendTerritoryBuildingCost(int32 CrystalCost, int32 AbyssalMaterialCost);

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	bool ArmRangedBuildingPlacement();

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	bool ConstructRangedBuildingAtPlot(int32 PlotIndex);

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool IsRangedBuildingConstructed() const { return Progress.bRangedBuildingConstructed; }

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool IsCityBuildingPlacementArmed() const { return bCityBuildingPlacementArmed; }

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetArmyUnitCount() const { return FMath::Max(0, InitialArmyUnits + TotalProducedUnits - GarrisonUnits); }

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetArmyUnitCap() const { return MaxArmyUnits; }

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetRangedProductionProgress() const { return RangedUnitsProducedForObjective; }

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool IsRangedProductionObjectiveComplete() const
	{
		return RangedUnitsProducedForObjective >= RangedProductionTarget;
	}

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool IsDefenseMissionReady() const { return Progress.bDefenseMissionReady; }

	void MarkRivalAlertShown() { Progress.bRivalAlertShown = true; }
	void SetDefenseMissionReady(bool bReady) { Progress.bDefenseMissionReady = bReady; }
	bool WasRivalAlertShown() const { return Progress.bRivalAlertShown; }

	// Coût en cristaux pour produire une unité de cette catégorie (0 = non productible ici).
	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetProductionCost(EDemoUnitCategory Category) const;

	// Vrai si la catégorie est débloquée ET abordable maintenant.
	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool CanProduce(EDemoUnitCategory Category) const;

	// Produit une unité (dépense les cristaux, l'ajoute à la réserve). Renvoie faux si refusé.
	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	bool ProduceUnit(EDemoUnitCategory Category);

	// Nombre d'unités de ce type en réserve (prêtes à déployer).
	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetReserveCount(FName UnitID) const;

	// Consomme toute la réserve (appelé quand la bataille commence -> transfert au Director).
	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void DrainReserve(TMap<FName, int32>& OutUnits);

	// ─── PROGRESSION DES BÂTIMENTS (1 bâtiment par type d'unité, amélioré indépendamment) ──
	// Le NIVEAU du bâtiment = le NIVEAU des unités qu'il produit (façon Age of Empires).
	// Voir Docs/SYSTEME_CITE_ET_DEFENSE.md. Niveau 1 par défaut, jusqu'à MaxBuildingLevel.
	static constexpr int32 MaxBuildingLevel = 3;

	// Le dernier palier (MaxBuildingLevel) exige ce Niveau Cité minimum, en plus du coût habituel
	// — donne enfin un BUT à l'XP de Cité gagnée en complétant les objectifs/batailles (demande
	// Liamor 30/07/2026 : "il n'y a aucun but à l'objectif"). PROVISOIRE, tunable.
	static constexpr int32 RequiredCityLevelForBuildingLevel3 = 3;

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetBuildingLevel(EDemoUnitCategory Category) const;

	// Coût d'amélioration du bâtiment vers le niveau suivant (0 si déjà au max).
	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetBuildingUpgradeCost(EDemoUnitCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool CanUpgradeBuilding(EDemoUnitCategory Category) const;

	// Améliore le bâtiment (dépense les cristaux). Renvoie faux si au max / insuffisant.
	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	bool UpgradeBuilding(EDemoUnitCategory Category);

	// ─── AXE / VOIE par type d'unité (onglet Compétences — Axe 1 / Axe 2 du GDD) ────────
	// 0 = base, 1 = Axe 1, 2 = Axe 2. Change l'identité tactique de tout le groupe de ce type.
	// Choix PERMANENT une fois fait (clarification Liamor 29/07/2026) : SetUnitAxis refuse
	// silencieusement tout appel hors Phase 3 préparation (Territory_Management), sans le
	// Grade 1 requis, ou si un axe est déjà choisi (0 -> 1/2 uniquement, jamais de retour en
	// arrière ni de changement d'axe). Grade 0 (phases 1&2) reste donc TOUJOURS à l'axe 0
	// (base) — comportement inchangé à l'octet près tant que le Grade n'est pas monté.
	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	int32 GetUnitAxis(EDemoUnitCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Skills")
	void SetUnitAxis(EDemoUnitCategory Category, int32 Axis);

	// ─── GRADE de compétence par type d'unité (distinct du GradeLevel de variance de spawn) ──
	// 0 = Grade 0 (base, phases 1&2, TOUJOURS le comportement par défaut). Monte à 1 (débloque
	// le choix d'Axe) puis, pour le Chef UNIQUEMENT, à 2. Amélioré en Phase 3 préparation
	// (Territory_Management) contre Cristaux/Biolumens + Minéraux Abyssaux + Énergie Océanique.
	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	int32 GetUnitGrade(EDemoUnitCategory Category) const;

	// Grade maximum atteignable : 2 pour le Chef (personnage joué), 1 pour toutes les autres
	// unités (simplification confirmée par Liamor le 29/07/2026).
	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	int32 GetMaxUnitGrade(EDemoUnitCategory Category) const;

	// Le Grade 2 du Chef (dernier palier) exige ce Niveau Héros minimum, en plus du coût
	// habituel — donne un BUT concret à l'XP Héros gagnée en jouant l'histoire (demande Liamor
	// 30/07/2026). PROVISOIRE, tunable.
	static constexpr int32 RequiredHeroLevelForChefGrade2 = 3;

	// Coût du prochain palier de Grade (Grade actuel + 1). Ressources à zéro si déjà au max.
	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	void GetUnitGradeUpgradeCost(EDemoUnitCategory Category, int32& OutCrystals,
		int32& OutAbyssalMaterials, int32& OutOceanicEnergy) const;

	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	bool CanUpgradeUnitGrade(EDemoUnitCategory Category) const;

	// Améliore le Grade (dépense les ressources). Renvoie faux si refusé (hors Phase 3, déjà
	// au max, ressources insuffisantes, catégorie non débloquée).
	UFUNCTION(BlueprintCallable, Category = "Demo|Skills")
	bool UpgradeUnitGrade(EDemoUnitCategory Category);

	// ─── PALIER complémentaire DANS l'axe déjà choisi (demande Liamor 29/07/2026) ──────────
	// Distinct du Grade (qui débloque le CHOIX d'axe) et du Grade/Axe binaire : une fois un axe
	// pris, certains effets d'axe doivent continuer à grandir progressivement au lieu d'un bonus
	// fixe unique (ex. la portée Hydrosniper/Hydropompe d'Aquisphères — "plus ils montent dans
	// cet axe, plus la portée augmente, mais jamais toute la carte"). 0 par défaut = juste le
	// bonus de base de l'axe. Monte jusqu'à MaxAxisTier, en Phase 3 prépa uniquement, contre
	// ressources — mêmes règles de sécurité que le Grade (Grade 0 / axe non choisi -> jamais
	// impacté, aucun changement pour qui n'investit pas).
	static constexpr int32 MaxAxisTier = 3;

	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	int32 GetAxisTier(EDemoUnitCategory Category) const;

	// Vrai seulement pour les catégories dont l'axe choisi doit s'améliorer par palier au lieu
	// d'un bonus fixe — toutes les unités à DISTANCE (étendu le 29/07/2026 : Aquisphères,
	// Noxeblast, Noxar, Noxedrake). Évite d'afficher un bouton Palier inutile pour les autres.
	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	bool DoesCategoryAxisScaleByTier(EDemoUnitCategory Category) const;

	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	void GetAxisTierUpgradeCost(EDemoUnitCategory Category, int32& OutCrystals,
		int32& OutAbyssalMaterials, int32& OutOceanicEnergy) const;

	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	bool CanUpgradeAxisTier(EDemoUnitCategory Category) const;

	// Améliore le palier (dépense les ressources). Renvoie faux si refusé (axe non choisi, hors
	// Phase 3, déjà au max, ressources insuffisantes, catégorie non concernée).
	UFUNCTION(BlueprintCallable, Category = "Demo|Skills")
	bool UpgradeAxisTier(EDemoUnitCategory Category);

	// ─── Fenêtre d'objectif MODALE (validation manuelle — canon v0.8) ───────────
	// Aucune phase ne s'enchaîne automatiquement : on ouvre une fenêtre (« Objectif
	// rempli », « Placez le Cristalliseur »…) et le joueur clique « Continuer ». Quand
	// il valide, OnObjectiveConfirmed est diffusé avec l'ID d'étape -> le Director agit.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Objective")
	bool bObjectiveWindowOpen = false;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Objective")
	FString ObjWinTitle;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Objective")
	FString ObjWinBody;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Objective")
	FString ObjWinButton;

	// Identifiant de l'étape en attente de validation (le Director le lit pour agir).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Objective")
	FName ObjWinStepId;

	// Vrai = fenêtre d'ÉCHEC (rouge, ex. « Le Cristalliseur a été détruit »).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Objective")
	bool bObjWinIsFailure = false;

	// Ouvre une fenêtre d'objectif. Gèle l'action tant qu'elle est ouverte.
	UFUNCTION(BlueprintCallable, Category = "Demo|Objective")
	void OpenObjectiveWindow(FName StepId, const FString& Title, const FString& Body,
		const FString& ButtonLabel = TEXT("Continuer"), bool bFailure = false);

	// Valide la fenêtre courante (clic « Continuer ») -> diffuse OnObjectiveConfirmed.
	UFUNCTION(BlueprintCallable, Category = "Demo|Objective")
	void ConfirmObjectiveWindow();

	UFUNCTION(BlueprintPure, Category = "Demo|Objective")
	bool IsObjectiveWindowOpen() const { return bObjectiveWindowOpen; }

	// Diffusé quand le joueur valide la fenêtre (paramètre = ObjWinStepId).
	UPROPERTY(BlueprintAssignable, Category = "Demo|Objective")
	FOnObjectiveConfirmed OnObjectiveConfirmed;

private:
	int32 GetRequiredCrystalReserveForRangedObjective() const;

	TWeakObjectPtr<AActor> BossActor;
	TWeakObjectPtr<AActor> CaptureObjectActor;

	EDemoPhase    CurrentPhase = EDemoPhase::None;
	FDemoProgress Progress;

	// Niveau de chaque bâtiment (clé = catégorie d'unité). Absent = niveau 1.
	TMap<EDemoUnitCategory, int32> BuildingLevels;
	// Axe/voie choisi par type d'unité (clé = catégorie). Absent = 0 (base).
	TMap<EDemoUnitCategory, int32> UnitAxes;
	// Grade de compétence par type d'unité (clé = catégorie). Absent = 0. DISTINCT du
	// GradeLevel de variance de spawn (RollUnitGrade) — pas la même notion.
	TMap<EDemoUnitCategory, int32> UnitGrades;
	// Palier d'investissement complémentaire DANS l'axe déjà choisi (clé = catégorie). Absent = 0.
	TMap<EDemoUnitCategory, int32> AxisTiers;
};
