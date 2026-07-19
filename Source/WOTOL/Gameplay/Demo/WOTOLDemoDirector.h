#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "DemoFlowSubsystem.h" // EDemoPhase (mémorisation de la phase à rejouer)
#include "WOTOLRewardActor.h"  // EWOTOLRewardType (séquence Cœur-Éclat / œuf — module 8)
#include "WOTOLDemoDirector.generated.h"

class AWOTOLDemoUnit;
class AWOTOLCaptureObject;
class AWOTOLCoverStructure;
class AWOTOLRewardActor;
class AWOTOLHeroCharacter;
class UUnitDataAsset;
class AUnitBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDemoMessage, const FString&, Message);

// ─────────────────────────────────────────────────────────────────────────────
// DIRECTOR DE DÉMO — orchestre une boucle greybox AUTO-JOUABLE en un seul niveau.
//
//   1) Bataille CRÉATURE  : armée joueur (chef + infanterie + montée) vs créature
//   2) Victoire           : débloque la distance, découvre le mythique,
//                           pose l'objet de capture (Cristalliseur/Abyssalyseur),
//                           "Zone capturée — Grade 1"
//   3) Bataille RIVALE     : armée joueur (AVEC distance) vs escouade rivale
//   4) Victoire           : objet de capture endommagé puis réparé
//   5) Fin de démo
//
// Tout s'enchaîne tout seul (idéal projection). Réutilise RTSBattleManager,
// FactionRegistrySubsystem, UnitDataRegistrySubsystem, WOTOLDemoUnit. Additif.
//
// UTILISATION : poser ce Director (ou le GameMode WOTOLGameMode_Demo) dans un
// niveau avec sol + NavMeshBoundsVolume, puis Play.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLDemoDirector();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	EFactionID DefaultPlayerFaction = EFactionID::Aquiloris;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 InfantryCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 MountedCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 RangedCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float UnitSpacing = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float ArmySeparation = 4500.f;

	// Phase 3 : bataille rangée massive (~80 unités/faction) -> plus résistante (dure ~15 min)
	// et formation ÉTALÉE sur toute la largeur du tiers (pas empilée).
	float ArmyHealthScale = 2.2f;   // PV des armées (relevé en phase 3 pour un combat long)
	bool  bGrandBattle    = false;  // vrai en phase 3 (formation large + gros roster)
	int32 SpecialCount    = 3;      // nb d'unités spéciales par armée (relevé en phase 3)

	// Compteur d'identifiants de GROUPE de formation (blocs de ~5) — unique sur toute la
	// bataille (joueur + rivale) pour agréger les étiquettes et garder les formations.
	int32 NextFormationGroupId = 0;

	// Multiplicateur de PV de la créature/boss (réduit -> phase 1 gagnable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float CreatureHealthScale = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float BattleStartDelay = 1.5f;

	// Délai entre deux phases (lecture des messages à l'écran)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float PhaseTransitionDelay = 4.f;

	// ── Introduction / nage libre connectée ───────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Exploration", meta = (ClampMin = "0.5"))
	float OpeningLoadingDuration = 2.5f;

	// Le combat se déclenche à 5–10 m du Kraken (900 uu = 9 m avec l'échelle UE standard).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Exploration", meta = (ClampMin = "500.0", ClampMax = "1000.0"))
	float EncounterTriggerDistance = 900.f;

	// Décalage Y volontaire (héros / Kraken) : le trajet direct n'est plus une ligne droite
	// figée sur X, il coupe en diagonale près des repères ajoutés à l'exploration
	// (arche, ruine à gradins — WOTOLGreyboxEnvironment::BuildArena) sans jamais forcer
	// le joueur à passer par eux (rien n'est bloquant, la nage libre reste libre).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Exploration")
	FVector ExplorationHeroOffset = FVector(-2600.f, -350.f, 260.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Exploration")
	FVector ExplorationKrakenOffset = FVector(2200.f, 450.f, 80.f);

	// Valeurs PROVISOIRES et éditables : le joueur a précisé que les nombres cités à l'oral
	// n'étaient que des exemples. Aucun de ces montants n'est considéré comme équilibrage final.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 CreatureRewardCrystals = 2200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 CreatureRewardAbyssalMaterials = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 CreatureRewardBiomass = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 CreatureRewardFood = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 CrystalliserCrystalCost = 26;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 CrystalliserAbyssalMaterialCost = 15;

	// La défense finance la réparation, la première fortification et la croissance
	// du mythique : aucune expédition intermédiaire n'est imposée dans la démo.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 DefenseRewardCrystals = 600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 DefenseRewardAbyssalMaterials = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 DefenseRewardBiomass = 70;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	int32 DefenseRewardFood = 15;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Exploration", meta = (ClampMin = "200.0"))
	float CrystalliserPlacementRadius = 520.f;

	// ── ÉQUILIBRAGE ADAPTATIF DES PERTES (phases 1, 2 et 3) ───────────────────
	// Les nombres validés sont des centres de plage. Une cible différente est tirée à chaque
	// tentative ; le système corrige doucement la pression sans dicter les victimes ni le déroulé.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Balance")
	bool bEnableAdaptiveCasualtyBalance = true;

	// Horizons de rythme, pas des durées forcées : si les pertes sont en retard à cet instant,
	// la pression atteint son maximum. La victoire peut arriver avant/après selon le joueur.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Balance", meta = (ClampMin = "60.0"))
	float KrakenCasualtyPacingSeconds = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Balance", meta = (ClampMin = "120.0"))
	float DefenseCasualtyPacingSeconds = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Balance", meta = (ClampMin = "180.0"))
	float GrandBattleCasualtyPacingSeconds = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Balance", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float AdaptiveMaxEnemyPressure = 2.4f;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Balance")
	int32 AdaptiveInitialPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Balance")
	int32 AdaptiveTargetLossMin = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Balance")
	int32 AdaptiveTargetLossPreferred = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Balance")
	int32 AdaptiveTargetLossMax = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Demo|Balance")
	float AdaptiveEnemyPressure = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TSubclassOf<AWOTOLDemoUnit> DemoUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TSubclassOf<AWOTOLCaptureObject> CaptureObjectClass;

	// ── MUSIQUE / BANDE-SON ── 3 pistes pilotées par l'ÉCRAN (elles bouclent, et ne
	// REDÉMARRENT PAS quand on passe d'un écran à un autre qui partage la même musique) :
	//   MenuMusic    -> Menu principal + Choix de faction/difficulté
	//   BattleMusic  -> Placement des unités + Bataille (jusqu'à la fin du combat)
	//   SummaryMusic -> Résumé de bataille + écran de transition
	// Dépose tes sons dans Content/Audio/Music et NOMME-les MenuMusic / BattleMusic /
	// SummaryMusic (ou assigne-les ici dans l'éditeur) -> tout se déclenche tout seul.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Audio")
	TObjectPtr<class USoundBase> MenuMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Audio")
	TObjectPtr<class USoundBase> BattleMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Audio")
	TObjectPtr<class USoundBase> SummaryMusic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float MusicVolume = 0.7f;                       // volume global de la musique

	// Composant audio courant (musique en boucle) -> pour l'arrêter en fondu.
	UPROPERTY(Transient)
	TObjectPtr<class UAudioComponent> CurrentMusic;

	// Piste actuellement jouée (pour ne PAS la relancer si l'écran change sans changer de piste).
	UPROPERTY(Transient)
	TObjectPtr<class USoundBase> CurrentMusicAsset;

	// PLUSIEURS musiques de BATAILLE (BattleMusic, BattleMusic1..N) -> tirées AU HASARD au
	// lancement d'un combat, et on enchaîne sur une AUTRE quand la piste se termine (variété).
	UPROPERTY(Transient)
	TArray<TObjectPtr<class USoundBase>> BattleTracks;

	uint8 CurrentMusicCat = 0;   // 0=aucune, 1=menu, 2=bataille, 3=résumé
	FTimerHandle MusicPollHandle;

	// Lance une musique (arrête l'ancienne en fondu). bLoop=false pour un stinger.
	void PlayMusic(class USoundBase* Music, bool bLoop);

	// Pilotage de la musique selon l'écran (catégorie), avec boucle et rotation aléatoire.
	void UpdateMusicForScreen();
	uint8 MusicCatForScreen(uint8 Screen) const;                 // écran -> catégorie
	class USoundBase* PickMusicForCat(uint8 Cat, bool bAvoidCurrent); // choisit une piste

	// Messages narratifs (le HUD/BP peut s'y abonner pour les afficher à l'écran)
	UPROPERTY(BlueprintAssignable, Category = "Demo")
	FOnDemoMessage OnDemoMessage;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartCurrentBattle();

	// Appelé UNIQUEMENT par le bouton « Lancer la partie » après faction/difficulté.
	// Enchaîne chargement narratif → nage 3D → approche Kraken → préparation RTS.
	UFUNCTION(BlueprintCallable, Category = "Demo|Exploration")
	void StartDemoAfterSelection();

	// Flux d'écrans : monte les armées en PRÉPARATION (placement, sans combat).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void BeginPreparation();

	// Lance réellement la bataille depuis la préparation (active l'IA + le boss).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartBattleNow();

	// Depuis l'écran de RÉSUMÉ phase 1 : affiche l'écran de transition narrative
	// (déblocages hors-champ : œuf mythique -> cité -> nouveau bâtiment distance).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ShowInterlude();

	// Depuis l'écran de TRANSITION : enchaîne sur la préparation de la phase 2.
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ContinueToPhase2();

	// Depuis l'écran de RÉSUMÉ final : relance la démo (même faction ou choix).
	//   bKeepFaction=true  -> rejoue directement avec la faction actuelle
	//   bKeepFaction=false -> retourne à l'écran de choix de faction
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void RestartDemo(bool bKeepFaction);

	// Depuis l'écran de RÉSUMÉ : REJOUE la phase qu'on vient de terminer/perdre (pas toute la
	// démo). Réinstalle l'état de CETTE phase (déblocages, objet de capture, terrain) puis
	// repart en préparation.
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ReplayCurrentPhase();

	// Depuis l'écran de RÉSUMÉ : revient au MENU PRINCIPAL (remet la démo à zéro et affiche
	// l'accueil).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ReturnToMainMenu();

	// Séquence interactive après le Kraken : inventaire de bâtiment -> emplacement lumineux
	// dans le monde -> dépense atomique -> construction réelle du Cristalliseur.
	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void ArmCrystalliserPlacement();

	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	bool TryPlaceCrystalliserAt(const FVector& ClickedWorldLocation);

	UFUNCTION(BlueprintPure, Category = "Demo|Flux")
	bool IsCrystalliserPlacementAvailable() const { return bCrystalliserPlacementAvailable; }

	UFUNCTION(BlueprintPure, Category = "Demo|Flux")
	bool IsCrystalliserPlacementArmed() const { return bCrystalliserPlacementArmed; }

	UFUNCTION(BlueprintPure, Category = "Demo|Flux")
	FVector GetCrystalliserPlacementLocation() const { return CrystalliserPlacementLocation; }

	// Appelé quand la dixième unité à distance est produite : affiche l'alerte noxéenne,
	// mais laisse le joueur libre d'améliorer la cité avant de cliquer « Défendre ».
	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void NotifyRangedProductionObjectiveComplete();

	// Lance la défense seulement après validation de l'alerte et conserve le Cristalliseur
	// réellement placé (aucun doublon n'est recréé au centre de l'arène).
	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void LaunchDefenseFromCity();

	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void ContinueFromBattleSummary();

	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void ReturnToCityAfterDefenseDefeat();

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void RequestTerritoryRepair();

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void RequestInstallDefense();

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void RequestAssignGarrison();

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void RequestAssignGarrisonByCategory(EDemoUnitCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void RequestRemoveGarrisonByCategory(EDemoUnitCategory Category);

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	void ArmDefensePlacement();

	UFUNCTION(BlueprintCallable, Category = "Demo|Territory")
	bool TryPlaceDefenseAt(const FVector& ClickedWorldLocation);

	UFUNCTION(BlueprintPure, Category = "Demo|Territory")
	bool IsDefensePlacementArmed() const { return bDefensePlacementArmed; }

	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void ReturnToCityAfterTerritorySecured();

	UFUNCTION(BlueprintCallable, Category = "Demo|Mythic")
	void FeedMythicAndContinue();

	// Phase à rejouer sur « Rejouer » (mémorisée à la conclusion de la bataille, avant que la
	// phase ne bascule sur DemoEnd).
	EDemoPhase ReplayPhase = EDemoPhase::Battle_Creature;

	// Décalage (depuis le centre) de la limite de placement du joueur (côté gauche -X).
	// Le joueur ne peut PAS placer/déplacer ses unités au-delà (vers le centre).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float PlacementBoundaryOffsetX = -1200.f;

	// Limite de placement en X monde (lue par le PlayerController pour clamper).
	UFUNCTION(BlueprintPure, Category = "Demo")
	float GetPlacementBoundaryWorldX() const { return GetActorLocation().X + PlacementBoundaryOffsetX; }

protected:
	virtual void BeginPlay() override;

	// ── MODULE 8 — Séquence post-créature (Cristalliseur → Cœur-Éclat → œuf) ──
	// Point d'entrée : ouvre la 1ère fenêtre d'objectif de la séquence. Enchaîné ensuite
	// par les validations de fenêtre (OnObjectiveConfirmed). Exposé pour le câblage du flux
	// (module 10) et testable directement depuis un Blueprint.
	UFUNCTION(BlueprintCallable, Category = "Demo|Flux")
	void BeginPostCreatureSequence();

	// Le flux demandé est désormais le chemin par défaut. L'interrupteur reste exposé pour
	// permettre un diagnostic de l'ancienne boucle de bataille sans supprimer le code de repli.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Flux")
	bool bEnableFullFlowV08 = true;

private:
	// Nage 3D dans la même carte procédurale que la bataille : on alterne la possession entre
	// le héros et la caméra RTS, sans OpenLevel et sans perdre l'état du GameInstance.
	void BeginOpeningExploration();
	void CheckExplorationEncounter();
	void TransitionExplorationToBattle();
	void BeginCreaturePreparationAfterExploration();
	void ResumePostBattleExploration();
	void BeginPostBattleExploration();
	void PossessBattleCamera();
	void PossessExplorationHero(const FVector& SpawnLocation, const FRotator& SpawnRotation);
	void DestroyExplorationHero();
	void BeginCrystalliserPlacement();
	void CompleteCrystalliserPlacement();
	void FinishCrystalliserConstruction();
	void CreateCrystalliserPlacementMarkers();
	void ClearCrystalliserPlacementMarkers();
	void BeginPostDefenseTransition();
	void EnterPostDefenseManagement();
	void CompleteReturnToCityAfterDefeat();
	void RefreshPostDefenseObjective();
	void TickTerritoryThreat();
	void RegisterDemoTerritoryGraph();
	void SyncFortificationToTerritoryManager();
	void RefreshDefenseStructuresFromTerritory();
	void CreateDefensePlacementMarkers();
	void ClearDefensePlacementMarkers();
	void CompleteReturnToCityAfterTerritorySecured();
	void EnterMythicGrowthInterlude();
	// Réagit à la validation d'une fenêtre d'objectif -> avance la séquence.
	UFUNCTION()
	void HandleObjectiveConfirmed(FName StepId);
	// Réagit à la récupération d'une récompense (Cœur-Éclat / œuf) par proximité.
	UFUNCTION()
	void HandleRewardCollected(EWOTOLRewardType Type);
	// Fait apparaître une récompense greybox flottante à l'emplacement donné.
	AWOTOLRewardActor* SpawnReward(EWOTOLRewardType Type, const FVector& Loc);

	UPROPERTY()
	TObjectPtr<AWOTOLRewardActor> ActiveReward;

	UPROPERTY()
	TObjectPtr<AWOTOLHeroCharacter> ExplorationHero;

	UPROPERTY()
	TObjectPtr<AWOTOLDemoUnit> ExplorationCreature;

	FTimerHandle ExplorationTransitionHandle;
	FTimerHandle ExplorationProximityHandle;
	FTimerHandle TerritoryThreatHandle;

	EFactionID ResolvePlayerFaction() const;
	EFactionID RivalOf(EFactionID Faction) const;
	int32 CountAlive(EFactionID Faction) const;

	// Noms propres à chaque faction (respect strict Aquiloris / Noxéens)
	FString BuildingDisplayName(EFactionID Faction) const;   // Cristalliseur / Abyssalyseur
	FString RangedUnitDisplayName(EFactionID Faction) const; // Aquispheres / Noxeblast
	FString MythicDisplayName(EFactionID Faction) const;     // Leviaphenix / Noxedrake

	// ── Siège du bâtiment (phase 2) : une partie de l'IA attaque l'objet de capture ──
	void SiegeTick();  // applique des dégâts au bâtiment selon les assiégeants proches
	UFUNCTION()
	void HandleCaptureDestroyed(); // bâtiment tombé à 0 -> objectif perdu -> défaite
	FTimerHandle SiegeHandle;
	float BattleStartTime = 0.f; // horodatage du début de la bataille (durée du résumé)

	// ── Cerveau tactique : ré-évalue périodiquement les 2 armées (poursuite, étagement
	//    vertical par rôle, contournement de flanc) -> comportement vivant, plus figé. ──
	void TacticalTick();
	FVector FactionCentroid(EFactionID Faction) const;
	FTimerHandle TacticalHandle;

	// Directeur de difficulté en boucle fermée : cible de pertes -> observation -> correction.
	void InitializeAdaptiveBattleBalance();
	void UpdateAdaptiveBattleBalance();
	void ConfigureAdaptiveCasualtyTargets(EDemoPhase Phase, EDemoDifficulty Difficulty,
		int32 ActualPlayerCount);
	void ReleaseAdaptiveEnemyAnchor();
	void ProtectAdaptivePlayerSurvivors();
	void ResetAdaptiveBattleBalance();
	float SamplePlayerCommandIntensity() const;
	UFUNCTION()
	void HandleAdaptivePlayerUnitDied(AUnitBase* Unit);
	FTimerHandle AdaptiveBalanceHandle;

	void SpawnPlayerArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing);
	void SpawnEnemyForCreature(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing);
	void SpawnRivalSquad(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing);
	AWOTOLDemoUnit* SpawnUnit(FName UnitID, const FVector& Loc, const FRotator& Facing,
		float ScaleBoost, float HealthScale = 2.2f, bool bAsBoss = false); // armées plus résistantes = bataille plus longue

	void LaunchBattle();
	void CheckBattleEnd();
	void OnPlayerVictory();
	void OnPlayerDefeat();
	void CleanupUnits();
	void SpawnCaptureObject(EFactionID Faction);
	void SpawnCaptureObjectAt(EFactionID Faction, const FVector& ActorLocation);
	void StartRivalDefense();
	// PHASE 3 : grande bataille rangée en ZONE NEUTRE (pas d'objectif, pas d'avantage de
	// terrain). Débloque tout le roster (spéciale + mythique), agrandit l'arène.
	void StartGrandBattle();
	void EndDemo(bool bPlayerWon);
	// Calcule les pertes par type d'unité (à partir de SpawnedUnits, morts inclus).
	void BuildBattleSummary(bool bVictory, bool bFinal, const FString& Title);
	void Say(const FString& Message);
	void FocusCameraOnPlayer(); // recadre la caméra derrière l'armée, vers l'ennemi
	void SpawnPlacementBoundary(); // marqueurs colorés de la zone de placement
	void ClearPlacementBoundary();
	void SpawnCoverStructures();   // ruines/piliers (couverture, destructibles ou non)
	void ClearCoverStructures();
	void SpawnZoneCrystals();      // (phase 2) cristaux de terraformation disséminés
	void ClearZoneCrystals();

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ZoneCrystals;

	UPROPERTY()
	TArray<TObjectPtr<AWOTOLCoverStructure>> CoverStructures;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> PlacementMarkers;

	// Aperçu holographique de l'unique emplacement valide du Cristalliseur pendant la nage
	// libre post-Kraken : une instance du VRAI bâtiment (même silhouette), en coquille
	// translucide pulsante (AWOTOLCaptureObject::SetGhostPreviewMode).
	UPROPERTY()
	TObjectPtr<class AWOTOLCaptureObject> CrystalliserGhost;

	// Délai de l'animation de construction (montée en échelle) après le clic de pose.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Economy")
	float CrystalliserConstructionSeconds = 2.5f;

	FVector CrystalliserPlacementLocation = FVector::ZeroVector;
	bool bCrystalliserPlacementAvailable = false;
	bool bCrystalliserPlacementArmed = false;
	FTimerHandle CrystalliserConstructionHandle;

	EFactionID CachedPlayerFaction = EFactionID::None;
	EFactionID CachedRivalFaction  = EFactionID::None;

	UPROPERTY()
	TArray<TObjectPtr<AWOTOLDemoUnit>> SpawnedUnits;

	UPROPERTY()
	TObjectPtr<AWOTOLCaptureObject> CaptureObject;

	// Structures de défense posées autour du bâtiment (tourelles/sentinelles).
	UPROPERTY()
	TArray<TObjectPtr<class AWOTOLDefenseStructure>> DefenseStructures;
	void ClearDefenseStructures();

	UPROPERTY()
	TArray<TObjectPtr<AActor>> DefensePlacementMarkers;

	TArray<FVector> DefenseSlotLocations;
	bool bDefensePlacementArmed = false;

	FTimerHandle BattleStartHandle;
	FTimerHandle BattleCheckHandle;
	FTimerHandle PhaseHandle;
	bool bBattleConcluded = false;

	UPROPERTY()
	TObjectPtr<AUnitBase> AdaptiveEnemyAnchor;

	EDemoPhase AdaptiveBalancePhase = EDemoPhase::None;
	int32 AdaptiveInitialEnemyCount = 0;
	float AdaptiveBalanceStartTime = 0.f;
	int32 BattleAttemptSerial = 0;
	int32 AdaptiveEncounterSeed = 0;
	int32 TacticalVariant = 0;
	float TacticalPhaseOffset = 0.f;
	float AdaptiveEncounterVariance = 1.f;
	float AdaptivePlayerCommandIntensity = 0.f;
	FRandomStream EncounterRandom;
	bool bAdaptiveBalanceActive = false;
	bool bAdaptiveSurvivorsProtected = false;
};
