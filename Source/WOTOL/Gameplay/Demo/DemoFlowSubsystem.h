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
	Exploration   UMETA(DisplayName = "Exploration — nage libre 3D")
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
	UPROPERTY(BlueprintReadOnly) bool bMythicDiscovered      = false; // juvénile découvert
	UPROPERTY(BlueprintReadOnly) bool bMythicBuildingUnlocked = false;
	UPROPERTY(BlueprintReadOnly) bool bRecruitBuildingActive = false;
	UPROPERTY(BlueprintReadOnly) bool bZoneCaptured          = false; // Grade 1 posé
	UPROPERTY(BlueprintReadOnly) bool bZoneDamaged           = false; // attaquée par la rivale
	UPROPERTY(BlueprintReadOnly) bool bZoneRepaired          = false;
	UPROPERTY(BlueprintReadOnly) bool bAllUnlocked           = false; // phase 3 : spéciale + mythique débloquées
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemoPhaseChanged,
	EDemoPhase, NewPhase, EDemoPhase, PreviousPhase);

// Diffusé quand le joueur valide une fenêtre d'objectif modale (paramètre = ID d'étape).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveConfirmed, FName, StepId);

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
	void SetSelectedFaction(EFactionID F) { SelectedFaction = F; }

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

	// Réinitialise la progression (déblocages) pour rejouer la démo depuis le début.
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ResetProgress()
	{
		Progress = FDemoProgress();
		CurrentPhase = EDemoPhase::None;
		PlayerCrystals = 0;
		PlayerAbyssalMaterials = 0;
		PlayerBiomass = 0;
		PlayerFood = 0;
		LastRewardCrystals = 0;
		LastRewardAbyssalMaterials = 0;
		LastRewardBiomass = 0;
		LastRewardFood = 0;
		ReserveUnits.Empty();
		BuildingLevels.Empty();
		UnitAxes.Empty();
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
	void SetScreen(EDemoScreen S) { Screen = S; }

	UFUNCTION(BlueprintPure, Category = "Demo")
	EDemoScreen GetScreen() const { return Screen; }

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetMessage(const FString& Msg) { CurrentMessage = Msg; }

	// Objectif courant affiché en permanence dans le HUD
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	FString ObjectiveText;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetObjective(const FString& Text) { ObjectiveText = Text; }

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

	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	int32 PlayerFood = 0;

	// Dernier lot gagné : affiché sur le résumé de bataille, sans inventer de conversion entre
	// les ressources. Remis à zéro au début d'une nouvelle récompense.
	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardCrystals = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardAbyssalMaterials = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardBiomass = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Summary")
	int32 LastRewardFood = 0;

	// Unités produites en cité, en attente de déploiement à la bataille suivante
	// (clé = ID d'unité canonique, valeur = nombre en réserve).
	UPROPERTY(BlueprintReadOnly, Category = "Demo|City")
	TMap<FName, int32> ReserveUnits;

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	int32 GetCrystals() const { return PlayerCrystals; }

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void AddCrystals(int32 Amount) { PlayerCrystals = FMath::Max(0, PlayerCrystals + Amount); }

	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	void GrantMissionRewards(int32 Crystals, int32 AbyssalMaterials, int32 Biomass, int32 Food);

	UFUNCTION(BlueprintPure, Category = "Demo|City")
	bool CanAffordTerritoryBuilding(int32 CrystalCost, int32 AbyssalMaterialCost) const;

	// Dépense atomique : aucune ressource n'est retirée si l'un des deux soldes est insuffisant.
	UFUNCTION(BlueprintCallable, Category = "Demo|City")
	bool SpendTerritoryBuildingCost(int32 CrystalCost, int32 AbyssalMaterialCost);

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

	// ─── AXE / VOIE par type d'unité (futur onglet Compétences — Axe 1 / Axe 2 du GDD) ──
	// 0 = base, 1 = Axe 1, 2 = Axe 2. Change l'identité tactique de tout le groupe de ce type.
	UFUNCTION(BlueprintPure, Category = "Demo|Skills")
	int32 GetUnitAxis(EDemoUnitCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "Demo|Skills")
	void SetUnitAxis(EDemoUnitCategory Category, int32 Axis);

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
	TWeakObjectPtr<AActor> BossActor;
	TWeakObjectPtr<AActor> CaptureObjectActor;

	EDemoPhase    CurrentPhase = EDemoPhase::None;
	FDemoProgress Progress;

	// Niveau de chaque bâtiment (clé = catégorie d'unité). Absent = niveau 1.
	TMap<EDemoUnitCategory, int32> BuildingLevels;
	// Axe/voie choisi par type d'unité (clé = catégorie). Absent = 0 (base).
	TMap<EDemoUnitCategory, int32> UnitAxes;
};
