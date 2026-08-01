#include "WOTOLDemoDirector.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLDamageNumber.h"
#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLGlow.h"
#include "OceanCurrentSubsystem.h"
#include "WOTOLCoverStructure.h"
#include "WOTOLCurrentField.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Gameplay/Battle/TerritoryStateManager.h"
#include "WOTOLDefenseStructure.h"
#include "Gameplay/Battle/UnitSelectionManager.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/Battle/WOTOLBattleCamera.h"
#include "Gameplay/Battle/WOTOLPlayerController_Battle.h"
#include "Gameplay/Exploration/WOTOLHeroCharacter.h"
#include "Core/FactionRegistrySubsystem.h"
#include "EngineUtils.h"
#include "WOTOLGreyboxEnvironment.h"
#include "WOTOLCityCamera.h"
#include "WOTOLCityEnvironment.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/UnitDataRegistrySubsystem.h"
#include "Core/WOTOLGameInstance.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

// ── COEFFICIENT DE SURVIVABILITÉ PAR FACTION ──────────────────────────────────────────────
// RÈGLE GÉNÉRALE d'équilibrage : tout réglage vaut pour LES DEUX factions, mais adapté à leur
// identité. Les Noxéens sont des GLASS CANNONS (PV + défense plus bas, DPS/agressivité plus
// hauts, stats venant de S_UnitData). À réglage identique (mêmes coups de Kraken, même échelle
// de PV), ils mourraient bien plus vite -> on COMPENSE partiellement leur fragilité pour viser
// « à peu près le même RÉSULTAT » (victoire + pertes comparables), SANS effacer leur identité
// (ils restent plus fragiles que les Aquiloris). Appliqué à TOUTE armée, joueur comme rivale,
// quelle que soit la phase -> chaque futur ajustement s'adapte automatiquement aux deux camps.
// ÉGALISATION DES FACTIONS : les deux factions doivent avoir une FORCE DE COMBAT quasi
// identique (S = mult PV × mult dégâts) pour que le joueur puisse gagner QUELLE QUE SOIT la
// faction choisie. Noxéens = glass cannon (base PV faible, DPS haut) -> on relève leur mult PV ;
// Aquiloris = tanky (base PV haute, DPS bas) -> on relève leur mult dégâts. Résultat visé :
//   S_Aquiloris = 1.00 × 1.22 = 1.220 ;  S_Noxeens = 1.10 × 1.11 = 1.221  (≈ égal).
static float FactionSurvivability(EFactionID F)
{
	switch (F)
	{
		case EFactionID::Noxeens:   return 1.10f; // fragiles -> compensation PV (equilibrage)
		case EFactionID::Aquiloris: return 1.00f; // référence (tanky, défensifs)
		default:                    return 1.00f;
	}
}

// NORMALISATEUR DE PUISSANCE PAR FACTION : conserve les identités tank/glass-cannon tout en
// rapprochant leur puissance globale. La phase 3 ajoute ensuite son asymétrie 60/100.
static float FactionDamage(EFactionID F)
{
	switch (F)
	{
		case EFactionID::Aquiloris: return 1.22f; // compense leur DPS/AoE plus faible
		case EFactionID::Noxeens:   return 1.11f; // glass cannon (mais egalise avec Aquiloris)
		default:                    return 1.00f;
	}
}

// ════════ MODÈLE D'ÉQUILIBRAGE UNIFIÉ (calculé, pas bricolé au cas par cas) ════════
//
// Objectif : le JOUEUR doit pouvoir GAGNER les 3 phases, avec n'importe quelle faction, à
// chaque difficulté — juste plus dur en Difficile qu'en Facile.
//
// Force de combat d'un camp  S ≈ (multiplicateur de PV) × (multiplicateur de dégâts).
// On vise un RATIO joueur/ennemi  R = S_joueur / S_ennemi  garanti > 1 à toutes les phases :
//     Facile ≈ 1.93   |   Normal ≈ 1.40   |   Difficile ≈ 1.11  (dur mais gagnable).
//
// Levier de difficulté = k (appliqué À L'ENNEMI seulement). Le joueur est INDÉPENDANT de la
// difficulté (baseline stable) : on ne fait que renforcer/affaiblir l'ADVERSAIRE.
//
// RÉPARTITION ASYMÉTRIQUE (kHP ≠ kDMG) : même en Facile, l'ennemi doit infliger QUELQUES
// PERTES (sinon aucun enjeu). On garde donc ses DÉGÂTS assez élevés (kDMG proche de 1) mais on
// le rend FRAGILE (kHP plus bas -> il meurt vite = facile). Le PRODUIT kHP×kDMG reste = k²
// (force de combat ennemie inchangée) -> le ratio R = 1/(kHP×kDMG) est conservé :
//     Facile   kHP 0.50 × kDMG 0.80 = 0.40  -> R≈2.50 (kHP baissé 0.65->0.50 le 01/08/2026,
//              retour terrain : une annihilation complete en Phase 3/Facile n'etait pas
//              atteignable dans le temps imparti meme en jouant bien -- combine a la reduction
//              d'effectif ennemi de SpawnRivalSquad et au chrono etendu 900->1080s)
//     Normal   kHP 0.79 × kDMG 0.90 = 0.711 -> R≈1.41
//     Difficile kHP 0.93 × kDMG 0.97 = 0.902 -> R≈1.11
static float EnemyDiffKHP(EDemoDifficulty D)
{
	switch (D)
	{
		// 0.65 -> 0.50 (01/08/2026, retour terrain) : ennemi ENCORE PLUS FRAGILE en Facile pour
		// qu'une annihilation complete de l'armee rivale reste atteignable en Phase 3.
		case EDemoDifficulty::Facile:    return 0.50f;
		case EDemoDifficulty::Difficile: return 0.93f;
		default:                         return 0.79f; // Normal
	}
}
static float EnemyDiffKDMG(EDemoDifficulty D)
{
	switch (D)
	{
		case EDemoDifficulty::Facile:    return 0.80f; // frappe ENCORE assez fort -> quelques pertes
		case EDemoDifficulty::Difficile: return 0.97f;
		default:                         return 0.90f; // Normal
	}
}

// ÉVOLUTION DU JOUEUR PAR PHASE : le joueur a progressé entre les phases (améliorations,
// niveaux) -> ses unités sont plus fortes en phase 3 qu'en phase 1. Appliqué au JOUEUR ET au
// miroir ennemi de la phase 3 (même roster « évolué ») -> le RATIO de difficulté est préservé
// (la grande bataille reste gagnable). Les ennemis distincts des phases 1/2 (Kraken, squad
// rivale) sont calibrés via k, avec la même évolution de phase.
static float PhaseEvoHP(EDemoPhase P)
{
	switch (P)
	{
		case EDemoPhase::Battle_Rival: return 1.12f; // phase 2
		case EDemoPhase::Battle_Grand: return 1.25f; // phase 3 (pleinement évolué)
		default:                       return 1.00f; // phase 1 (débuts)
	}
}
static float PhaseEvoDMG(EDemoPhase P)
{
	switch (P)
	{
		case EDemoPhase::Battle_Rival: return 1.10f; // phase 2
		case EDemoPhase::Battle_Grand: return 1.20f; // phase 3
		default:                       return 1.00f; // phase 1
	}
}

// En phase 3, les 60 Aquiloris compensent leur infériorité numérique par une cohésion plus
// forte, quel que soit le camp contrôlé. Ce bonus reste modeste : la boucle adaptative et les
// ordres réels déterminent ensuite les pertes, pas un multiplicateur de victoire caché.
static float GrandAquilorisCoordinationHP(EFactionID F, EDemoPhase P)
{
	return (F == EFactionID::Aquiloris && P == EDemoPhase::Battle_Grand) ? 1.15f : 1.f;
}

static float GrandAquilorisCoordinationDMG(EFactionID F, EDemoPhase P)
{
	return (F == EFactionID::Aquiloris && P == EDemoPhase::Battle_Grand) ? 1.12f : 1.f;
}

AWOTOLDemoDirector::AWOTOLDemoDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	DemoUnitClass      = AWOTOLDemoUnit::StaticClass();
	CaptureObjectClass = AWOTOLCaptureObject::StaticClass();
}

void AWOTOLDemoDirector::BeginPlay()
{
	Super::BeginPlay();

	CachedPlayerFaction = ResolvePlayerFaction();
	CachedRivalFaction  = RivalOf(CachedPlayerFaction);
	RegisterDemoTerritoryGraph();

	// ── MUSIQUE : si un slot n'est pas rempli dans l'éditeur, on tente de charger
	// automatiquement un son portant le bon NOM dans Content/Audio/Music (ou Content/Audio).
	// -> il suffit d'importer tes musiques et de les nommer EXACTEMENT :
	//    MenuMusic, BattleMusic, SummaryMusic.
	// On tente les DEUX emplacements probables : Content/Audio/Music/<Nom> puis Content/Audio/<Nom>.
	auto TryLoadMusic = [](TObjectPtr<USoundBase>& Slot, const TCHAR* AssetName)
	{
		if (Slot) return; // déjà assigné dans l'éditeur -> on n'écrase pas
		const FString P1 = FString::Printf(TEXT("/Game/Audio/Music/%s.%s"), AssetName, AssetName);
		Slot = LoadObject<USoundBase>(nullptr, *P1);
		if (Slot) return;
		const FString P2 = FString::Printf(TEXT("/Game/Audio/%s.%s"), AssetName, AssetName);
		Slot = LoadObject<USoundBase>(nullptr, *P2);
	};
	TryLoadMusic(MenuMusic,    TEXT("MenuMusic"));
	TryLoadMusic(BattleMusic,  TEXT("BattleMusic"));
	TryLoadMusic(SummaryMusic, TEXT("SummaryMusic"));

	// PLUSIEURS musiques de bataille : BattleMusic + BattleMusic1..BattleMusic5 si presentes.
	BattleTracks.Reset();
	if (BattleMusic) BattleTracks.Add(BattleMusic);
	for (int32 i = 1; i <= 5; ++i)
	{
		TObjectPtr<USoundBase> Extra = nullptr;
		TryLoadMusic(Extra, *FString::Printf(TEXT("BattleMusic%d"), i));
		if (Extra) BattleTracks.Add(Extra);
	}

	// Visualisation du courant (traînées dérivantes sur les couches hautes) — persistante.
	if (UWorld* W = GetWorld())
	{
		W->SpawnActor<AWOTOLCurrentField>(AWOTOLCurrentField::StaticClass(),
			GetActorLocation(), FRotator::ZeroRotator);
	}

	// On NE lance plus la bataille tout de suite : on attend le flux d'écrans
	// (menu principal → choix de faction → préparation → bataille).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetScreen(EDemoScreen::MainMenu);
			// Module 8/10 : le Director réagit aux validations des fenêtres d'objectif.
			Demo->OnObjectiveConfirmed.AddDynamic(this, &AWOTOLDemoDirector::HandleObjectiveConfirmed);
			// Bascule vers la caméra isométrique de la cité à chaque entrée dans cet écran.
			Demo->OnDemoScreenChanged.AddDynamic(this, &AWOTOLDemoDirector::HandleScreenChanged);
		}
	}

	// MUSIQUE PILOTÉE PAR L'ÉCRAN : on sonde l'écran courant ~4 fois/s et on change de piste
	// UNIQUEMENT quand la musique cible change (menu -> bataille -> résumé). Léger et robuste.
	UpdateMusicForScreen();
	GetWorldTimerManager().SetTimer(MusicPollHandle, this,
		&AWOTOLDemoDirector::UpdateMusicForScreen, 0.25f, /*bLoop=*/true);

	// FILE D'ATTENTE DE PRODUCTION (01/08/2026, demande explicite de Liamor : vraie file
	// chronométrée façon AoE4/StarCraft/Warcraft III). Tick permanent à 0.25s (même cadence que
	// le sondage musique ci-dessus) pour une barre de progression fluide côté HUD.
	GetWorldTimerManager().SetTimer(ProductionQueueHandle, this,
		&AWOTOLDemoDirector::TickProductionQueue, 0.25f, /*bLoop=*/true);
}

void AWOTOLDemoDirector::TickProductionQueue()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->TickProductionQueues(0.25f);
			// Un ordre à distance peut désormais finir SEUL, sans clic (le minuteur tourne même
			// écran fermé) -> revérifier ici plutôt qu'uniquement au clic PRODUIRE. Idempotent
			// (NotifyRangedProductionObjectiveComplete se protège via WasRivalAlertShown()).
			NotifyRangedProductionObjectiveComplete();
		}
	}
}

void AWOTOLDemoDirector::StartDemoAfterSelection()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || Demo->SelectedFaction == EFactionID::None) return;

	CachedPlayerFaction = ResolvePlayerFaction();
	CachedRivalFaction  = RivalOf(CachedPlayerFaction);
	bEnableFullFlowV08  = true;
	bCrystalliserPlacementAvailable = false;
	bCrystalliserPlacementArmed = false;
	ClearCrystalliserPlacementMarkers();

	GetWorldTimerManager().ClearTimer(ExplorationTransitionHandle);
	GetWorldTimerManager().ClearTimer(ExplorationProximityHandle);
	Demo->SetPhase(EDemoPhase::Exploration_Creature);
	Demo->SetMessage(TEXT("Une nouvelle zone inconnue a ete localisee..."));
	Demo->SetScreen(EDemoScreen::Loading);

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::BeginOpeningExploration,
		FMath::Max(0.5f, OpeningLoadingDuration), false);
}

void AWOTOLDemoDirector::PossessExplorationHero(const FVector& SpawnLocation,
	const FRotator& SpawnRotation)
{
	DestroyExplorationHero();
	UWorld* W = GetWorld();
	if (!W) return;

	FActorSpawnParameters P;
	P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ExplorationHero = W->SpawnActor<AWOTOLHeroCharacter>(
		AWOTOLHeroCharacter::StaticClass(), SpawnLocation, SpawnRotation, P);

	if (APlayerController* PC = W->GetFirstPlayerController())
	{
		if (ExplorationHero) PC->Possess(ExplorationHero);
		PC->bShowMouseCursor = true; // l'introduction modale doit pouvoir être validée
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
}

void AWOTOLDemoDirector::DestroyExplorationHero()
{
	if (ExplorationHero)
	{
		ExplorationHero->Destroy();
		ExplorationHero = nullptr;
	}
}

void AWOTOLDemoDirector::PossessBattleCamera()
{
	UWorld* W = GetWorld();
	if (!W) return;
	AWOTOLPlayerController_Battle* PC = Cast<AWOTOLPlayerController_Battle>(W->GetFirstPlayerController());
	if (!PC) return;
	for (TActorIterator<AWOTOLBattleCamera> It(W); It; ++It)
	{
		PC->SetBattleCamera(*It);
		break;
	}
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

void AWOTOLDemoDirector::PossessCityCamera()
{
	UWorld* W = GetWorld();
	if (!W) return;
	APlayerController* PC = W->GetFirstPlayerController();
	if (!PC) return;

	FVector Hub = FVector::ZeroVector;
	for (TActorIterator<AWOTOLCityEnvironment> ItEnv(W); ItEnv; ++ItEnv)
	{
		Hub = ItEnv->GetHubLocation();
		break;
	}
	for (TActorIterator<AWOTOLCityCamera> It(W); It; ++It)
	{
		It->ResetToHub(Hub);
		PC->Possess(*It);
		break;
	}
	PC->bShowMouseCursor = true;
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(Mode);
}

void AWOTOLDemoDirector::HandleScreenChanged(EDemoScreen NewScreen)
{
	// Les autres écrans possèdent déjà explicitement la bonne caméra à chaque point d'entrée
	// existant (PossessBattleCamera / PossessExplorationHero) -> on n'agit ici QUE pour City,
	// seul écran qui n'avait jusqu'ici aucune caméra 3D dédiée (juste un Canvas plein écran).
	if (NewScreen == EDemoScreen::City)
	{
		PossessCityCamera();
	}

	// BUG CORRIGE (retour terrain 31/07/2026, recherche dédiée) : AWOTOLGreyboxEnvironment
	// (brouillard/post-process/ciel de bataille) est créé UNE SEULE FOIS pour toute la session
	// et jamais détruit -> son ambiance (tous ses volumes en bUnbound=true) restait active sur
	// TOUS les écrans, y compris la Cité, la poussant vers un bleu bien plus saturé que sa
	// couleur codée. Désactivée uniquement en Cité (seul écran où le probleme a été confirmé) ;
	// réactivée pour tous les autres (bataille/exploration, où elle a été conçue).
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AWOTOLGreyboxEnvironment> It(W); It; ++It)
		{
			It->SetAtmosphereActive(NewScreen != EDemoScreen::City);
			break;
		}
	}
}

void AWOTOLDemoDirector::BeginOpeningExploration()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	CleanupUnits();
	ClearPlacementBoundary();
	ClearCoverStructures();
	Demo->SetBoss(nullptr);
	Demo->SetPhase(EDemoPhase::Exploration_Creature);
	Demo->SetScreen(EDemoScreen::Exploration);
	Demo->SetObjective(TEXT("Explorez la zone et approchez-vous de la creature inconnue"));

	const FVector Center = GetActorLocation();
	PossessExplorationHero(Center + ExplorationHeroOffset, FRotator(0.f, 0.f, 0.f));

	// Le même Kraken greybox est visible au loin, cerveau désactivé. À l'approche il sera
	// détruit puis recréé par BeginPreparation avec ses PV et son IA de combat complets.
	SpawnEnemyForCreature(CachedRivalFaction, Center + ExplorationKrakenOffset,
		FRotator(0.f, 180.f, 0.f));
	ExplorationCreature = Cast<AWOTOLDemoUnit>(Demo->GetBoss());
	if (ExplorationCreature) ExplorationCreature->bCreatureBrain = false;

	const FString IntroBody = CachedPlayerFaction == EFactionID::Noxeens
		? TEXT("Depuis leurs failles bioluminescentes, les Noxeens etendent leur influence.\n"
			"Une expedition vient de decouvrir une zone inconnue : identifiez la menace qui s'y cache.")
		: TEXT("Depuis Aquilor, les Aquiloris protegent les cristaux d'energie des profondeurs.\n"
			"Une expedition vient de decouvrir une zone inconnue : identifiez la menace qui s'y cache.");
	Demo->OpenObjectiveWindow(TEXT("intro_begin_exploration"),
		CachedPlayerFaction == EFactionID::Noxeens ? TEXT("LES NOXEENS") : TEXT("LES AQUILORIS"),
		IntroBody,
		TEXT("COMMENCER L'EXPLORATION"));

	GetWorldTimerManager().SetTimer(ExplorationProximityHandle, this,
		&AWOTOLDemoDirector::CheckExplorationEncounter, 0.15f, true);
}

void AWOTOLDemoDirector::CheckExplorationEncounter()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || Demo->IsObjectiveWindowOpen() || !ExplorationHero || !ExplorationCreature) return;

	const float Dist = FVector::Dist(ExplorationHero->GetActorLocation(),
		ExplorationCreature->GetActorLocation());
	if (Dist <= EncounterTriggerDistance) TransitionExplorationToBattle();
}

void AWOTOLDemoDirector::TransitionExplorationToBattle()
{
	GetWorldTimerManager().ClearTimer(ExplorationProximityHandle);
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	Demo->SetMessage(TEXT("Creature detectee — deploiement de l'armee..."));
	Demo->SetScreen(EDemoScreen::Loading);
	PossessBattleCamera();
	DestroyExplorationHero();

	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::BeginCreaturePreparationAfterExploration, 1.5f, false);
}

void AWOTOLDemoDirector::BeginCreaturePreparationAfterExploration()
{
	if (UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr)
	{
		Demo->SetPhase(EDemoPhase::Battle_Creature);
	}
	BeginPreparation();
	ExplorationCreature = nullptr;
}

void AWOTOLDemoDirector::ResumePostBattleExploration()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	Demo->SetMessage(TEXT("Retour dans la zone liberee..."));
	Demo->SetScreen(EDemoScreen::Loading);
	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::BeginPostBattleExploration, 1.2f, false);
}

void AWOTOLDemoDirector::BeginPostBattleExploration()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	CleanupUnits();
	Demo->SetBoss(nullptr);
	Demo->SetPhase(EDemoPhase::Capture_Zone);
	Demo->SetScreen(EDemoScreen::Exploration);
	Demo->SetObjective(TEXT("Consultez le rapport puis purifiez la nouvelle zone"));
	PossessExplorationHero(GetActorLocation() + FVector(-1200.f, 0.f, 260.f),
		FRotator(0.f, 0.f, 0.f));
	BeginPostCreatureSequence();
}

uint8 AWOTOLDemoDirector::MusicCatForScreen(uint8 Screen) const
{
	switch (static_cast<EDemoScreen>(Screen))
	{
	case EDemoScreen::MainMenu:
	case EDemoScreen::FactionSelect:
	case EDemoScreen::HeroCustomization:
	case EDemoScreen::PreGameSummary:
	case EDemoScreen::Loading:
	case EDemoScreen::City:
	case EDemoScreen::Skills:        return 1; // menu / préparation narrative
	case EDemoScreen::Prepare:
	case EDemoScreen::Playing:
	case EDemoScreen::Exploration:
	case EDemoScreen::Territory:     return 2; // monde 3D + bataille
	case EDemoScreen::Summary:
	case EDemoScreen::Interlude:     return 3; // résumé
	default:                         return 0;
	}
}

USoundBase* AWOTOLDemoDirector::PickMusicForCat(uint8 Cat, bool bAvoidCurrent)
{
	if (Cat == 1) return MenuMusic;
	if (Cat == 3) return SummaryMusic;
	if (Cat == 2)
	{
		if (BattleTracks.Num() == 0) return BattleMusic; // secours
		if (BattleTracks.Num() == 1) return BattleTracks[0];
		// Tirage AU HASARD ; si demandé, on EVITE la piste courante (pas 2 fois de suite).
		for (int32 Try = 0; Try < 8; ++Try)
		{
			USoundBase* Cand = BattleTracks[FMath::RandRange(0, BattleTracks.Num() - 1)];
			if (!bAvoidCurrent || Cand != CurrentMusicAsset) return Cand;
		}
		return BattleTracks[0];
	}
	return nullptr;
}

void AWOTOLDemoDirector::UpdateMusicForScreen()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	const uint8 Cat = MusicCatForScreen(static_cast<uint8>(Demo->GetScreen()));

	if (Cat != CurrentMusicCat)
	{
		// Changement de catégorie (menu -> bataille -> résumé…) : nouvelle piste.
		CurrentMusicCat = Cat;
		CurrentMusicAsset = PickMusicForCat(Cat, /*bAvoidCurrent=*/false);
		PlayMusic(CurrentMusicAsset, /*bLoop=*/true);
		return;
	}

	// MÊME catégorie : on ne coupe pas. Si la piste est arrivée au bout -> on enchaîne.
	if (CurrentMusic && !CurrentMusic->IsPlaying())
	{
		if (Cat == 2 && BattleTracks.Num() > 1)
		{
			// Bataille : on passe à une AUTRE musique aléatoire (variété, pas de répétition).
			CurrentMusicAsset = PickMusicForCat(Cat, /*bAvoidCurrent=*/true);
			PlayMusic(CurrentMusicAsset, /*bLoop=*/true);
		}
		else if (CurrentMusicAsset)
		{
			CurrentMusic->Play(); // menu/résumé (ou 1 seule piste) : simple boucle
		}
	}
}

// ── MUSIQUE ── Joue une musique (arrête l'ancienne en fondu). bLoop=true = boucle
// (préparation/combat), bLoop=false = stinger ponctuel (victoire/défaite).
void AWOTOLDemoDirector::PlayMusic(USoundBase* Music, bool bLoop)
{
	// Fondu de sortie de la musique en cours.
	if (CurrentMusic)
	{
		CurrentMusic->FadeOut(1.2f, 0.f);
		CurrentMusic = nullptr;
	}
	if (!Music) return;

	// SpawnSound2D : joue un son "2D" (non spatialisé) = parfait pour de la musique.
	// bAutoDestroy = false pour une boucle (on la garde pour l'arrêter plus tard),
	// true pour un stinger (il se détruit tout seul à la fin).
	UAudioComponent* AC = UGameplayStatics::SpawnSound2D(
		this, Music, MusicVolume, 1.f, 0.f, nullptr, false, /*bAutoDestroy=*/!bLoop);
	if (AC && bLoop)
	{
		CurrentMusic = AC; // on ne garde que les boucles (pour le fondu de sortie)
	}
}

void AWOTOLDemoDirector::SetMusicVolume(float NewVolume)
{
	MusicVolume = FMath::Clamp(NewVolume, 0.f, 1.f);
	if (CurrentMusic)
	{
		CurrentMusic->SetVolumeMultiplier(MusicVolume);
	}
}

// Monte les armées en PRÉPARATION (placement libre), SANS lancer le combat.
// Fonctionne pour LES DEUX phases (créature ou défense rivale) selon la phase courante.
void AWOTOLDemoDirector::BeginPreparation()
{
	// (La musique est gérée automatiquement par l'écran -> BattleMusic dès l'écran Prepare.)
	CachedPlayerFaction = ResolvePlayerFaction();
	CachedRivalFaction  = RivalOf(CachedPlayerFaction);
	// Nouvelle tentative = nouvelle lecture tactique. Le seed mélange le temps, la phase et
	// le numéro d'essai ; il est journalisé pour pouvoir reproduire un cas de test précis.
	++BattleAttemptSerial;
	AdaptiveEncounterSeed = static_cast<int32>(FPlatformTime::Cycles64())
		^ (BattleAttemptSerial * 7919);
	EncounterRandom.Initialize(AdaptiveEncounterSeed);
	TacticalVariant = EncounterRandom.RandRange(0, 3);
	TacticalPhaseOffset = EncounterRandom.FRandRange(0.f, 17.f);
	AdaptiveEncounterVariance = EncounterRandom.FRandRange(0.92f, 1.08f);

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (Demo)
	{
		Demo->LastRewardCrystals = 0;
		Demo->LastRewardAbyssalMaterials = 0;
		Demo->LastRewardBiomass = 0;
		Demo->LastRewardOceanicEnergy = 0;
	}
	// Première prépa (menu) : on est sur la phase créature.
	if (Demo && Demo->GetPhase() == EDemoPhase::None)
	{
		Demo->SetPhase(EDemoPhase::Battle_Creature);
	}
	const EBattleType BT = Demo ? Demo->GetCurrentBattleType() : EBattleType::CreatureEncounter;
	const bool bGrand = Demo && Demo->GetPhase() == EDemoPhase::Battle_Grand; // phase 3, zone neutre

	// Phase 2 (défense rivale) : 35 contre 35 (25 unités de base + les 10 distances
	// produites côté joueur). Valeurs encore mesurées pour rester fluides sur portable.
	bGrandBattle = bGrand;
	if (bGrand)
	{
		// Les compositions exactes sont attribuées par faction dans SpawnPlayerArmy et
		// SpawnRivalSquad : Aquiloris 60, Noxéens 100, quel que soit le camp contrôlé.
		InfantryCount = 20; MountedCount = 12; RangedCount = 18; SpecialCount = 8;
		// Bataille jugee TROP COURTE (~3 min sur un budget de 15). On AUGMENTE fortement les PV
		// des deux armees pour ETIRER l'affrontement : plus les unites encaissent, plus la
		// bataille dure. Reste gagnable (le joueur perd deja ~la moitie de son armee). [Reglable]
		ArmyHealthScale = 6.5f;
	}
	else if (BT == EBattleType::RivalDefense)
	{
		// Phase 2 = GRANDE bataille : plus d'unités des deux côtés -> siège plus long et
		// plus disputé (vise >= 5 min). Reste mesuré pour la fluidité sur portable.
		InfantryCount = 16; MountedCount = 8; RangedCount = 10; SpecialCount = 3;
		ArmyHealthScale = 2.2f;
	}
	else
	{
		// PHASE 1 (Kraken) : 2.0x -> armee solide mais pas increvable. Le Kraken fait des pertes
		// via son ecrasement (dose plus bas qu'avant), mais la bataille reste GAGNABLE.
		SpecialCount = 3; ArmyHealthScale = 2.0f;
	}

	CleanupUnits(); // repart d'une armée propre (utile en phase 2)
	bBattleConcluded = false;

	// REMET LE CHRONO À 10:00 dès la préparation (sinon il affiche le reliquat de la
	// phase précédente). Nouvelle bataille = tout repart à zéro.
	if (UWorld* W = GetWorld())
		if (URTSBattleManager* RTS = W->GetSubsystem<URTSBattleManager>())
			RTS->ResetForNewBattle(bGrand ? 900.f : 600.f); // phase 3 : grande bataille -> plus de temps (~15 min)

	// On VIDE la sélection : sinon le HUD (barre de commandement bas-gauche) continue
	// d'afficher le roster de la phase précédente (unités désormais détruites/différentes).
	// Le joueur re-sélectionnera ses nouvelles unités et le HUD se réaffichera alors.
	if (UWorld* W = GetWorld())
		if (UUnitSelectionManager* Sel = W->GetSubsystem<UUnitSelectionManager>())
			Sel->ClearSelection();

	// Nouveau COURANT océanique (sens + intensité) pour cette bataille.
	if (UWorld* W = GetWorld())
	{
		if (UOceanCurrentSubsystem* Cur = W->GetSubsystem<UOceanCurrentSubsystem>())
		{
			Cur->Regenerate();
		}
	}
	const FVector Center = GetActorLocation();
	// PHASE 3 : on AVANCE les deux armées vers la limite de placement (elles occupent l'AVANT
	// de leur tiers, près de la barrière), au lieu d'être collées au fond contre les rochers.
	const float FwdShift = bGrand ? 900.f : 0.f;
	const FVector PlayerOrigin = Center + FVector(-ArmySeparation * 0.5f + FwdShift, 0.f, 0.f);
	const FVector EnemyOrigin  = Center + FVector( ArmySeparation * 0.5f - FwdShift, 0.f, 0.f);
	SpawnPlayerArmy(CachedPlayerFaction, PlayerOrigin, FRotator(0.f, 0.f, 0.f));
	if (BT == EBattleType::RivalDefense)
	{
		SpawnRivalSquad(CachedRivalFaction, EnemyOrigin, FRotator(0.f, 180.f, 0.f));
	}
	else
	{
		SpawnEnemyForCreature(CachedRivalFaction, EnemyOrigin, FRotator(0.f, 180.f, 0.f));
	}
	if (BT == EBattleType::RivalDefense)
	{
		RefreshDefenseStructuresFromTerritory();
	}

	SpawnPlacementBoundary(); // barrière visuelle : zone de placement = ton premier tiers
	SpawnCoverStructures();   // ruines Éthériennes (couverture au centre de l'arène)
	// Cristaux de terraformation : UNIQUEMENT en phase 2 (zone acquise). Phase 3 = zone
	// NEUTRE -> aucun cristal, aucun avantage de terrain.
	if (BT == EBattleType::RivalDefense && !bGrand) SpawnZoneCrystals();
	else                                            ClearZoneCrystals();
	FocusCameraOnPlayer();
	if (Demo)
	{
		Demo->SetScreen(EDemoScreen::Prepare);
		// BUG DE COMMUNICATION CORRIGE (01/08/2026, retour terrain : "j'ai 47 ennemis encore en
		// face a 3:30 de la fin... j'aurais pas pu detruire la totalite de l'armee") : le texte
		// "Mettez la faction rivale en DEROUTE" laissait croire qu'il fallait ANEANTIR l'ennemi.
		// Le VRAI critere de victoire au chrono ecoule (cf. CheckBattleEnd, bGrandPhase) est
		// "avoir PLUS d'unites vivantes que l'ennemi", pas "tuer tout le monde" -> explicite
		// maintenant pour eviter la panique/confusion en fin de partie.
		Demo->SetObjective(bGrand
			? FString(TEXT("PHASE 3 — Ayez PLUS d'unites vivantes que l'ennemi au temps limite (annihilation totale non requise)"))
			: (BT == EBattleType::RivalDefense
				? FString::Printf(TEXT("Proteger le %s — ne le laissez pas tomber a 0"),
					*BuildingDisplayName(CachedPlayerFaction))
				: FString(TEXT("Vaincre la creature — le KRAKEN"))));
	}
	Say(TEXT("PREPARATION : placez vos unites dans VOTRE zone (barriere coloree), puis lancez."));
	UE_LOG(LogTemp, Log, TEXT("[WOTOL Encounter] Attempt=%d Seed=%d Variant=%d Variance=%.3f"),
		BattleAttemptSerial, AdaptiveEncounterSeed, TacticalVariant, AdaptiveEncounterVariance);
}

void AWOTOLDemoDirector::StartBattleNow()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EBattleType BT = Demo ? Demo->GetCurrentBattleType() : EBattleType::CreatureEncounter;

	// Active le cerveau autonome du boss (il attendait pendant la prépa)
	if (Demo)
	{
		if (AWOTOLDemoUnit* Boss = Cast<AWOTOLDemoUnit>(Demo->GetBoss()))
		{
			Boss->bCreatureBrain = true;
		}
		Demo->SetScreen(EDemoScreen::Playing);
	}

	ClearPlacementBoundary(); // la barrière disparaît quand la bataille commence

	const bool bGrandSay = Demo && Demo->GetPhase() == EDemoPhase::Battle_Grand;
	Say(bGrandSay
		? TEXT("Phase 3 — Bataille rangee ! Mettez la rivale en DEROUTE !")
		: (BT == EBattleType::RivalDefense
			? TEXT("Phase 2 — La faction rivale attaque ! Defendez la zone !")
			: TEXT("Phase 1 — Bataille contre le Kraken. Aneantissez-le !")));
	LaunchBattle();

	// Dès le lancement, TOUTES les unités du joueur sont déjà sélectionnées (groupe entier)
	// -> le roster complet s'affiche, prêt à recevoir des ordres, sans clic préalable.
	if (UWorld* W = GetWorld())
	{
		if (UUnitSelectionManager* Sel = W->GetSubsystem<UUnitSelectionManager>())
		{
			Sel->SelectAllOfFaction(ResolvePlayerFaction());
		}
	}
}

EFactionID AWOTOLDemoDirector::ResolvePlayerFaction() const
{
	// 1) Source fiable : le subsystem de démo (rempli au choix de faction).
	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			if (Demo->SelectedFaction != EFactionID::None) return Demo->SelectedFaction;
		}
	}
	// 2) Repli : le GameInstance WOTOL (si configuré).
	if (const UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		const EFactionID F = GI->GetSelectedFaction();
		if (F != EFactionID::None) return F;
	}
	return DefaultPlayerFaction;
}

EFactionID AWOTOLDemoDirector::RivalOf(EFactionID Faction) const
{
	return (Faction == EFactionID::Aquiloris) ? EFactionID::Noxeens : EFactionID::Aquiloris;
}

FString AWOTOLDemoDirector::BuildingDisplayName(EFactionID Faction) const
{
	return (Faction == EFactionID::Noxeens) ? TEXT("Abyssalyseur") : TEXT("Cristalliseur");
}

FString AWOTOLDemoDirector::RangedUnitDisplayName(EFactionID Faction) const
{
	return (Faction == EFactionID::Noxeens) ? TEXT("Noxeblast") : TEXT("Aquispheres");
}

FString AWOTOLDemoDirector::MythicDisplayName(EFactionID Faction) const
{
	return (Faction == EFactionID::Noxeens) ? TEXT("Noxedrake") : TEXT("Leviaphenix");
}

int32 AWOTOLDemoDirector::CountAlive(EFactionID Faction) const
{
	int32 Alive = 0;
	if (UWorld* W = GetWorld())
	{
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction(Faction))
			{
				if (U && U->IsAlive()) ++Alive;
			}
		}
	}
	return Alive;
}

void AWOTOLDemoDirector::StartCurrentBattle()
{
	bBattleConcluded = false;

	const FVector Center = GetActorLocation();
	const FVector PlayerOrigin = Center + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
	const FVector EnemyOrigin  = Center + FVector( ArmySeparation * 0.5f, 0.f, 0.f);
	const FRotator FaceRight(0.f, 0.f, 0.f);
	const FRotator FaceLeft(0.f, 180.f, 0.f);

	SpawnPlayerArmy(CachedPlayerFaction, PlayerOrigin, FaceRight);

	EBattleType BattleType = EBattleType::CreatureEncounter;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			BattleType = Demo->GetCurrentBattleType();
		}
	}

	if (BattleType == EBattleType::RivalDefense)
	{
		SpawnRivalSquad(CachedRivalFaction, EnemyOrigin, FaceLeft);
	}
	else
	{
		SpawnEnemyForCreature(CachedRivalFaction, EnemyOrigin, FaceLeft);
	}

	GetWorldTimerManager().SetTimer(
		BattleStartHandle, this, &AWOTOLDemoDirector::LaunchBattle,
		FMath::Max(0.1f, BattleStartDelay), false);
}

void AWOTOLDemoDirector::SpawnPlayerArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	// Formation d'armée STRUCTURÉE (vers +X = l'ennemi).
	//   - "Avant" (vers l'ennemi) = +X ; les rangées arrière sont en -X.
	//   - Chef devant, centré
	//   - Infanterie : 2 paquets de 5 (gauche/droite) avec un espace central
	//   - Montée : une rangée alignée derrière l'infanterie
	//   - Distance : une rangée alignée tout à l'arrière (si débloquée)
	const float Lat   = UnitSpacing;          // espacement latéral (Y)
	// En phase 3 (grande bataille), il y a 5 catégories empilées : on RESSERRE la profondeur
	// pour que la dernière (mythique) reste DANS l'arène, sans jamais toucher les montagnes.
	const float Depth = UnitSpacing * (bGrandBattle ? 1.15f : 1.5f); // espacement entre rangées (X)
	const float GroundZ = 100.f;
	// PV du JOUEUR = échelle de bataille × égalisation de faction × ÉVOLUTION DE PHASE.
	// Volontairement INDÉPENDANT de la difficulté : le joueur est une baseline stable, c'est
	// l'ENNEMI qui est renforcé/affaibli (voir EnemyDiffK) -> garantit la winnabilité.
	const float PScale = ArmyHealthScale * FactionSurvivability(Faction)
		* PhaseEvoHP(Demo->GetPhase())
		* GrandAquilorisCoordinationHP(Faction, Demo->GetPhase());

	// ── RENFORTS DE CITÉ (pattern Total War / XCOM) : les unités PRODUITES en cité
	// (réserve) rejoignent l'armée pour cette bataille. On draine la réserve et on gonfle
	// les effectifs par catégorie. Réserve vide (ex. bataille créature) = aucun effet. ──
	int32 EffInfantry = InfantryCount, EffMounted = MountedCount;
	// En phase 2, aucune unité à distance gratuite : celles que le joueur vient réellement
	// de produire constituent tout le contingent. La dotation de scénario revient seulement
	// pour la grande bataille finale, après l'ellipse temporelle.
	int32 EffRanged   = bGrandBattle ? RangedCount : 0;
	int32 EffSpecial  = SpecialCount;
	if (bGrandBattle)
	{
		// PHASE 3 (JOUEUR) : base fixe réduite (vétérans de la phase 2 ; chef + mythique
		// ajoutés séparément plus bas) — le reste vient du recrutement libre en cité, drainé
		// juste en dessous comme pour les phases 1/2 (demande de Liamor du 26/07/2026: fini
		// la composition scriptée imposée, place au recrutement joueur jusqu'au plafond
		// faction réglé dans ReturnToCityForGrandBattleReveal).
		EffInfantry = GrandBattleBaselineInfantry;
		EffMounted  = GrandBattleBaselineMounted;
		EffRanged   = GrandBattleBaselineRanged;
		EffSpecial  = GrandBattleBaselineSpecial;
	}
	{
		// Un ordre déjà PAYÉ mais dont le minuteur n'a pas fini ne doit jamais être perdu au
		// départ en bataille (01/08/2026, vraie file d'attente chronométrée) -> on le termine
		// instantanément avant de drainer, plutôt que de faire perdre les cristaux au joueur.
		Demo->CompleteAllQueuedProduction();
		TMap<FName, int32> Reserve;
		Demo->DrainReserve(Reserve); // phases 1/2/3 : toujours les unités RÉELLEMENT recrutées
		for (const TPair<FName, int32>& Pair : Reserve)
		{
			switch (UDemoFlowSubsystem::GetCategoryForUnit(Pair.Key))
			{
				case EDemoUnitCategory::Infanterie: EffInfantry += Pair.Value; break;
				case EDemoUnitCategory::Distance:   EffRanged   += Pair.Value; break;
				case EDemoUnitCategory::Montee:     EffMounted  += Pair.Value; break;
				case EDemoUnitCategory::Speciale:   EffSpecial  += Pair.Value; break;
				default: break; // chef / mythique : gérés séparément
			}
		}
	}

	// Place un groupe en rangées (se replie sur plusieurs lignes vers l'arrière -X). En
	// phase 3, les rangées sont bien plus LARGES -> la ligne s'étale sur la largeur du tiers
	// (fini l'empilement). Toutes les unités reçoivent l'échelle de PV de la bataille.
	// Curseur de PROFONDEUR partagé : chaque CATÉGORIE occupe SA/SES propre(s) rangée(s) et
	// on avance le curseur du NOMBRE RÉEL de rangées qu'elle utilise + un espace de séparation.
	// => JAMAIS deux types d'unités différents sur la même ligne (fini le chevauchement des
	// Aquilances sur la rangée des Aquisphères vu en phase 2).
	const float GroupGap = Depth * (bGrandBattle ? 0.5f : 1.0f); // couloir vide entre deux catégories

	// DÉPLOIEMENT PAR BLOCS DE ~5 (style Total War) : chaque catégorie est découpée en petits
	// groupes de 5 qui forment un mini-carré (2-1-2, l'unité CENTRALE porte l'étiquette) ou une
	// LIGNE (si <5). Les blocs sont TUILÉS sur la LARGEUR (Y) puis sur la PROFONDEUR (X) -> des
	// groupes bien distincts, espacés, sur la largeur ET la longueur de la zone de placement.
	// Espacements GÉNÉREUX (fini l'empilement) : les blocs sont bien séparés et occupent la
	// LARGEUR de la zone. La PROFONDEUR est économisée en plaçant les unités À DISTANCE et
	// SPÉCIALES sur la COUCHE SUPÉRIEURE (au-dessus de la mêlée) au lieu de les empiler derrière.
	const float IntraY = Lat * 0.95f;     // écart latéral DANS un bloc
	const float IntraX = Depth * 0.85f;   // écart de profondeur DANS un bloc
	const float BlockStepY = Lat * 4.2f;  // pas entre blocs (Y) — bien AÉRÉ
	const float BlockStepX = Depth * 3.3f;// pas entre bandes de blocs (X)
	// ÉCARTEMENT PROPORTIONNEL À LA TAILLE (montures/mythiques = beaucoup plus espacés).
	auto SizeFactor = [](FName Id) -> float
	{
		const FString S = Id.ToString();
		if (S.Contains(TEXT("Noxebeast")) || S.Contains(TEXT("Aquilances"))) return 2.6f; // montures massives
		if (S.Contains(TEXT("Leviaphenix")) || S.Contains(TEXT("Noxedrake"))) return 3.2f; // mythiques
		if (S.Contains(TEXT("Noxeons")))     return 1.8f;                                    // organismes larges
		return 1.0f;
	};
	// Place une catégorie en blocs de 5. Cursor = profondeur (par ré-usage : mêlée au sol vs
	// tireurs en l'air ont chacun LEUR curseur, ce qui les fait se SUPERPOSER en XY sur des
	// couches différentes -> on gagne de la profondeur). LayerZ = hauteur de couche (0 = sol).
	auto PlaceBlocks = [&](FName Id, int32 Count, int32 BlocksPerBand, float& Cursor, float LayerZ)
	{
		if (Id.IsNone() || Count <= 0) return;
		BlocksPerBand = FMath::Max(1, BlocksPerBand);
		const float SF = SizeFactor(Id);
		const float IY = IntraY * SF, IX = IntraX * SF;
		const float BSY = BlockStepY * SF, BSX = BlockStepX * SF;
		const int32 NumBlocks = (Count + 4) / 5;
		int32 BandsUsed = 0;
		for (int32 b = 0; b < NumBlocks; ++b)
		{
			const int32 Band = b / BlocksPerBand, ColB = b % BlocksPerBand;
			BandsUsed = FMath::Max(BandsUsed, Band + 1);
			const int32 InThisBand = FMath::Min(BlocksPerBand, NumBlocks - Band * BlocksPerBand);
			const float BlockY = (ColB - (InThisBand - 1) * 0.5f) * BSY;
			const float BlockX = -Cursor - Band * BSX;
			const int32 N = FMath::Min(5, Count - b * 5);
			const int32 Gid = NextFormationGroupId++;
			for (int32 s = 0; s < N; ++s)
			{
				FVector2D Slot; bool bCenter = false;
				if (N == 5)
				{
					switch (s)
					{
						case 0: Slot = FVector2D( IX, -IY); break;
						case 1: Slot = FVector2D( IX,  IY); break;
						case 2: Slot = FVector2D( 0.f, 0.f);  bCenter = true; break;
						case 3: Slot = FVector2D(-IX, -IY); break;
						default:Slot = FVector2D(-IX,  IY); break;
					}
				}
				else
				{
					Slot = FVector2D(0.f, (s - (N - 1) * 0.5f) * IY * 1.6f);
					bCenter = (s == N / 2);
				}
				const FVector Loc = Origin + FVector(BlockX + Slot.X, BlockY + Slot.Y, GroundZ);
				if (AWOTOLDemoUnit* U = SpawnUnit(Id, Loc, Facing, 1.f, PScale))
				{
					U->SetFormation(Gid, Slot, bCenter);
					if (LayerZ > 0.f) U->SetDesiredZ(LayerZ); // tireurs/spéciales : couche supérieure
				}
			}
		}
		Cursor += BandsUsed * BSX + GroupGap;
	};

	// Blocs plus LARGES avant de passer en profondeur -> on étale sur la largeur de la zone.
	const int32 BPBinf = bGrandBattle ? 8 : 5;
	const int32 BPBmon = bGrandBattle ? 5 : 4;
	const int32 BPBdis = bGrandBattle ? 7 : 5;
	const int32 BPBspe = bGrandBattle ? 5 : 3;

	// Deux CURSEURS de profondeur INDÉPENDANTS : la mêlée au SOL et les tireurs EN HAUTEUR
	// partent tous les deux du front et se superposent en XY (couches différentes) -> profondeur
	// au sol réduite de moitié (fini l'armée qui s'étire sur 5 rangs en profondeur).
	float GroundCursor = 0.f, AirCursor = 0.f;
	const float AirLayerZ = 700.f; // 1re couche au-dessus du sol

	// Chef en pointe (devant l'infanterie, centré) — son propre "groupe" solo.
	if (AWOTOLDemoUnit* Chef = SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Chef),
			Origin + FVector(Depth, 0.f, GroundZ), Facing, 1.f, PScale))
	{
		Chef->SetFormation(NextFormationGroupId++, FVector2D::ZeroVector, true);
	}

	// MÊLÉE au SOL (infanterie devant, montures derrière). Effectifs = base + renforts cité.
	PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie), EffInfantry, BPBinf, GroundCursor, 0.f);
	PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Montee), EffMounted, BPBmon, GroundCursor, 0.f);
	// TIREURS en HAUTEUR (couche 1), superposés à la mêlée -> ils tirent par-dessus.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Distance), EffRanged, BPBdis, AirCursor, AirLayerZ);
	}
	// PHASE 3 : SPÉCIALE (arrière-ligne) + MYTHIQUE (soutien) débloquées.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Speciale))
	{
		// Spéciale aussi EN HAUTEUR (couche 1), derrière les tireurs (même curseur aérien).
		PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Speciale), EffSpecial, BPBspe, AirCursor, AirLayerZ);
	}
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Mythique))
	{
		// BUGFIX (mythique « sous la map ») : le recul BackCursor pouvait le placer DERRIÈRE le
		// bord du tiers -> clampé sur un point hors sol / invisible. On le place désormais à un
		// endroit GARANTI dans le champ : juste derrière le CENTRE de l'armée, faible recul.
		const float MythBack = Depth * 2.0f;
		if (AWOTOLDemoUnit* Myth = SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Mythique),
				Origin + FVector(-MythBack, 0.f, GroundZ), Facing, /*ScaleBoost=*/1.0f,
				/*HealthScale=*/bGrandBattle ? PScale * 1.15f : 3.0f * FactionSurvivability(Faction)))
		{
			// Il NAGE AU-DESSUS de l'armée (couche haute) -> visible, sélectionnable SEUL (clic sur
			// son modèle en hauteur), sans gêner/être gêné par les unités au sol. Son IA de soutien
			// le recentrera au-dessus du gros de l'armée.
			Myth->SetDesiredZ(900.f);
			Myth->SetFormation(NextFormationGroupId++, FVector2D::ZeroVector, true);
		}
	}
}

void AWOTOLDemoDirector::SpawnEnemyForCreature(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const FName CreatureID = Demo
		? Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Mythique)
		: NAME_None;
	// PHASE 1 : PV du Kraken × k(difficulté) -> même levier que les autres phases (Facile =
	// Kraken bien plus fragile ; Difficile = à peine réduit). Sa frappe est modulée par
	// DifficultyEnemyDamageMult() côté unité. La phase 1 reste gagnable aux 3 niveaux.
	const float KrakenHP = CreatureHealthScale * (Demo ? EnemyDiffKHP(Demo->GetDifficulty()) : 0.79f);
	if (AWOTOLDemoUnit* Creature = SpawnUnit(
			CreatureID, Origin + FVector(0.f, 0.f, 80.f), Facing, 1.5f, KrakenHP, /*bAsBoss=*/true))
	{
		// bCreatureBrain reste FAUX pendant la préparation : le boss attend.
		// Il est activé par StartBattleNow() au lancement de la bataille.
		Creature->bIsBoss = true; // étiquette "Kraken" dès la préparation
		// KRAKEN CORIACE : c'est un pilier de la démo, il doit tenir bien plus longtemps.
		// On renforce défense + parade de son asset de données (seul le Kraken l'utilise
		// dans la démo — aucun Noxedrake allié n'est déployé).
		if (UUnitDataAsset* Data = Creature->GetUnitData())
		{
			// ÉQUILIBRAGE : le Kraken doit rester un défi mais la phase 1 doit être
			// GAGNABLE avec le petit groupe du joueur (les deux factions). On baisse donc
			// nettement sa robustesse et sa frappe (valeurs ABSOLUES = idempotentes).
			// Défi mesuré : le Kraken doit tenir un peu mais rester BATTABLE (valeurs ABSOLUES).
			Data->Stats.DefensePercent = FMath::Clamp(Data->Stats.DefensePercent, 16.f, 20.f);
			Data->Stats.BlockChance    = FMath::Clamp(Data->Stats.BlockChance, 8.f, 12.f);
			Data->Stats.DodgeChance    = FMath::Min(Data->Stats.DodgeChance, 2.f);
			Data->Stats.AttackDPS      = FMath::Clamp(Data->Stats.AttackDPS, 230.f, 280.f);
		}
		if (Demo)
		{
			Demo->SetBoss(Creature);
		}
	}
}

void AWOTOLDemoDirector::SpawnRivalSquad(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	// L'IA (Lia) déploie une armée ÉQUIVALENTE à celle du joueur, RÉPARTIE SUR 3
	// COUCHES : mêlée en bas, chef/montée au milieu, distance en haut (tire à travers).
	const float L0 = 200.f, L1 = 900.f, L2 = 1600.f;
	const float Lat = UnitSpacing;
	const float Depth = UnitSpacing * (bGrandBattle ? 1.15f : 1.4f);
	auto SetLayer = [](AWOTOLDemoUnit* U, float Z) { if (U) U->SetDesiredZ(Z); };

	// SEULE CONTRAINTE : l'ennemi est LIMITÉ À SON TIERS (comme le joueur au sien). On
	// borne donc chaque unité pour qu'elle ne franchisse pas la limite MIROIR (côté ennemi
	// = Center - PlacementBoundaryOffsetX). À l'intérieur, il place ses unités LIBREMENT.
	const float MirrorX = GetActorLocation().X - PlacementBoundaryOffsetX; // limite du tiers ennemi
	const FVector O = Origin;

	// ── ÉQUILIBRAGE : l'armée du joueur écrasait la rivale 35-0 sans une seule perte, et
	// trop vite (~2 min). On DURCIT l'armée RIVALE (PV majorés côté IA UNIQUEMENT) : sa ligne
	// de front tient plus longtemps, pousse jusqu'aux lignes arrière du joueur -> le joueur
	// SUBIT enfin des pertes et le combat DURE davantage, tout en restant GAGNABLE.
	// × compensation d'identité de la faction RIVALE (Noxéens fragiles compensés) : la règle
	// d'équilibrage s'applique aux DEUX camps, quel que soit celui contrôlé par le joueur.
	// PAS de sur-bonus rival : empile a 1.6/1.35 x compensation de faction, le Noxebeast (tank,
	// base 1700) montait a ~5800 PV. On retire le bonus phase 2 (leger en phase 3) -> l'armee
	// rivale reste tuable et fidele a son identite. La DIFFICULTE est le vrai curseur de defi.
	// Phase 3 : PV rival ABAISSES (0.90) -> le joueur ne perd plus la grande bataille malgre
	// des degats superieurs (l'ennemi etait plus tanky que lui).
	// PV de l'ENNEMI = même baseline que le joueur (échelle × faction × ÉVOLUTION DE PHASE),
	// puis × k(difficulté) : c'est le SEUL levier de difficulté (ennemi affaibli en Facile,
	// presque à parité en Difficile). Ratio PV joueur/ennemi = 1/k, cumulé au ratio dégâts ->
	// force de combat globale = 1/k² = R garanti > 1 (gagnable).
	const float RivalScale = ArmyHealthScale * FactionSurvivability(RivalFaction)
		* PhaseEvoHP(Demo->GetPhase())
		* GrandAquilorisCoordinationHP(RivalFaction, Demo->GetPhase())
		* EnemyDiffKHP(Demo->GetDifficulty());

	// PLACEMENT ALÉATOIRE par couche, PROPRE À LA FACTION : chaque unité peut être au sol
	// ou en hauteur. Les caps de verticalité (ex. Noxebeast au grade 1) sont respectés
	// automatiquement par SetDesiredZ (clamp sur MaxLayerZ). On choisit une couche au
	// hasard parmi celles qui conviennent au rôle.
	auto PickLayer = [&](EDemoUnitCategory Cat) -> float
	{
		const float r = FMath::FRand();
		switch (Cat)
		{
			case EDemoUnitCategory::Distance:  return (r < 0.6f) ? L2 : L1;           // tireurs plutôt haut
			case EDemoUnitCategory::Montee:    return (r < 0.5f) ? L0 : L1;           // montées sol/1re couche
			case EDemoUnitCategory::Chef:
			case EDemoUnitCategory::Mythique:  return (r < 0.5f) ? L1 : L2;           // commandement en hauteur
			default:                           return (r < 0.7f) ? L0 : L1;           // mêlée surtout au sol
		}
	};

	// Curseur de PROFONDEUR cumulatif (comme côté joueur) : chaque catégorie a SES rangées,
	// jamais deux types sur la même ligne. Côté rival, l'arrière = +X (il fait face à -X).
	float BackCursor = 0.f;
	const float GroupGap = Depth * (bGrandBattle ? 0.5f : 1.0f);
	// Décalages de slot (2-1-2) pour la COHÉSION + l'étiquette de groupe côté rival.
	const float RIX = Depth * 0.5f, RIY = Lat * 0.55f;
	auto PlaceRows = [&](FName Id, EDemoUnitCategory Cat, int32 Count, int32 PerRow)
	{
		if (Id.IsNone() || Count <= 0 || PerRow <= 0) return;
		const int32 Rows = (Count + PerRow - 1) / PerRow;
		int32 CurGid = -1;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow;
			const int32 Col = i % PerRow;
			const int32 InThisRow = FMath::Min(PerRow, Count - Row * PerRow);
			const float Y = (Col - (InThisRow - 1) * 0.5f) * Lat;
			FVector Loc = O + FVector(BackCursor + Row * Depth, Y, 100.f);
			Loc.X = FMath::Max(Loc.X, MirrorX); // ne pas franchir la limite de son tiers
			AWOTOLDemoUnit* U = SpawnUnit(Id, Loc, Facing, 1.f, RivalScale);
			SetLayer(U, PickLayer(Cat));
			// GROUPE DE 5 (comme le joueur) : étiquette groupée + unité CENTRALE porteuse -> UN
			// marqueur par groupe côté ENNEMI aussi (fini un indicateur par unité verte).
			const int32 ChunkStart = (i / 5) * 5;
			const int32 ChunkSize = FMath::Min(5, Count - ChunkStart);
			const int32 Pos = i - ChunkStart;
			if (Pos == 0) CurGid = NextFormationGroupId++;
			FVector2D Slot; const bool bCenter = (Pos == ChunkSize / 2);
			switch (Pos)
			{
				case 0: Slot = FVector2D( RIX, -RIY); break;
				case 1: Slot = FVector2D( RIX,  RIY); break;
				case 2: Slot = FVector2D( 0.f,  0.f); break;
				case 3: Slot = FVector2D(-RIX, -RIY); break;
				default:Slot = FVector2D(-RIX,  RIY); break;
			}
			if (U) U->SetFormation(CurGid, Slot, bCenter);
		}
		BackCursor += Rows * Depth + GroupGap;
	};
	const int32 PRinf = bGrandBattle ? 18 : 8;
	const int32 PRmon = bGrandBattle ? 12 : 6;
	const int32 PRdis = bGrandBattle ? 16 : 8;
	const int32 PRspe = bGrandBattle ? 6  : 3;
	int32 RivalInfantry = InfantryCount;
	int32 RivalMounted = MountedCount;
	int32 RivalRanged = RangedCount;
	int32 RivalSpecial = SpecialCount;
	if (bGrandBattle)
	{
		if (RivalFaction == EFactionID::Noxeens)
		{
			RivalInfantry = 34; RivalMounted = 22; RivalRanged = 30; RivalSpecial = 12;
		}
		else
		{
			RivalInfantry = 20; RivalMounted = 12; RivalRanged = 18; RivalSpecial = 8;
		}

		// REDUCTION EN FACILE (01/08/2026, retour terrain : "il faut faire en sorte que le
		// joueur puisse vaincre l'ennemi" -- une vraie partie en Facile a atteint le chrono de
		// 15 min avec 46/60 ennemis encore vivants). L'effectif ci-dessus ne variait jusqu'ici
		// QUE par le HP/DMG de l'ennemi (EnemyDiffKHP/DMG), jamais par son NOMBRE -> en Facile,
		// en plus d'etre plus fragile, l'armee rivale est maintenant aussi plus PETITE, pour
		// qu'une annihilation complete reste vraiment atteignable dans le temps imparti. Normal/
		// Difficile INCHANGES (aucune donnee de partie ne les signale comme problematiques).
		if (Demo->GetDifficulty() == EDemoDifficulty::Facile)
		{
			RivalInfantry = FMath::RoundToInt(RivalInfantry * 0.65f);
			RivalMounted  = FMath::RoundToInt(RivalMounted  * 0.65f);
			RivalRanged   = FMath::RoundToInt(RivalRanged   * 0.65f);
			RivalSpecial  = FMath::RoundToInt(RivalSpecial  * 0.65f);
		}
	}

	FVector ChefLoc = O + FVector(-Depth, 0.f, 100.f);
	ChefLoc.X = FMath::Max(ChefLoc.X, MirrorX);
	SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef), ChefLoc, Facing, 1.f, RivalScale),
		PickLayer(EDemoUnitCategory::Chef));

	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Infanterie), EDemoUnitCategory::Infanterie, RivalInfantry, PRinf);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Montee), EDemoUnitCategory::Montee, RivalMounted, PRmon);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Distance), EDemoUnitCategory::Distance, RivalRanged, PRdis);
	// PHASE 3 : la rivale déploie AUSSI sa spéciale + son mythique.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Speciale))
	{
		PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Speciale), EDemoUnitCategory::Speciale, RivalSpecial, PRspe);
	}
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Mythique))
	{
		FVector MLoc = O + FVector(BackCursor, 0.f, 100.f);
		MLoc.X = FMath::Max(MLoc.X, MirrorX);
		SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Mythique), MLoc, Facing, /*ScaleBoost=*/1.0f, RivalScale),
			PickLayer(EDemoUnitCategory::Mythique));
	}
}

int32 AWOTOLDemoDirector::RollUnitGrade(int32 CenterLevel) const
{
	// Tirage biaise AUTOUR du centre (60% le grade central, 25% un cran en-dessous si
	// possible, 15% un cran au-dessus si possible) -> variete individuelle sans deplacer la
	// moyenne du groupe.
	const int32 MaxLvl = UDemoFlowSubsystem::MaxBuildingLevel;
	const float R = EncounterRandom.FRand();
	int32 Lvl = CenterLevel;
	if (R < 0.25f && CenterLevel > 1) Lvl = CenterLevel - 1;
	else if (R > 0.85f && CenterLevel < MaxLvl) Lvl = CenterLevel + 1;
	return Lvl;
}

AWOTOLDemoUnit* AWOTOLDemoDirector::SpawnUnit(FName UnitID, const FVector& Loc, const FRotator& Facing,
	float ScaleBoost, float HealthScale, bool bAsBoss)
{
	if (!DemoUnitClass || UnitID.IsNone()) return nullptr;

	UGameInstance* GI = GetGameInstance();
	UUnitDataRegistrySubsystem* Registry = GI ? GI->GetSubsystem<UUnitDataRegistrySubsystem>() : nullptr;
	if (!Registry) return nullptr;

	UUnitDataAsset* Data = Registry->GetUnitData(UnitID);
	if (!Data) return nullptr;

	// SÉCURITÉ : ne JAMAIS générer une unité hors de l'arène (dans les montagnes/le mur), où
	// elle resterait bloquée. On ramène toute position au-delà du rayon sûr sur le cercle.
	FVector SafeLoc = Loc;
	{
		const FVector Ctr = GetActorLocation();
		FVector Flat = SafeLoc - Ctr; Flat.Z = 0.f;
		const float MaxR = bGrandBattle ? 8200.f : 4200.f;
		if (Flat.Size() > MaxR) SafeLoc = Ctr + Flat.GetSafeNormal() * MaxR + FVector(0.f, 0.f, SafeLoc.Z - Ctr.Z);
	}

	const FTransform SpawnTM(Facing, SafeLoc, FVector(ScaleBoost));

	// ── PROGRESSION (niveau de bâtiment / grade) : PRISE EN COMPTE dans les stats (décision
	// Liamor). Le niveau du bâtiment de la catégorie multiplie PV + dégâts des unités du JOUEUR
	// (Niv 1 = ×1.0, Niv 2 = ×1.15, Niv 3 = ×1.30). Voir Docs/SYSTEME_CITE_ET_DEFENSE.md.
	// GRADE INDIVIDUEL (demande de Liamor du 26/07/2026) : le niveau de bâtiment sert de CENTRE,
	// pas de valeur unique imposée à tout le groupe -> chaque unité tire son propre grade autour
	// de ce centre (RollUnitGrade), côté JOUEUR et côté RIVAL (qui n'a pas de bâtiment à
	// améliorer dans cette démo, donc centré sur le grade de base 1). Un même type d'unité n'est
	// plus détruit d'un seul coup en bloc : certaines sont encore au stade de base, d'autres déjà
	// améliorées, des deux côtés du champ de bataille. ──
	int32 GradeLvl = 1;
	if (!bAsBoss)
	{
		// Chef et Mythique sont des unités SOLO (MaxCountInSquad=1, un seul exemplaire en jeu) :
		// la variété de grade n'a aucun sens pour un exemplaire unique (rien à diversifier) et
		// ajouterait un bruit non voulu à l'équilibrage soigneusement calibré (ratios de force
		// documentés plus haut) -> pas de tirage pour elles, seulement pour les catégories en
		// escouade (Infanterie/Montée/Distance/Spéciale).
		const EDemoUnitCategory Cat = UDemoFlowSubsystem::GetCategoryForUnit(UnitID);
		const bool bSoloUnit = (Cat == EDemoUnitCategory::Chef || Cat == EDemoUnitCategory::Mythique);
		if (Data->Faction == CachedPlayerFaction)
		{
			if (UDemoFlowSubsystem* Flow = GI->GetSubsystem<UDemoFlowSubsystem>())
			{
				const int32 Lvl = Flow->GetBuildingLevel(Cat);
				GradeLvl = bSoloUnit ? Lvl : RollUnitGrade(Lvl);
			}
		}
		else
		{
			GradeLvl = bSoloUnit ? 1 : RollUnitGrade(1);
		}
	}
	const float ProgFactor = GradeLevelToFactor(GradeLvl);

	AWOTOLDemoUnit* Unit = GetWorld()->SpawnActorDeferred<AWOTOLDemoUnit>(
		DemoUnitClass, SpawnTM, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Unit) return nullptr;

	Unit->UnitData    = Data;
	Unit->HealthScale = HealthScale * ProgFactor;   // niveau/grade -> PV (appliqué dans BeginPlay)
	Unit->GradeLevel  = GradeLvl;    // sépare la barre de commandement en groupes par grade
	Unit->bIsBoss     = bAsBoss;       // AVANT FinishSpawning -> silhouette Kraken forcée
	Unit->TacticalPersonality = EncounterRandom.FRandRange(0.f, 1.f);
	UGameplayStatics::FinishSpawningActor(Unit, SpawnTM);

	// ── ÉQUILIBRAGE DES DÉGÂTS (difficulté + camp) : fixé au spawn, stable. Le boss (Kraken)
	// a sa propre gestion de difficulté -> exclu ici. ──
	if (!bAsBoss)
	{
		UDemoFlowSubsystem* Flow = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
		const EDemoDifficulty Diff = Flow ? Flow->GetDifficulty() : EDemoDifficulty::Normal;
		const EDemoPhase Phase     = Flow ? Flow->GetPhase() : EDemoPhase::Battle_Creature;
		const bool bPlayerSide = (Unit->GetFaction() == CachedPlayerFaction);
		// Dégâts = égalisation de faction × ÉVOLUTION DE PHASE (identique aux deux camps), et
		// côté ENNEMI × k(difficulté). Le joueur ne dépend PAS de la difficulté (baseline
		// stable) -> tout le curseur de défi est sur l'ennemi. Ratio dégâts joueur/ennemi = 1/k.
		float M = FactionDamage(Unit->GetFaction()) * PhaseEvoDMG(Phase)
			* GrandAquilorisCoordinationDMG(Unit->GetFaction(), Phase);
		if (!bPlayerSide) M *= EnemyDiffKDMG(Diff); // degats ennemis peu reduits -> pertes garanties
		M *= ProgFactor; // niveau/grade de bâtiment -> dégâts (progression prise en compte)
		Unit->BalanceDamageMult = M;
	}

	SpawnedUnits.Add(Unit);
	return Unit;
}

void AWOTOLDemoDirector::LaunchBattle()
{
	// (Musique geree par l'ecran : BattleMusic continue de Prepare a Playing, sans coupure.)
	UDemoFlowSubsystem* BattleFlow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const EDemoPhase BattlePhase = BattleFlow ? BattleFlow->GetPhase() : EDemoPhase::None;
	const bool bTerritoryDefense = BattlePhase == EDemoPhase::Battle_Rival;
	const bool bCreatureBattle = BattlePhase == EDemoPhase::Battle_Creature;
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		// Phase 3 (grande bataille) : chrono ÉTENDU à 18 min (1080 s, relevé de 900 -- retour
		// terrain 01/08/2026 : "il faut faire en sorte que le joueur puisse vaincre l'ennemi" --
		// une vraie partie a atteint le chrono avec seulement 14/60 ennemis tués, 15 min ne
		// laissait pas assez de marge pour une annihilation complète même en jouant bien) ;
		// sinon 10 min.
		const bool bGrand = GetGameInstance() && GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()
			&& GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()->GetPhase() == EDemoPhase::Battle_Grand;
		RTS->StartBattlePhase(bGrand ? 1080.f : 600.f);
		// PERF phase 3 : dizaines de pouvoirs simultanés -> on coupe les LAMPES dynamiques des
		// VFX (rayons/projectiles). L'émissif + le bloom restent : le spectacle est intact, le
		// GPU respire. Les phases 1/2 (peu d'unités) gardent les lampes.
		WOTOLGlow::bLowGpuVFX = bGrand;
	}

	// TES unités attaquent D'OFFICE l'ennemi le plus proche tant que tu ne leur donnes
	// pas d'ordre (clic droit = déplacer/attaquer, qui prend le dessus). Comportement
	// symétrique avec l'IA : elles cherchent et engagent jusqu'à ce qu'il n'y ait plus
	// personne (portée de vue immense = elles voient tout le champ de bataille).
	if (UWorld* W = GetWorld())
	{
		const FVector EnemyCenter = GetActorLocation() + FVector(ArmySeparation * 0.5f, 0.f, 0.f);
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction(CachedPlayerFaction))
			{
				if (!U) continue;
				if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(U->GetController()))
				{
					AIC->ActivateRTSBehavior();
					AIC->IssueOrder_AttackMove(EnemyCenter); // cherche + attaque en avançant
				}
				if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
				{
					St->SightRange = 60000.f;
					St->bAllowRetreat = false; // unités du joueur : ne fuient jamais
				}
				// AVANTAGE DE ZONE (phase 2) : le joueur a POSÉ le Cristalliseur et capturé
				// la zone en phase 1 -> ses unités défendent un terrain qui leur appartient
				// (cristaux de terraformation). Les Noxéens surclassaient trop la défense
				// (35/35 pertes vs 8) : on donne un avantage FORT sur les deux fronts —
				// encaisser beaucoup moins ET frapper plus fort. [Réglable]
				// Calibrage : 0.7/1.0 -> défaite (35-8), 0.5/1.4 -> stomp (1-35). On vise le
				// MILIEU pour un vrai combat disputé (~50/50, pertes des deux côtés).
				if (bTerritoryDefense && CaptureObject != nullptr)
				{
					// AVANTAGE DE ZONE dépendant de la FACTION du joueur : les Noxéens sont
					// déjà plus forts (DPS + bonus bioluminescent) -> avec un avantage FORT ils
					// gagnaient sans AUCUNE perte. On leur donne un avantage MODÉRÉ pour que les
					// Aquiloris leur causent quand même quelques pertes. Les Aquiloris (plus
					// fragiles en attaque adverse) gardent l'avantage FORT dont ils ont besoin.
					if (CachedPlayerFaction == EFactionID::Noxeens)
					{
						U->IncomingDamageMult = 0.82f; // -18% seulement -> l'ennemi fait des pertes
						U->OutgoingDamageMult = 1.08f;
					}
					else
					{
						// -22% (au lieu de -40%) : les Aquiloris tiennent TOUJOURS la défense
						// (ils gagnent), mais encaissent assez pour subir de VRAIES pertes
						// (avant : -40% + combat quasi ability-only = 0 perte). [Réglable]
						U->IncomingDamageMult = 0.78f;
						U->OutgoingDamageMult = 1.2f;
					}
				}
			}

			// ── ÉQUILIBRAGE AUTOMATIQUE PAR FACTION (phase 1, Kraken) ──
			// Les Noxéens (fragiles) perdaient toujours, les Aquiloris (résistants)
			// gagnaient : on CALIBRE les PV du Kraken sur la puissance RÉELLE de l'armée
			// du joueur (PV totaux + un peu de sa capacité de survie) pour viser ~50/50
			// quel que soit le camp. Vaut pour les deux factions, sans rien coder en dur.
			if (bCreatureBattle) // uniquement la bataille de créature
			{
				if (AWOTOLDemoUnit* Boss = Cast<AWOTOLDemoUnit>(
						GetGameInstance() && GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()
						? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()->GetBoss() : nullptr))
				{
					// PV du Kraken = fraction DÉTERMINISTE des PV totaux de l'armée du joueur.
					// La composition d'armée est FIXE -> valeur CONSTANTE à chaque partie.
					// 0.42 redonne ~22500 PV pour les Aquiloris (le bon ressenti : victoire,
					// ~6 pertes). [Réglable : 0.35 plus facile .. 0.50 plus dur]
					float ArmyHP = 0.f;
					for (AUnitBase* U : Reg->GetUnitsForFaction(CachedPlayerFaction))
					{
						if (!U || !U->IsAlive() || !U->GetUnitData()) continue;
						if (AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U)) ArmyHP += DU->GetEffectiveMaxHealth();
						else                                              ArmyHP += U->GetUnitData()->Stats.MaxHealth;
					}
					if (ArmyHP > 0.f && Boss->GetUnitData())
					{
						// PHASE 1 : combat jugé trop court -> on ALLONGE en donnant plus de PV au
						// Kraken (0.42 -> 0.58). Avec la défense/parade relevées ci-dessus, il
						// tient nettement plus longtemps et fait quelques pertes de plus, tout en
						// restant BATTABLE par le groupe du joueur. [Réglable : 0.50 court .. 0.65 dur]
						// × difficulté : Facile amincit le Kraken, Difficile l'épaissit (compense
						// aussi l'inflation de PV joueur en Facile pour que ce soit vraiment plus simple).
						const float DMul = GetGameInstance() && GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()
							? EnemyDiffKHP(GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()->GetDifficulty()) : 0.79f;
						const float TargetHP = FMath::Clamp(ArmyHP * 0.42f * DMul, 9000.f, 40000.f);
						const int32 BaseMax  = FMath::Max(1, Boss->GetUnitData()->Stats.MaxHealth);
						Boss->HealthScale    = FMath::Max(1.f, TargetHP / (float)BaseMax);
						Boss->SetHealthToFull(); // applique PV = HealthScale * base
					}
				}
			}
		}
	}

	// IA ENNEMIE : armée rivale STRUCTURÉE et OFFENSIVE — elle avance droit sur
	// l'armée du joueur et engage (plus d'errance/patrouille passive au spawn).
	if (UWorld* W = GetWorld())
	{
		const FVector PlayerCenter = GetActorLocation() + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
		// En phase 2, ~40% des rivaux FONCENT sur le bâtiment (siège), le reste engage
		// l'armée du joueur -> il faut à la fois défendre le bâtiment ET tenir la ligne.
		const bool bSiege = bTerritoryDefense && CaptureObject != nullptr;
		const FVector BuildingLoc = bSiege ? CaptureObject->GetActorLocation() : PlayerCenter;
		int32 RivalIndex = 0;
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction(CachedRivalFaction))
			{
				if (!U) continue;
				AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U);
				if (DU && DU->bCreatureBrain) continue; // le boss a son propre cerveau
				// Léger avantage de survie/mordant à l'IA rivale (phases 1 & 2) : elle encaisse
				// ~10% de moins et frappe ~12% de plus -> elle fait des pertes au joueur sans
				// renverser l'issue. EN PHASE 3 : ZONE NEUTRE -> AUCUN avantage artificiel, les
				// deux camps sont à égalité (seuls leurs bonus de faction comptent).
				if (!bGrandBattle)
				{
					U->IncomingDamageMult *= 0.90f;
					U->OutgoingDamageMult *= 1.12f;
				}
				const bool bSieger = bSiege && (RivalIndex++ % 5 < 2); // ~40% assiégeurs
				if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(U->GetController()))
				{
					AIC->ActivateRTSBehavior();
					AIC->IssueOrder_AttackMove(bSieger ? BuildingLoc : PlayerCenter);
				}
				// CHASSE PERSISTANTE : portée de vue immense -> l'ennemi voit et poursuit
				// TOUTE unité du joueur sur la carte (il ne s'arrête jamais tant qu'il
				// reste des cibles), et ne se disperse plus une fois les locaux abattus.
				if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
				{
					St->SightRange = 60000.f;
				}
			}
		}
	}

	// Les créatures/boss sont pilotées par leur propre cerveau (Tick) : on coupe
	// leur IA RTS standard pour éviter tout conflit de mouvement.
	for (AWOTOLDemoUnit* U : SpawnedUnits)
	{
		if (U && U->bCreatureBrain)
		{
			if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(U->GetController()))
			{
				AIC->DeactivateRTSBehavior();
			}
		}
	}

	FocusCameraOnPlayer(); // recadrage au lancement du combat

	// Surveille la fin de bataille (un camp anéanti) toutes les 2 s
	GetWorldTimerManager().SetTimer(
		BattleCheckHandle, this, &AWOTOLDemoDirector::CheckBattleEnd, 2.f, true);

	// Siège du bâtiment (phase 2) : dégâts en continu selon les assiégeants proches.
	if (bTerritoryDefense && CaptureObject)
	{
		// ── ÉQUILIBRAGE ASYMÉTRIQUE de l'objectif selon l'ATTAQUANT ──
		// Les Noxéens attaquants (gros DPS) écrasaient la défense Aquiloris et détruisaient
		// le Cristalliseur. Les Aquiloris attaquants laissaient le Noxéen défenseur gagner
		// (cas "parfait" -> on n'y touche PAS). On RENFORCE donc l'objectif UNIQUEMENT quand
		// l'attaquant est Noxéen, pour que la défense Aquiloris soit tenable jusqu'au chrono.
		const float ObjHP = (CachedRivalFaction == EFactionID::Noxeens) ? 26000.f : 16000.f;
		const bool bRetryingThreat = BattleFlow && BattleFlow->GetProgress().bZoneThreatened;
		const float RestoredPct = bRetryingThreat
			? BattleFlow->GetTerritoryHealthPercent() : 1.f;
		CaptureObject->MaxHealth = ObjHP;
		CaptureObject->CurrentHealth = FMath::Clamp(ObjHP * RestoredPct, 1.f, ObjHP);
		if (BattleFlow)
		{
			BattleFlow->SnapshotTerritoryBuilding(
				CaptureObject->CurrentHealth, CaptureObject->MaxHealth);
		}
		GetWorldTimerManager().SetTimer(
			SiegeHandle, this, &AWOTOLDemoDirector::SiegeTick, 1.f, true);
	}

	// Cerveau tactique : ré-évalue les manœuvres des 2 armées toutes les 3,5 s.
	GetWorldTimerManager().SetTimer(
		TacticalHandle, this, &AWOTOLDemoDirector::TacticalTick, 2.0f, true, 2.0f);

	BattleStartTime = GetWorld()->GetTimeSeconds(); // pour la durée du résumé
	InitializeAdaptiveBattleBalance();
}

void AWOTOLDemoDirector::ConfigureAdaptiveCasualtyTargets(EDemoPhase Phase,
	EDemoDifficulty Difficulty, int32 ActualPlayerCount)
{
	AdaptiveTargetLossMin = AdaptiveTargetLossPreferred = AdaptiveTargetLossMax = 0;
	if (ActualPlayerCount <= 1) return;

	// Conversion proportionnelle depuis les effectifs explicitement validés (16 / 35).
	// Elle garde les mêmes pourcentages si une composition, une sauvegarde ou un futur réglage
	// modifie l'effectif réel ; aucune hypothèse fixe n'est injectée dans le combat.
	auto Scale = [ActualPlayerCount](float ReferenceLosses, float ReferenceArmy) -> int32
	{
		return FMath::Clamp(FMath::RoundToInt(
			ActualPlayerCount * ReferenceLosses / ReferenceArmy), 0, ActualPlayerCount - 1);
	};
	auto SetRange = [&](float ReferenceMin, float ReferenceMax, float ReferenceArmy)
	{
		AdaptiveTargetLossMin = Scale(ReferenceMin, ReferenceArmy);
		AdaptiveTargetLossMax = Scale(ReferenceMax, ReferenceArmy);
		AdaptiveTargetLossMax = FMath::Max(AdaptiveTargetLossMax, AdaptiveTargetLossMin);
		// Le centre réel change à chaque tentative. Les chiffres de design restent les
		// bornes de crédibilité, pas une issue écrite d'avance.
		AdaptiveTargetLossPreferred = EncounterRandom.RandRange(
			AdaptiveTargetLossMin, AdaptiveTargetLossMax);
	};

	if (Phase == EDemoPhase::Battle_Creature)
	{
		switch (Difficulty)
		{
		case EDemoDifficulty::Facile:
			SetRange(2.f, 3.f, 16.f);
			break;
		case EDemoDifficulty::Difficile:
			// 16 engagés -> 6 survivants, donc 10 pertes.
			SetRange(8.f, 12.f, 16.f);
			break;
		default:
			// Conserve le garde-fou validé : en Normal, le Kraken ne tombe pas avant 5 pertes.
			SetRange(5.f, 7.f, 16.f);
			break;
		}
	}
	else if (Phase == EDemoPhase::Battle_Rival)
	{
		switch (Difficulty)
		{
		case EDemoDifficulty::Facile:
			SetRange(6.f, 10.f, 35.f);
			break;
		case EDemoDifficulty::Difficile:
			SetRange(15.f, 20.f, 35.f);
			break;
		default:
			SetRange(12.f, 18.f, 35.f);
			break;
		}
	}
	else if (Phase == EDemoPhase::Battle_Grand)
	{
		// Phase 3 : chaque quota validé est le centre d'une plage ±5.
		const bool bAquiloris = CachedPlayerFaction == EFactionID::Aquiloris;
		const float ReferenceArmy = bAquiloris ? 60.f : 100.f;
		float Center = 0.f;
		if (bAquiloris)
		{
			switch (Difficulty)
			{
			case EDemoDifficulty::Facile:    Center = 15.f; break;
			case EDemoDifficulty::Difficile: Center = 45.f; break;
			default:                         Center = 25.f; break;
			}
		}
		else
		{
			switch (Difficulty)
			{
			case EDemoDifficulty::Facile:    Center = 25.f; break;
			case EDemoDifficulty::Difficile: Center = 75.f; break;
			default:                         Center = 45.f; break;
			}
		}
		SetRange(FMath::Max(0.f, Center - 5.f), Center + 5.f, ReferenceArmy);
	}

	AdaptiveTargetLossMax = FMath::Max(AdaptiveTargetLossMax, AdaptiveTargetLossMin);
	AdaptiveTargetLossPreferred = FMath::Clamp(AdaptiveTargetLossPreferred,
		AdaptiveTargetLossMin, AdaptiveTargetLossMax);
}

void AWOTOLDemoDirector::InitializeAdaptiveBattleBalance()
{
	ResetAdaptiveBattleBalance();
	if (!bEnableAdaptiveCasualtyBalance || !GetWorld()) return;

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	AdaptiveBalancePhase = Demo->GetPhase();
	if (AdaptiveBalancePhase != EDemoPhase::Battle_Creature
		&& AdaptiveBalancePhase != EDemoPhase::Battle_Rival
		&& AdaptiveBalancePhase != EDemoPhase::Battle_Grand) return;

	AdaptiveInitialPlayerCount = CountAlive(CachedPlayerFaction);
	AdaptiveInitialEnemyCount = CountAlive(CachedRivalFaction);
	ConfigureAdaptiveCasualtyTargets(AdaptiveBalancePhase, Demo->GetDifficulty(),
		AdaptiveInitialPlayerCount);
	if (AdaptiveInitialPlayerCount <= 1 || AdaptiveTargetLossMax <= 0) return;

	// Abonnement immédiat aux morts : le plafond est posé dans la même frame, y compris au
	// milieu d'une attaque de zone, avant qu'elle ne puisse dépasser la plage maximale.
	for (TObjectPtr<AWOTOLDemoUnit>& Unit : SpawnedUnits)
	{
		if (!Unit) continue;
		Unit->AdaptiveOutgoingDamageMult = 1.f;
		Unit->AdaptiveIncomingDamageMult = 1.f;
		Unit->MinimumHealthFloor = 0.f;
		if (Unit->GetFaction() == CachedPlayerFaction)
			Unit->OnUnitDied.AddUniqueDynamic(this,
				&AWOTOLDemoDirector::HandleAdaptivePlayerUnitDied);
	}

	// Un seul ancrage de rencontre suffit : le Kraken en phase 1, le chef rival (ou l'unité
	// la plus robuste disponible) en phase 2. Il empêche une victoire prématurée sans rendre
	// toute l'armée ennemie artificiellement immortelle.
	if (AdaptiveBalancePhase == EDemoPhase::Battle_Creature)
	{
		AdaptiveEnemyAnchor = Cast<AUnitBase>(Demo->GetBoss());
	}
	else
	{
		AUnitBase* Fallback = nullptr;
		int32 BestMaxHealth = -1;
		for (TObjectPtr<AWOTOLDemoUnit>& Unit : SpawnedUnits)
		{
			if (!Unit || Unit->GetFaction() != CachedRivalFaction || !Unit->IsAlive()) continue;
			if (UDemoFlowSubsystem::GetCategoryForUnit(
				Unit->GetUnitData() ? Unit->GetUnitData()->GetFName() : NAME_None)
				== EDemoUnitCategory::Chef)
			{
				AdaptiveEnemyAnchor = Unit;
				break;
			}
			const int32 MaxHealth = Unit->GetEffectiveMaxHealth();
			if (MaxHealth > BestMaxHealth) { BestMaxHealth = MaxHealth; Fallback = Unit; }
		}
		if (!AdaptiveEnemyAnchor) AdaptiveEnemyAnchor = Fallback;
	}

	if (AWOTOLDemoUnit* Anchor = Cast<AWOTOLDemoUnit>(AdaptiveEnemyAnchor.Get()))
	{
		Anchor->MinimumHealthFloor = FMath::Max(1.f,
			Anchor->GetEffectiveMaxHealth() * 0.06f);
	}
	else if (AdaptiveEnemyAnchor)
	{
		AdaptiveEnemyAnchor->MinimumHealthFloor = 1.f;
	}

	AdaptiveBalanceStartTime = GetWorld()->GetTimeSeconds();
	bAdaptiveBalanceActive = true;
	UpdateAdaptiveBattleBalance();
	GetWorldTimerManager().SetTimer(AdaptiveBalanceHandle, this,
		&AWOTOLDemoDirector::UpdateAdaptiveBattleBalance, 0.75f, true, 0.75f);

	UE_LOG(LogTemp, Log, TEXT("[WOTOL Balance] Phase=%d Diff=%d Faction=%d Effectif=%d Cible=%d/%d/%d"),
		static_cast<int32>(AdaptiveBalancePhase), static_cast<int32>(Demo->GetDifficulty()),
		static_cast<int32>(CachedPlayerFaction), AdaptiveInitialPlayerCount,
		AdaptiveTargetLossMin, AdaptiveTargetLossPreferred, AdaptiveTargetLossMax);
}

void AWOTOLDemoDirector::UpdateAdaptiveBattleBalance()
{
	if (!bAdaptiveBalanceActive || bBattleConcluded || !GetWorld()) return;
	const int32 PlayerAlive = CountAlive(CachedPlayerFaction);
	const int32 EnemyAlive = CountAlive(CachedRivalFaction);
	const int32 Losses = FMath::Clamp(AdaptiveInitialPlayerCount - PlayerAlive,
		0, AdaptiveInitialPlayerCount);

	const float Elapsed = FMath::Max(0.f,
		GetWorld()->GetTimeSeconds() - AdaptiveBalanceStartTime);
	const float Pacing = AdaptiveBalancePhase == EDemoPhase::Battle_Creature
		? KrakenCasualtyPacingSeconds
		: (AdaptiveBalancePhase == EDemoPhase::Battle_Grand
			? GrandBattleCasualtyPacingSeconds : DefenseCasualtyPacingSeconds);

	// GARDE-FOU CONTRE LE BLOCAGE INFINI (retour terrain 31/07/2026) : Losses >= AdaptiveTargetLossMin
	// est la SEULE condition qui libère le plancher de vie de l'ancre (MinimumHealthFloor). Un
	// joueur qui domine (peu/pas de pertes) ne l'atteint jamais -> le dernier ennemi restait
	// increvable indéfiniment (constaté : 98 unités contre 1, 5+ minutes, l'ennemi ne meurt
	// jamais). Le "pas de victoire prématurée" reste respecté pendant la fenêtre de pacing
	// normale, mais passé un délai de grâce (+20%) le plancher se libère de toute façon.
	if (Losses >= AdaptiveTargetLossMin || Elapsed >= Pacing * 1.2f) ReleaseAdaptiveEnemyAnchor();
	if (Losses >= AdaptiveTargetLossMax) ProtectAdaptivePlayerSurvivors();

	const float ExpectedLosses = AdaptiveTargetLossPreferred
		* FMath::Clamp(Elapsed / FMath::Max(1.f, Pacing), 0.f, 1.f);
	const float Error = (ExpectedLosses - Losses)
		/ FMath::Max(1.f, static_cast<float>(AdaptiveTargetLossPreferred));

	// Boucle fermée proportionnelle : retard de pertes -> l'ennemi frappe plus fort et tient
	// mieux ; avance de pertes -> pression réduite et vulnérabilité accrue. Les bornes évitent
	// tout saut brutal et conservent l'influence des ordres, formations, axes et améliorations.
	AdaptivePlayerCommandIntensity = SamplePlayerCommandIntensity();
	// Respiration organique de la rencontre + réaction mesurée à l'implication du joueur.
	// Aucun jet ne choisit une victime : il change seulement le tempo collectif.
	const float Rhythm = 1.f
		+ FMath::Sin(Elapsed * 0.031f + TacticalPhaseOffset) * 0.06f
		+ FMath::Sin(Elapsed * 0.079f + TacticalVariant * 1.7f) * 0.035f;
	const float CommandResponse = FMath::Lerp(0.96f, 1.08f,
		AdaptivePlayerCommandIntensity);
	float Pressure = FMath::Clamp((1.f + Error * 1.8f) * Rhythm
		* AdaptiveEncounterVariance * CommandResponse, 0.35f, AdaptiveMaxEnemyPressure);
	float EnemyIncoming = FMath::Clamp(1.f - Error * 1.15f, 0.40f, 1.85f);

	const AWOTOLDemoUnit* AdaptiveBoss = Cast<AWOTOLDemoUnit>(AdaptiveEnemyAnchor.Get());
	const bool bEnemyNearDefeat = AdaptiveBalancePhase == EDemoPhase::Battle_Creature
		? (AdaptiveBoss && AdaptiveBoss->GetEffectiveHealthPercent() <= 0.18f)
		: (EnemyAlive <= FMath::Max(3, FMath::RoundToInt(AdaptiveInitialEnemyCount * 0.25f)));
	if (Losses < AdaptiveTargetLossMin && bEnemyNearDefeat)
	{
		Pressure = FMath::Max(Pressure, FMath::Min(AdaptiveMaxEnemyPressure, 2.0f));
		EnemyIncoming = FMath::Min(EnemyIncoming, 0.42f);
		// "Mode Frenesie" (retour terrain 31/07/2026, inspire des Enrage/Berserk des raids MMO
		// — ex. Deathbringer Saurfang qui entre en Frenzy sous 30% de vie) : annonce visible
		// UNE SEULE FOIS quand l'ancre passe sous le seuil, pour que le joueur COMPRENNE
		// pourquoi ses coups portent moins bien, au lieu de le percevoir comme un bug silencieux.
		if (!bAdaptiveFrenzyAnnounced && AdaptiveEnemyAnchor && GetWorld())
		{
			bAdaptiveFrenzyAnnounced = true;
			AWOTOLDamageNumber::SpawnText(GetWorld(),
				AdaptiveEnemyAnchor->GetActorLocation() + FVector(0.f, 0.f, 220.f),
				TEXT("MODE FRENESIE — CARAPACE DURCIE !"), FLinearColor(1.f, 0.35f, 0.15f, 1.f));
		}
	}
	if (Losses >= AdaptiveTargetLossPreferred)
	{
		Pressure = FMath::Min(Pressure, 0.38f);
		EnemyIncoming = FMath::Max(EnemyIncoming, 1.75f);
	}
	if (Losses >= AdaptiveTargetLossMax)
	{
		Pressure = 0.08f;
		EnemyIncoming = 3.0f;
	}

	AdaptiveEnemyPressure = Pressure;
	for (TObjectPtr<AWOTOLDemoUnit>& Unit : SpawnedUnits)
	{
		if (!Unit || Unit->GetFaction() != CachedRivalFaction) continue;
		Unit->AdaptiveOutgoingDamageMult = Pressure;
		Unit->AdaptiveIncomingDamageMult = EnemyIncoming;
	}
}

float AWOTOLDemoDirector::SamplePlayerCommandIntensity() const
{
	if (!GetWorld()) return 0.f;
	const float Now = GetWorld()->GetTimeSeconds();
	int32 Alive = 0;
	float Commanded = 0.f;
	for (const TObjectPtr<AWOTOLDemoUnit>& Unit : SpawnedUnits)
	{
		if (!Unit || !Unit->IsAlive() || Unit->GetFaction() != CachedPlayerFaction) continue;
		++Alive;
		const float Age = Now - Unit->LastPlayerOrderTime;
		if (Age < 18.f) Commanded += FMath::Clamp(1.f - Age / 18.f, 0.f, 1.f);
		if (const UUnitAIStateComponent* State =
			Unit->FindComponentByClass<UUnitAIStateComponent>())
		{
			if (State->bFollowingPlayerOrder) Commanded += 0.35f;
		}
	}
	return Alive > 0 ? FMath::Clamp(Commanded / static_cast<float>(Alive), 0.f, 1.f) : 0.f;
}

void AWOTOLDemoDirector::ReleaseAdaptiveEnemyAnchor()
{
	if (AdaptiveEnemyAnchor) AdaptiveEnemyAnchor->MinimumHealthFloor = 0.f;
}

void AWOTOLDemoDirector::ProtectAdaptivePlayerSurvivors()
{
	if (bAdaptiveSurvivorsProtected) return;
	bAdaptiveSurvivorsProtected = true;
	for (TObjectPtr<AWOTOLDemoUnit>& Unit : SpawnedUnits)
	{
		if (Unit && Unit->IsAlive() && Unit->GetFaction() == CachedPlayerFaction)
			Unit->MinimumHealthFloor = FMath::Max(Unit->MinimumHealthFloor, 1.f);
	}
}

void AWOTOLDemoDirector::HandleAdaptivePlayerUnitDied(AUnitBase* Unit)
{
	if (!bAdaptiveBalanceActive || !Unit || Unit->GetFaction() != CachedPlayerFaction) return;
	const int32 Losses = FMath::Clamp(
		AdaptiveInitialPlayerCount - CountAlive(CachedPlayerFaction), 0, AdaptiveInitialPlayerCount);
	if (Losses >= AdaptiveTargetLossMin) ReleaseAdaptiveEnemyAnchor();
	if (Losses >= AdaptiveTargetLossMax) ProtectAdaptivePlayerSurvivors();
	UpdateAdaptiveBattleBalance();
}

void AWOTOLDemoDirector::ResetAdaptiveBattleBalance()
{
	GetWorldTimerManager().ClearTimer(AdaptiveBalanceHandle);
	for (TObjectPtr<AWOTOLDemoUnit>& Unit : SpawnedUnits)
	{
		if (!Unit) continue;
		Unit->OnUnitDied.RemoveDynamic(this,
			&AWOTOLDemoDirector::HandleAdaptivePlayerUnitDied);
		Unit->AdaptiveOutgoingDamageMult = 1.f;
		Unit->AdaptiveIncomingDamageMult = 1.f;
		Unit->MinimumHealthFloor = 0.f;
	}
	AdaptiveEnemyAnchor = nullptr;
	AdaptiveEnemyPressure = 1.f;
	AdaptiveInitialPlayerCount = 0;
	AdaptiveInitialEnemyCount = 0;
	AdaptiveTargetLossMin = AdaptiveTargetLossPreferred = AdaptiveTargetLossMax = 0;
	AdaptiveBalancePhase = EDemoPhase::None;
	AdaptivePlayerCommandIntensity = 0.f;
	bAdaptiveBalanceActive = false;
	bAdaptiveSurvivorsProtected = false;
	bAdaptiveFrenzyAnnounced = false;
}

void AWOTOLDemoDirector::CheckBattleEnd()
{
	if (bBattleConcluded) return;

	const int32 PlayerAlive = CountAlive(CachedPlayerFaction);
	const int32 EnemyAlive  = CountAlive(CachedRivalFaction);
	UDemoFlowSubsystem* Flow = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	if (EnemyAlive <= 0 && PlayerAlive > 0)
	{
		bBattleConcluded = true;
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		OnPlayerVictory();
		return;
	}
	if (PlayerAlive <= 0)
	{
		bBattleConcluded = true;
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		OnPlayerDefeat();
		return;
	}

	// ── TEMPS ÉCOULÉ ── L'objectif a-t-il tenu ?
	// Phase 2 : si le Cristalliseur n'est pas détruit à la fin du chrono -> VICTOIRE
	// (objectif "défendre la zone" rempli). Phase 1 (tuer le Kraken) : temps écoulé
	// sans avoir anéanti l'ennemi = échec de l'objectif -> défaite.
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		if (RTS->GetTimeRemaining() <= 0.f)
		{
			bBattleConcluded = true;
			GetWorldTimerManager().ClearTimer(BattleCheckHandle);
			UDemoFlowSubsystem* D2 = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
			const bool bGrandPhase = D2 && D2->GetPhase() == EDemoPhase::Battle_Grand;
			if (bGrandPhase)
			{
				// Phase 3 (annihilation, zone neutre) : au chrono, l'armée la plus nombreuse l'emporte.
				if (PlayerAlive >= EnemyAlive) OnPlayerVictory();
				else                           OnPlayerDefeat();
			}
			else
			{
				const bool bObjectiveHeld = (CaptureObject != nullptr); // bâtiment encore debout
				if (bObjectiveHeld) OnPlayerVictory();
				else                OnPlayerDefeat();
			}
		}
	}
}

void AWOTOLDemoDirector::OnPlayerVictory()
{
	// (Musique geree par l'ecran : SummaryMusic des le passage a l'ecran Summary.)
	GetWorldTimerManager().ClearTimer(SiegeHandle);
	GetWorldTimerManager().ClearTimer(TacticalHandle);
	GetWorldTimerManager().ClearTimer(AdaptiveBalanceHandle);
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->EndBattle(CachedPlayerFaction, EBattleResult::Victory);
	}

	const EDemoPhase Phase = Demo ? Demo->GetPhase() : EDemoPhase::None;

	if (Phase == EDemoPhase::Battle_Creature)
	{
		// Les récompenses sont attribuées au moment du rapport de victoire, pas plus tard dans
		// la cité. Elles restent affichables sur le résumé et disponibles pour le Cristalliseur.
		if (Demo)
		{
			Demo->GrantMissionRewards(CreatureRewardCrystals,
				CreatureRewardAbyssalMaterials, CreatureRewardBiomass, CreatureRewardOceanicEnergy);
			Demo->GrantProgressionXP(40, 30);
			Demo->SummaryContinueLabel = TEXT("RETOURNER DANS LA ZONE");
		}
		// Résumé INTERMÉDIAIRE (pertes de la bataille du Kraken), puis bouton "Continuer".
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		BuildBattleSummary(true, /*bFinal=*/false, TEXT("KRAKEN VAINCU"));
		if (Demo) Demo->SetScreen(EDemoScreen::Summary);
		Say(TEXT("Le Kraken est vaincu ! Consultez le resume, puis lancez la defense."));
		// FLUX v0.8 : après le résumé, on enchaînera la séquence de purification via
		// BeginPostCreatureSequence (déclenchée au clic « Continuer » — voir OnSummaryContinue).
	}
	else if (Phase == EDemoPhase::Battle_Rival)
	{
		// Le bâtiment a tenu, mais ses dégâts PERSISTENT : la réparation se paie ensuite
		// en cité selon le pourcentage manquant. L'ancien remplissage gratuit est supprimé.
		if (Demo && CaptureObject)
		{
			Demo->SnapshotTerritoryBuilding(
				CaptureObject->CurrentHealth, CaptureObject->MaxHealth);
		}
		if (Demo)
		{
			Demo->ResolveZoneThreat();
			Demo->SetDefenseMissionReady(false);
			Demo->GrantMissionRewards(DefenseRewardCrystals,
				DefenseRewardAbyssalMaterials, DefenseRewardBiomass, DefenseRewardOceanicEnergy);
			Demo->GrantProgressionXP(60, 70);
			Demo->SummaryContinueLabel = TEXT("SECURISER LA ZONE");
		}
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		BuildBattleSummary(true, /*bFinal=*/false, TEXT("VICTOIRE — LA FACTION RIVALE RECULE"));
		if (Demo) Demo->SetScreen(EDemoScreen::Summary);
		Say(TEXT("La rivale est repoussee. Reparez le batiment et installez sa premiere defense autonome."));
	}
	else if (Phase == EDemoPhase::Battle_Grand)
	{
		// Fin de la PHASE 3 : résumé FINAL de démo (victoire) -> Rejouer / Changer de faction.
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		// Titre CORRIGE (01/08/2026, retour terrain : capture montrant "VICTOIRE TOTALE" avec
		// seulement 14/60 ennemis tues -- le vrai critere au chrono ecoule est "plus d'unites
		// vivantes que l'ennemi", pas l'aneantissement, donc "TOTALE" etait trompeur dans ce
		// cas). "VICTOIRE TOTALE" reste reservee au cas ou l'ennemi est REELLEMENT aneanti
		// (EnemyAlive == 0, cf. CheckBattleEnd) ; sinon "VICTOIRE" simple, plus honnete.
		const int32 EnemyAliveAtEnd = CountAlive(CachedRivalFaction);
		BuildBattleSummary(true, /*bFinal=*/true,
			EnemyAliveAtEnd <= 0 ? TEXT("VICTOIRE TOTALE") : TEXT("VICTOIRE — SUPERIORITE NUMERIQUE AU TEMPS LIMITE"));
		if (Demo)
		{
			// Récompense d'XP du CLIMAX — la plus grosse du jeu, manquait totalement jusqu'ici
			// (seules les 2 batailles précédentes en accordaient). Demande Liamor 30/07/2026 :
			// "il n'y a aucun but à l'objectif" -> le but final doit être le mieux récompensé.
			Demo->GrantProgressionXP(150, 150);
			ReplayPhase = EDemoPhase::Battle_Grand; // « Rejouer » relance la grande bataille
			Demo->bDemoVictory = true;
			Demo->SetPhase(EDemoPhase::DemoEnd);
			Demo->SetScreen(EDemoScreen::Summary);
		}
	}
}

void AWOTOLDemoDirector::OnPlayerDefeat()
{
	// (Musique geree par l'ecran : SummaryMusic des le passage a l'ecran Summary.)
	GetWorldTimerManager().ClearTimer(SiegeHandle);
	GetWorldTimerManager().ClearTimer(TacticalHandle);
	GetWorldTimerManager().ClearTimer(AdaptiveBalanceHandle);
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->EndBattle(CachedRivalFaction, EBattleResult::Defeat);
	}
	GetWorldTimerManager().ClearTimer(BattleCheckHandle);
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			// MÉMORISE la phase perdue AVANT de basculer sur DemoEnd -> « Rejouer » la relance.
			ReplayPhase = Demo->GetPhase();
			const bool bDefenseDefeat = ReplayPhase == EDemoPhase::Battle_Rival;
			if (bDefenseDefeat && CaptureObject && CaptureObject->CurrentHealth > 0.f)
			{
				Demo->SnapshotTerritoryBuilding(
					CaptureObject->CurrentHealth, CaptureObject->MaxHealth);
				Demo->bSummaryBuildingDestroyed = false;
			}
			else if (bDefenseDefeat)
			{
				Demo->MarkZoneLost();
				Demo->bSummaryBuildingDestroyed = true;
			}
			Demo->bSummaryCanReturnToCity = bDefenseDefeat;
			BuildBattleSummary(false, /*bFinal=*/true,
				bDefenseDefeat ? TEXT("ECHEC — DEFENSE DE LA ZONE") : TEXT("DEFAITE"));
			Demo->bDemoVictory = false;
			if (!bDefenseDefeat) Demo->SetPhase(EDemoPhase::DemoEnd);
			Demo->SetScreen(EDemoScreen::Summary);
		}
	}
}

// Agrège SpawnedUnits (morts INCLUS — les unités C++ ne sont pas détruites à la mort)
// par faction + nom d'unité pour produire le détail des pertes des deux camps.
void AWOTOLDemoDirector::BuildBattleSummary(bool bVictory, bool bFinal, const FString& Title)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	auto Accumulate = [](TArray<FUnitLossEntry>& Out, const AWOTOLDemoUnit* U, const FString& Name)
	{
		FUnitLossEntry* E = Out.FindByPredicate([&](const FUnitLossEntry& X){ return X.UnitName == Name; });
		if (!E)
		{
			FUnitLossEntry New; New.UnitName = Name; New.Faction = U->GetFaction();
			if (const UUnitDataAsset* D = U->GetUnitData())
			{
				New.DefPct   = FMath::RoundToInt(D->Stats.DefensePercent);
				New.BlockPct = FMath::RoundToInt(D->Stats.BlockChance);
				New.DodgePct = FMath::RoundToInt(D->Stats.DodgeChance);
			}
			E = &Out[Out.Add(New)];
		}
		E->Total++;
		if (!U->IsAlive()) E->Lost++;
		E->DamageDealt += U->DamageDealt;
	};

	Demo->PlayerLosses.Reset();
	Demo->EnemyLosses.Reset();

	for (const TObjectPtr<AWOTOLDemoUnit>& U : SpawnedUnits)
	{
		if (!U) continue;
		const bool bBoss = U->bCreatureBrain || U->bIsBoss;
		const FString Name = bBoss ? FString(TEXT("Kraken"))
			: ((U->GetUnitData() && !U->GetUnitData()->DisplayName.IsEmpty())
				? U->GetUnitData()->DisplayName.ToString() : U->GetName());
		const bool bPlayer = (U->GetFaction() == CachedPlayerFaction);
		Accumulate(bPlayer ? Demo->PlayerLosses : Demo->EnemyLosses, U, Name);
	}

	Demo->SummaryTitle    = Title;
	Demo->bSummaryVictory = bVictory;
	Demo->bSummaryIsFinal = bFinal;
	if (bVictory)
	{
		Demo->bSummaryCanReturnToCity = false;
		Demo->bSummaryBuildingDestroyed = false;
	}
	Demo->SummaryDurationSeconds = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - BattleStartTime);
	if (bAdaptiveBalanceActive && AdaptiveTargetLossMax > 0)
	{
		const int32 ActualLosses = FMath::Clamp(
			AdaptiveInitialPlayerCount - CountAlive(CachedPlayerFaction),
			0, AdaptiveInitialPlayerCount);
		UE_LOG(LogTemp, Log,
			TEXT("[WOTOL Balance] Resultat pertes=%d, plage=%d..%d, cible=%d, effectif=%d"),
			ActualLosses, AdaptiveTargetLossMin, AdaptiveTargetLossMax,
			AdaptiveTargetLossPreferred, AdaptiveInitialPlayerCount);
	}
}

void AWOTOLDemoDirector::ContinueFromBattleSummary()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	if (Demo->GetPhase() == EDemoPhase::Battle_Rival)
	{
		BeginPostDefenseTransition();
		return;
	}
	ShowInterlude();
}

void AWOTOLDemoDirector::BeginPostDefenseTransition()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	Demo->SetMessage(TEXT("Stabilisation de la zone — preparation des reparations..."));
	Demo->SetScreen(EDemoScreen::Loading);
	GetWorldTimerManager().ClearTimer(ExplorationTransitionHandle);
	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::EnterPostDefenseManagement, 1.35f, false);
}

void AWOTOLDemoDirector::EnterPostDefenseManagement()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	if (CaptureObject)
	{
		Demo->SnapshotTerritoryBuilding(
			CaptureObject->CurrentHealth, CaptureObject->MaxHealth);
	}
	CleanupUnits();
	PossessBattleCamera();
	Demo->SetPhase(EDemoPhase::Repair_Zone);
	if (Demo->GetProgress().bZoneLost)
	{
		if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
		Demo->SetScreen(EDemoScreen::City);
	}
	else
	{
		Demo->SetScreen(EDemoScreen::Territory);
		CreateDefensePlacementMarkers();
		RefreshDefenseStructuresFromTerritory();
	}
	RefreshPostDefenseObjective();
}

void AWOTOLDemoDirector::RequestTerritoryRepair()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->RepairTerritory()) return;
	if (CaptureObject)
	{
		CaptureObject->Repair(CaptureObject->MaxHealth);
		Demo->SnapshotTerritoryBuilding(
			CaptureObject->CurrentHealth, CaptureObject->MaxHealth);
	}
	RefreshPostDefenseObjective();
}

void AWOTOLDemoDirector::RequestInstallDefense()
{
	ArmDefensePlacement();
}

void AWOTOLDemoDirector::RequestAssignGarrison()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->AssignGarrisonUnit()) return;
	SyncFortificationToTerritoryManager();
	RefreshPostDefenseObjective();
}

void AWOTOLDemoDirector::RequestAssignGarrisonByCategory(EDemoUnitCategory Category)
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	const FName UnitID = Demo->GetUnitID(CachedPlayerFaction, Category);
	if (!Demo->AssignGarrisonUnitByID(UnitID)) return;
	SyncFortificationToTerritoryManager();
	RefreshPostDefenseObjective();
}

void AWOTOLDemoDirector::RequestRemoveGarrisonByCategory(EDemoUnitCategory Category)
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	const FName UnitID = Demo->GetUnitID(CachedPlayerFaction, Category);
	if (!Demo->RemoveGarrisonUnitByID(UnitID)) return;
	SyncFortificationToTerritoryManager();
	RefreshPostDefenseObjective();
}

void AWOTOLDemoDirector::ArmDefensePlacement()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->CanInstallNextDefense()) return;
	bDefensePlacementArmed = true;
	Demo->SetObjective(TEXT("Selectionnez l'un des cinq emplacements lumineux autour du batiment"));
}

bool AWOTOLDemoDirector::TryPlaceDefenseAt(const FVector& ClickedWorldLocation)
{
	if (!bDefensePlacementArmed || DefenseSlotLocations.Num() != 5) return false;
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return false;
	int32 BestSlot = INDEX_NONE;
	float BestDistance = 430.f;
	for (int32 Slot = 0; Slot < DefenseSlotLocations.Num(); ++Slot)
	{
		if (Demo->InstalledDefenseSlots.Contains(Slot)) continue;
		const float Distance = FVector::Dist2D(ClickedWorldLocation, DefenseSlotLocations[Slot]);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestSlot = Slot;
		}
	}
	if (BestSlot == INDEX_NONE || !Demo->InstallDefenseAtSlot(BestSlot)) return false;
	bDefensePlacementArmed = false;
	SyncFortificationToTerritoryManager();
	RefreshDefenseStructuresFromTerritory();
	CreateDefensePlacementMarkers();
	RefreshPostDefenseObjective();
	return true;
}

void AWOTOLDemoDirector::ReturnToCityAfterTerritorySecured()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || Demo->GetRepairCrystalCost() > 0 || Demo->InstalledDefenseCount <= 0) return;
	ClearDefensePlacementMarkers();
	Demo->SetMessage(TEXT("Retour a la cite — le juvenile ressent l'appel de la biomasse..."));
	Demo->SetScreen(EDemoScreen::Loading);
	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::CompleteReturnToCityAfterTerritorySecured, 1.35f, false);
}

void AWOTOLDemoDirector::CompleteReturnToCityAfterTerritorySecured()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	PossessBattleCamera();
	Demo->RefreshBiomassGoal();
	Demo->SetScreen(EDemoScreen::City);
	Demo->SetObjective(FString::Printf(TEXT(
		"Nourrissez le %s — biomasse disponible %d / %d"),
		*MythicDisplayName(CachedPlayerFaction), Demo->PlayerBiomass,
		Demo->MythicGrowthBiomassGoal));
}

void AWOTOLDemoDirector::FeedMythicAndContinue()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->FeedMythicForGrowth()) return;
	Demo->SetMessage(FString::Printf(TEXT("Le %s grandit..."),
		*MythicDisplayName(CachedPlayerFaction)));
	Demo->SetScreen(EDemoScreen::Loading);
	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::EnterMythicGrowthInterlude, 1.8f, false);
}

void AWOTOLDemoDirector::EnterMythicGrowthInterlude()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	Demo->UnlockAll();
	Demo->SetInterludeText(FString::Printf(TEXT(
		"Votre victoire a consolide le royaume.\n"
		"Le heros atteint le niveau %d et la cite le niveau %d.\n\n"
		"Nourri par la biomasse recoltee, le %s juvenile a grandi.\n"
		"Il devient une unite mythique jouable pour la prochaine bataille.\n"
		"Son cadeau ouvrira une nouvelle voie de progression dans le jeu complet.\n\n"
		"Une grande zone voisine est maintenant contestee.\n"
		"Les armees se rassemblent pour un affrontement d'une ampleur inedite."),
		Demo->HeroLevel, Demo->CityLevel, *MythicDisplayName(CachedPlayerFaction)));
	Demo->SetScreen(EDemoScreen::Interlude);
}

void AWOTOLDemoDirector::RefreshPostDefenseObjective()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	if (Demo->GetProgress().bZoneLost)
	{
		Demo->SetObjective(TEXT("ZONE PERDUE — elle devra etre reconquise depuis un territoire adjacent"));
		return;
	}
	if (Demo->GetProgress().bZoneThreatened)
	{
		const int32 Remaining = FMath::CeilToInt(Demo->ZoneDefenseWindowRemainingSeconds);
		Demo->SetObjective(FString::Printf(TEXT(
			"ZONE MENACEE — repartez defendre dans %02d:%02d"), Remaining / 60, Remaining % 60));
		return;
	}
	if (Demo->GetRepairCrystalCost() > 0)
	{
		Demo->SetObjective(TEXT("Reparez le batiment territorial endommage"));
		return;
	}
	if (Demo->InstalledDefenseCount <= 0)
	{
		Demo->SetObjective(TEXT("Installez la premiere defense autonome de la zone"));
		return;
	}
	Demo->SetPhase(EDemoPhase::Territory_Management);
	Demo->SetObjective(TEXT(
		"Zone securisee — garnison facultative, puis retournez a la cite"));
}

void AWOTOLDemoDirector::ReturnToCityAfterDefenseDefeat()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || ReplayPhase != EDemoPhase::Battle_Rival) return;
	if (!Demo->bSummaryBuildingDestroyed && CaptureObject && CaptureObject->CurrentHealth > 0.f)
	{
		Demo->SnapshotTerritoryBuilding(
			CaptureObject->CurrentHealth, CaptureObject->MaxHealth);
		Demo->StartZoneThreat();
		if (UWorld* W = GetWorld())
		{
			if (UTerritoryStateManager* Territory = W->GetSubsystem<UTerritoryStateManager>())
			{
				Territory->SetZoneThreat(TEXT("NeutralZone_01"), CachedRivalFaction,
					Demo->ZoneDefenseReactionWindowSeconds);
			}
		}
		GetWorldTimerManager().SetTimer(TerritoryThreatHandle, this,
			&AWOTOLDemoDirector::TickTerritoryThreat, 1.f, true);
	}
	else
	{
		Demo->MarkZoneLost();
	}
	Demo->SetMessage(TEXT("Retour vers la cite — la situation strategique est mise a jour..."));
	Demo->SetScreen(EDemoScreen::Loading);
	GetWorldTimerManager().SetTimer(ExplorationTransitionHandle, this,
		&AWOTOLDemoDirector::CompleteReturnToCityAfterDefeat, 1.35f, false);
}

void AWOTOLDemoDirector::CompleteReturnToCityAfterDefeat()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	CleanupUnits();
	PossessBattleCamera();
	Demo->SetPhase(EDemoPhase::Repair_Zone);
	if (Demo->GetProgress().bZoneLost)
	{
		if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
		Demo->SetDefenseMissionReady(false);
		Demo->SetObjective(TEXT("ZONE PERDUE — elle devra etre reconquise depuis un territoire adjacent"));
	}
	else
	{
		Demo->SetDefenseMissionReady(true);
		const int32 Remaining = FMath::CeilToInt(Demo->ZoneDefenseWindowRemainingSeconds);
		Demo->SetObjective(FString::Printf(TEXT(
			"ZONE MENACEE — repartez defendre dans %02d:%02d"), Remaining / 60, Remaining % 60));
	}
	Demo->SetScreen(EDemoScreen::City);
}

void AWOTOLDemoDirector::TickTerritoryThreat()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	const bool bExpired = Demo->TickZoneThreat(1.f);
	if (UWorld* W = GetWorld())
	{
		if (UTerritoryStateManager* Territory = W->GetSubsystem<UTerritoryStateManager>())
		{
			Territory->TickZoneThreat(TEXT("NeutralZone_01"), 1.f);
		}
	}
	if (bExpired)
	{
		GetWorldTimerManager().ClearTimer(TerritoryThreatHandle);
		if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
		ClearDefenseStructures();
		Demo->SetObjective(TEXT(
			"ZONE PERDUE — le batiment a cede et le territoire redevient neutre"));
	}
	else if (Demo->GetProgress().bZoneThreatened
		&& Demo->GetScreen() == EDemoScreen::City)
	{
		const int32 Remaining = FMath::CeilToInt(Demo->ZoneDefenseWindowRemainingSeconds);
		Demo->SetObjective(FString::Printf(TEXT(
			"ZONE MENACEE — repartez defendre dans %02d:%02d"), Remaining / 60, Remaining % 60));
	}
	if (Demo->GetScreen() == EDemoScreen::Territory) RefreshPostDefenseObjective();
}

// Écran de TRANSITION narrative (hors-champ) — appelé depuis le bouton du résumé phase 1.
// Raconte ce qui s'est passé entre les deux batailles et le déblocage de la distance,
// avec les NOMS propres à la faction jouée (œuf du Cœur-Éclat -> cité -> nouveau bâtiment).
void AWOTOLDemoDirector::ShowInterlude()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	// Flux actuel : après le rapport du Kraken, le joueur reprend réellement le contrôle du
	// héros dans la zone libérée. L'ancien écran hors-champ reste le repli de diagnostic.
	if (bEnableFullFlowV08 && Demo->GetPhase() == EDemoPhase::Battle_Creature)
	{
		ResumePostBattleExploration();
		return;
	}

	const FString Building = BuildingDisplayName(CachedPlayerFaction);
	const FString Ranged   = RangedUnitDisplayName(CachedPlayerFaction);
	const FString Mythic   = MythicDisplayName(CachedPlayerFaction);
	// Nom de la SPÉCIALE de la faction (débloquée en phase 3).
	const FString Special  = (CachedPlayerFaction == EFactionID::Noxeens) ? TEXT("Noxeons") : TEXT("Aquilombres");

	FString Lore;
	// La MÊME fonction sert aux deux transitions ; le texte dépend de la phase courante.
	if (Demo->GetPhase() == EDemoPhase::Battle_Rival)
	{
		// ── TRANSITION PHASE 2 -> PHASE 3 : ellipse temporelle ──
		Lore = FString::Printf(TEXT(
			"La faction rivale a ete repoussee et votre %s tient toujours.\n"
			"\n"
			"Les mois passent. Votre cite prospere et votre puissance grandit : le %s, autrefois\n"
			"juvenile, est devenu un veritable MYTHIQUE — il rejoint desormais vos rangs au combat.\n"
			"Vos artisans ont aussi acheve l'entrainement d'une unite d'elite : les %s.\n"
			"\n"
			"Mais la rivale n'a pas dit son dernier mot : elle revient EN FORCE, sur un TERRAIN\n"
			"NEUTRE et inconnu, avec elle aussi son mythique et son elite. Aucun avantage de\n"
			"territoire cette fois — seule la valeur de vos troupes decidera. ANEANTISSEZ-LES."),
			*Building, *Mythic, *Special);
	}
	else
	{
		// ── TRANSITION PHASE 1 -> PHASE 2 (Kraken vaincu -> defense de la zone) ──
		Lore = FString::Printf(TEXT(
			"Apres votre victoire sur le Kraken, une creature des abysses — prisonniere elle aussi\n"
			"des griffes du colosse — a ete liberee. Vous l'avez recueillie et adoptee : le %s,\n"
			"qui grandira pour devenir votre creature MYTHIQUE.\n"
			"\n"
			"De retour a votre cite, cette decouverte vous a apporte l'experience necessaire pour\n"
			"eriger un NOUVEAU batiment et former une nouvelle categorie : les %s (a distance).\n"
			"\n"
			"Mais la faction rivale a repere votre %s et lance l'assaut pour s'emparer de la zone.\n"
			"Deployez vos forces — distance comprise — et PROTEGEZ le batiment a tout prix."),
			*Mythic, *Ranged, *Building);
	}

	Demo->SetInterludeText(Lore);
	Demo->SetScreen(EDemoScreen::Interlude);
}

void AWOTOLDemoDirector::ContinueToPhase2()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	// Le MÊME bouton « Continuer » enchaîne : après la phase 2 (Battle_Rival déjà jouée) il
	// mène à la PHASE 3 (grande bataille neutre) au lieu de re-lancer la phase 2. On ne lance
	// PLUS directement la bataille depuis cet écran hors-champ : on repasse d'abord par la
	// cité (déjà débloquée par UnlockAll) pour que sa croissance se VOIE (demande de Liamor
	// du 26/07/2026 — la matérialisation ne doit pas reposer uniquement sur ce texte).
	if (Demo && (Demo->GetPhase() == EDemoPhase::Battle_Rival
		|| (Demo->GetProgress().bMythicPlayable
			&& Demo->GetPhase() == EDemoPhase::Territory_Management)))
	{
		ReturnToCityForGrandBattleReveal();
		return;
	}

	// FLUX v0.8 (module 10) : au lieu d'aller directement à la défense, on joue la
	// séquence de purification (Cristalliseur → Cœur-Éclat → œuf → cité → puis défense).
	if (bEnableFullFlowV08)
	{
		BeginPostCreatureSequence();
		return;
	}

	if (Demo)
	{
		Demo->UnlockRangedUnit();  // distance débloquée pour la phase 2
		Demo->DiscoverMythic();
	}
	SpawnCaptureObject(CachedPlayerFaction); // objet à défendre (visible en phase 2)
	StartRivalDefense();                     // -> phase 2 en PRÉPARATION
}

// Bouton « Partir en expédition » de la cité (module 5) -> lance la défense (phase 10).
void AWOTOLDemoDirector::LaunchDefenseFromCity()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->IsDefenseMissionReady()) return;

	PossessBattleCamera();
	DestroyExplorationHero();
	Demo->SetScreen(EDemoScreen::Playing);
	ClearCrystalliserPlacementMarkers();
	// Le bâtiment posé par le joueur persiste. Repli de sécurité uniquement pour une ancienne
	// sauvegarde/procédure de test qui entrerait dans la défense sans objet existant.
	if (!IsValid(CaptureObject)) SpawnCaptureObject(CachedPlayerFaction);
	StartRivalDefense();                     // -> défense en PRÉPARATION
}

// Retour obligatoire à la cité entre l'interlude de croissance et la grande bataille : la
// cité est DÉJÀ débloquée (UnlockAll appelé dans EnterMythicGrowthInterlude) donc les
// bâtiments Spéciale/Mythique s'affichent immédiatement actifs (AWOTOLCityBuildingProp::
// Refresh relit IsCategoryUnlocked à chaque frame) — le joueur les VOIT au lieu de se
// contenter du texte de l'interlude (demande de Liamor du 26/07/2026).
void AWOTOLDemoDirector::ReturnToCityForGrandBattleReveal()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	PossessBattleCamera();

	// PHASE 3 : l'armée passe d'une composition scriptée fixe à une base réduite (vétérans
	// de la phase 2, chef + mythique compris) + recrutement libre en cité, plafonné par
	// faction (60 Aquiloris / 100 Noxéens) — demande de Liamor du 26/07/2026. On repart
	// d'une réserve vide : la garnison de la phase 2 reste garder la zone, elle ne rejoint
	// pas la grande bataille en zone neutre.
	const EFactionID Fac = Demo->GetPlayerFaction();
	Demo->MaxArmyUnits = (Fac == EFactionID::Noxeens) ? 100 : 60;
	Demo->InitialArmyUnits = 2 // chef + mythique, ajoutés séparément dans SpawnPlayerArmy
		+ GrandBattleBaselineInfantry + GrandBattleBaselineMounted
		+ GrandBattleBaselineRanged + GrandBattleBaselineSpecial;
	Demo->TotalProducedUnits = 0;
	Demo->GarrisonUnits = 0;
	Demo->GarrisonByUnit.Empty();
	Demo->ReserveUnits.Empty();

	// Bonus de ressources "ellipse temporelle" : le texte de l'interlude dit explicitement
	// "les mois passent, votre cite prospere", mais rien ne matérialisait ça en jeu -> avec
	// seulement le reliquat de la phase 2, impossible d'approcher le plafond d'armee (Liamor
	// a fini la phase 3 avec 22/60 unites faute de ressources, defaite qui n'avait rien a voir
	// avec la tactique). Calcule de quoi produire la difference jusqu'au plafond meme en
	// partant de zero (cout moyen ~150/unite, marge incluse), plutot qu'un chiffre arbitraire.
	{
		const int32 UnitsToFund = FMath::Max(0, Demo->MaxArmyUnits - Demo->InitialArmyUnits);
		const int32 ProsperityCrystals = UnitsToFund * 170; // cout moyen (130-180) + marge
		Demo->GrantMissionRewards(ProsperityCrystals, ProsperityCrystals / 2, ProsperityCrystals / 3, 150);
	}

	Demo->SetReadyForGrandBattleDeparture(true);
	Demo->SetObjective(FString::Printf(TEXT(
		"VOTRE CITE A GRANDI — nouveaux batiments debloques. Recrutez votre armee (%d / %d) puis embarquez."),
		Demo->GetArmyUnitCount(), Demo->GetArmyUnitCap()));
	Demo->SetScreen(EDemoScreen::City);
}

// Bouton "EMBARQUER" affiché en cité tant que bReadyForGrandBattleDeparture est actif.
void AWOTOLDemoDirector::EmbarkGrandBattleFromCity()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->bReadyForGrandBattleDeparture) return;
	Demo->SetReadyForGrandBattleDeparture(false);
	StartGrandBattle();
}

// PHASE 3 — grande bataille rangée en ZONE NEUTRE : on débloque TOUT le roster (spéciale +
// mythique), on RETIRE l'objectif (pas de Cristalliseur -> pas d'avantage de zone : les
// deux camps n'ont que leurs bonus de faction), et on AGRANDIT l'arène (bataille épique).
void AWOTOLDemoDirector::StartGrandBattle()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	// Manquait ici : tous les autres points d'entree de bataille (LaunchDefenseFromCity,
	// TransitionExplorationToBattle...) possedent explicitement la camera de bataille avant
	// BeginPreparation(). Celui-ci ne le faisait pas -> le joueur restait sur la camera de
	// cite (EDemoScreen::City l'avait possedee juste avant, dans EmbarkGrandBattleFromCity)
	// pendant toute la Phase 3 : bug reel remonte par Liamor le 31/07/2026 (captures a l'appui
	// -> le decor de cite/le disque du sol restaient visibles derriere le HUD de bataille,
	// bataille jamais vue, seulement suivie via la minicarte).
	PossessBattleCamera();
	if (Demo) Demo->UnlockAll(); // spéciale (Aquilombres/Noxéons) + mythique (Léviaphénix/Noxedrake)

	// ZONE NEUTRE : plus aucun objet de capture -> le bloc d'avantage de zone (gaté sur
	// CaptureObject) est automatiquement ignoré. Bataille purement arme contre arme.
	if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
	if (Demo) Demo->SetCaptureObject(nullptr);
	ClearZoneCrystals();

	// Grande plaine : le décor de phase 3 repousse récifs, reliefs et collision à l'extérieur.
	ArmySeparation = 9000.f;
	PlacementBoundaryOffsetX = -2600.f;
	// Nettoie l'objectif/message résiduel de la phase 2 (sinon il reste affiché sous celui-ci).
	if (Demo) { Demo->SetMessage(TEXT("")); }
	if (UWorld* W = GetWorld())
	{
		if (UOceanCurrentSubsystem* Cur = W->GetSubsystem<UOceanCurrentSubsystem>())
			Cur->Regenerate();
		// REMODÈLE LE DÉCOR pour la phase 3 : autre lieu (palette/brume/disposition abyssales).
		for (TActorIterator<AWOTOLGreyboxEnvironment> It(W); It; ++It)
		{
			It->RebuildForPhase(3);
			break;
		}
	}

	if (Demo) Demo->SetPhase(EDemoPhase::Battle_Grand);
	BeginPreparation(); // -> phase 3 en PRÉPARATION (roster complet, arène agrandie)
}

void AWOTOLDemoDirector::RestartDemo(bool bKeepFaction)
{
	// Repart d'un état propre : plus d'armées, plus d'objet de capture, progression RAZ.
	GetWorldTimerManager().ClearTimer(BattleCheckHandle);
	GetWorldTimerManager().ClearTimer(PhaseHandle);
	GetWorldTimerManager().ClearTimer(BattleStartHandle);
	GetWorldTimerManager().ClearTimer(SiegeHandle);
	GetWorldTimerManager().ClearTimer(TacticalHandle);
	GetWorldTimerManager().ClearTimer(CrystalliserConstructionHandle);
	CleanupUnits();
	ClearPlacementBoundary();
	ClearCoverStructures();
	ClearCrystalliserPlacementMarkers();
	if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
	bBattleConcluded = false;
	// Roster + arène de phase 1 (les valeurs phase 2/3 sont réappliquées à leur lancement)
	InfantryCount = 10; MountedCount = 5; RangedCount = 5;
	ArmySeparation = 4500.f;
	PlacementBoundaryOffsetX = -1200.f;
	// Remet le TERRAIN de base (si on rejoue après la phase 3, qui l'avait passé en abyssal).
	if (UWorld* W = GetWorld())
		for (TActorIterator<AWOTOLGreyboxEnvironment> It(W); It; ++It) { It->RebuildForPhase(1); break; }

	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (Demo)
	{
		Demo->ResetProgress();          // phase None + déblocages remis à zéro
		Demo->PlayerLosses.Reset();
		Demo->EnemyLosses.Reset();
		// Efface TOUT résidu de l'écran de fin : bandeau victoire/défaite, objectif,
		// résumé, drapeau de fin -> on repart sur un HUD propre (aucune fenêtre restante).
		Demo->CurrentMessage.Empty();
		Demo->ObjectiveText.Empty();
		Demo->SummaryTitle.Empty();
		Demo->bDemoVictory = false;
		Demo->bSummaryIsFinal = false;
	}

	if (bKeepFaction)
	{
		BeginPreparation(); // rejoue la phase 1 avec la faction déjà choisie
	}
	else if (Demo)
	{
		Demo->SelectedFaction = EFactionID::None;
		Demo->SetScreen(EDemoScreen::FactionSelect); // re-choix de faction
	}
}

void AWOTOLDemoDirector::ReplayCurrentPhase()
{
	// Nettoyage commun MAIS on NE remet PAS la progression/faction à zéro : on rejoue
	// UNIQUEMENT la phase perdue (ReplayPhase), pas toute la démo.
	GetWorldTimerManager().ClearTimer(BattleCheckHandle);
	GetWorldTimerManager().ClearTimer(PhaseHandle);
	GetWorldTimerManager().ClearTimer(BattleStartHandle);
	GetWorldTimerManager().ClearTimer(SiegeHandle);
	GetWorldTimerManager().ClearTimer(TacticalHandle);
	GetWorldTimerManager().ClearTimer(CrystalliserConstructionHandle);
	CleanupUnits();
	ClearPlacementBoundary();
	ClearCoverStructures();
	ClearCrystalliserPlacementMarkers();
	if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
	bBattleConcluded = false;

	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (Demo)
	{
		Demo->PlayerLosses.Reset();
		Demo->EnemyLosses.Reset();
		Demo->CurrentMessage.Empty();
		Demo->ObjectiveText.Empty();
		Demo->SummaryTitle.Empty();
		Demo->bDemoVictory   = false;
		Demo->bSummaryIsFinal = false;
		Demo->SetCaptureObject(nullptr);
	}

	switch (ReplayPhase)
	{
	case EDemoPhase::Battle_Rival:
		// PHASE 2 : distance + mythique déjà découverts, NOUVEL objet de capture, terrain
		// standard, arène phase 2. BeginPreparation réapplique les effectifs de la phase 2.
		ArmySeparation = 4500.f;
		PlacementBoundaryOffsetX = -1200.f;
		if (UWorld* W = GetWorld())
			for (TActorIterator<AWOTOLGreyboxEnvironment> It(W); It; ++It) { It->RebuildForPhase(1); break; }
		if (Demo) { Demo->UnlockRangedUnit(); Demo->DiscoverMythic(); Demo->SetPhase(EDemoPhase::Battle_Rival); }
		SpawnCaptureObject(CachedPlayerFaction);
		BeginPreparation();
		break;

	case EDemoPhase::Battle_Grand:
		// PHASE 3 : StartGrandBattle remet tout en place (déblocage total, terrain abyssal,
		// arène agrandie, aucun objet de capture).
		if (Demo) Demo->SetPhase(EDemoPhase::Battle_Grand);
		StartGrandBattle();
		break;

	default: // Battle_Creature (ou inconnu) -> PHASE 1
		InfantryCount = 10; MountedCount = 5; RangedCount = 5;
		ArmySeparation = 4500.f;
		PlacementBoundaryOffsetX = -1200.f;
		if (UWorld* W = GetWorld())
			for (TActorIterator<AWOTOLGreyboxEnvironment> It(W); It; ++It) { It->RebuildForPhase(1); break; }
		if (Demo) Demo->SetPhase(EDemoPhase::Battle_Creature);
		BeginPreparation();
		break;
	}
}

void AWOTOLDemoDirector::ReturnToMainMenu()
{
	// Remet TOUTE la démo à zéro (comme RestartDemo sans faction) puis affiche l'ACCUEIL.
	RestartDemo(/*bKeepFaction=*/false);
	if (UGameInstance* GI = GetGameInstance())
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
			Demo->SetScreen(EDemoScreen::MainMenu);
}

void AWOTOLDemoDirector::CleanupUnits()
{
	ResetAdaptiveBattleBalance();
	for (TObjectPtr<AWOTOLDemoUnit>& U : SpawnedUnits)
	{
		if (U) U->Destroy();
	}
	SpawnedUnits.Empty();
	ClearDefenseStructures();
	ClearDefensePlacementMarkers();
}

void AWOTOLDemoDirector::ClearDefenseStructures()
{
	for (TObjectPtr<AWOTOLDefenseStructure>& D : DefenseStructures)
	{
		if (D) D->Destroy();
	}
	DefenseStructures.Empty();
}

void AWOTOLDemoDirector::RegisterDemoTerritoryGraph()
{
	UWorld* W = GetWorld();
	if (!W) return;
	UTerritoryStateManager* Territory = W->GetSubsystem<UTerritoryStateManager>();
	if (!Territory) return;

	FZoneState Capital;
	Capital.Owner = CachedPlayerFaction;
	Capital.CapturingFaction = CachedPlayerFaction;
	Capital.Grade = 4;
	Capital.CaptureProgress = 100.f;
	Capital.ConquestObjective = EZoneConquestObjective::None;
	Capital.bConquestObjectiveCompleted = true;
	Territory->RegisterZone(TEXT("FactionCapital_00"), Capital);

	FZoneState KrakenZone;
	KrakenZone.ConquestObjective = EZoneConquestObjective::GuardianCreature;
	Territory->RegisterZone(TEXT("NeutralZone_01"), KrakenZone);

	FZoneState FrontierZone;
	FrontierZone.ConquestObjective = EZoneConquestObjective::RivalArmy;
	Territory->RegisterZone(TEXT("ContestedFrontier_02"), FrontierZone);

	Territory->ConnectZones(TEXT("FactionCapital_00"), TEXT("NeutralZone_01"));
	Territory->ConnectZones(TEXT("NeutralZone_01"), TEXT("ContestedFrontier_02"));
}

void AWOTOLDemoDirector::SyncFortificationToTerritoryManager()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	if (UWorld* W = GetWorld())
	{
		if (UTerritoryStateManager* Territory = W->GetSubsystem<UTerritoryStateManager>())
		{
			Territory->SetZoneFortification(TEXT("NeutralZone_01"),
				Demo->InstalledDefenseCount, Demo->DefenseTechnologyLevel,
				Demo->GarrisonUnits, Demo->GetGarrisonCapacity());
		}
	}
}

void AWOTOLDemoDirector::CreateDefensePlacementMarkers()
{
	ClearDefensePlacementMarkers();
	if (!CaptureObject) return;
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	DefenseSlotLocations.Reset();
	const FVector Center = CaptureObject->GetActorLocation();
	const float Ring = 820.f;
	for (int32 Slot = 0; Slot < 5; ++Slot)
	{
		const float Angle = -PI * 0.5f + static_cast<float>(Slot) / 5.f * 2.f * PI;
		DefenseSlotLocations.Add(Center + FVector(
			FMath::Cos(Angle) * Ring, FMath::Sin(Angle) * Ring, -190.f));
	}

	UWorld* W = GetWorld();
	if (!W) return;
	const FLinearColor Color = CachedPlayerFaction == EFactionID::Noxeens
		? FLinearColor(0.20f, 1.5f, 0.45f, 1.f)
		: FLinearColor(0.25f, 0.85f, 2.6f, 1.f);
	for (int32 Slot = 0; Slot < DefenseSlotLocations.Num(); ++Slot)
	{
		if (Demo->InstalledDefenseSlots.Contains(Slot)) continue;
		AStaticMeshActor* Marker = W->SpawnActor<AStaticMeshActor>(
			DefenseSlotLocations[Slot], FRotator::ZeroRotator);
		if (!Marker) continue;
		UStaticMeshComponent* Mesh = Marker->GetStaticMeshComponent();
		if (Mesh)
		{
			Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,
				TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
			Mesh->SetWorldScale3D(FVector(2.0f, 2.0f, 0.08f));
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(this, Color))
				Mesh->SetMaterial(0, MID);
		}
		DefensePlacementMarkers.Add(Marker);
	}
}

void AWOTOLDemoDirector::ClearDefensePlacementMarkers()
{
	for (TObjectPtr<AActor>& Marker : DefensePlacementMarkers)
	{
		if (Marker) Marker->Destroy();
	}
	DefensePlacementMarkers.Empty();
	bDefensePlacementArmed = false;
}

void AWOTOLDemoDirector::RefreshDefenseStructuresFromTerritory()
{
	ClearDefenseStructures();
	if (!CaptureObject) return;
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	UWorld* W = GetWorld();
	if (!Demo || !W) return;

	if (DefenseSlotLocations.Num() != 5)
	{
		const FVector Center = CaptureObject->GetActorLocation();
		const float Ring = 820.f;
		DefenseSlotLocations.Reset();
		for (int32 Slot = 0; Slot < 5; ++Slot)
		{
			const float Angle = -PI * 0.5f + static_cast<float>(Slot) / 5.f * 2.f * PI;
			DefenseSlotLocations.Add(Center + FVector(
				FMath::Cos(Angle) * Ring, FMath::Sin(Angle) * Ring, -190.f));
		}
	}

	for (const int32 Slot : Demo->InstalledDefenseSlots)
	{
		if (!DefenseSlotLocations.IsValidIndex(Slot)) continue;
		const FTransform TM(FRotator::ZeroRotator, DefenseSlotLocations[Slot]);
		AWOTOLDefenseStructure* Defense = W->SpawnActorDeferred<AWOTOLDefenseStructure>(
			AWOTOLDefenseStructure::StaticClass(), TM, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Defense) continue;
		Defense->OwnerFaction = CachedPlayerFaction;
		Defense->StructureLevel = Demo->DefenseTechnologyLevel;
		UGameplayStatics::FinishSpawningActor(Defense, TM);
		DefenseStructures.Add(Defense);
	}
}

void AWOTOLDemoDirector::SpawnCaptureObject(EFactionID Faction)
{
	SpawnCaptureObjectAt(Faction, GetActorLocation() + FVector(0.f, 0.f, 200.f));
}

void AWOTOLDemoDirector::SpawnCaptureObjectAt(EFactionID Faction, const FVector& ActorLocation)
{
	if (IsValid(CaptureObject)) return;
	if (!CaptureObjectClass) return;

	const FTransform TM(FRotator::ZeroRotator, ActorLocation);
	AWOTOLCaptureObject* Obj = GetWorld()->SpawnActorDeferred<AWOTOLCaptureObject>(
		CaptureObjectClass, TM, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Obj) return;

	Obj->OwnerFaction = Faction;
	UGameplayStatics::FinishSpawningActor(Obj, TM);
	Obj->ClaimZone();
	CaptureObject = Obj;

	// Barre de vie du bâtiment au HUD + fin d'objectif si détruit.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetCaptureObject(Obj);
		}
	}
	Obj->OnCaptureDestroyed.AddDynamic(this, &AWOTOLDemoDirector::HandleCaptureDestroyed);

	// Aucune défense gratuite pendant le premier assaut. Après la victoire, le joueur choisit
	// l'un des cinq emplacements et paie sa première tourelle/sentinelle. Une sauvegarde ou
	// un retour ultérieur recrée uniquement les emplacements réellement installés.
	RefreshDefenseStructuresFromTerritory();
}

// SIÈGE : périodiquement, chaque unité rivale proche du bâtiment lui inflige des dégâts
// -> la barre de vie du bâtiment descend en temps réel. Le joueur doit tuer/écarter les
// assiégeants avant qu'il ne tombe à 0.
// ─────────────────────────────────────────────────────────────────────────────
// MODULE 8 — Séquence post-créature : Cristalliseur → Cœur-Éclat → œuf.
// Pilotée par les fenêtres d'objectif (validation manuelle = clic « Continuer »),
// avec récompenses greybox flottantes récupérables aussi par proximité du héros.
// ─────────────────────────────────────────────────────────────────────────────
void AWOTOLDemoDirector::BeginPostCreatureSequence()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	Demo->SetPhase(EDemoPhase::Capture_Zone);
	const FString Building = BuildingDisplayName(CachedPlayerFaction);
	Demo->OpenObjectiveWindow(TEXT("seq_place_crystalliser"),
		TEXT("NOUVEL OBJECTIF — PURIFIER LA ZONE"),
		FString::Printf(TEXT(
			"Le Kraken est vaincu. Vous possedez maintenant les ressources necessaires.\n"
			"Placez le %s pour acquerir et terraformer ce territoire.\n"
			"Cout provisoire : %d cristaux + %d mineraux abyssaux."),
			*Building, CrystalliserCrystalCost, CrystalliserAbyssalMaterialCost),
		FString::Printf(TEXT("PLACER LE %s"), *Building.ToUpper()));
}

void AWOTOLDemoDirector::BeginCrystalliserPlacement()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	bCrystalliserPlacementAvailable = true;
	bCrystalliserPlacementArmed = false;
	CrystalliserPlacementLocation = GetActorLocation() + FVector(0.f, 0.f, 18.f);
	CreateCrystalliserPlacementMarkers();
	Demo->SetObjective(FString::Printf(TEXT("Ouvrez l'inventaire puis placez le %s sur l'emplacement lumineux"),
		*BuildingDisplayName(CachedPlayerFaction)));

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PC->bShowMouseCursor = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
}

void AWOTOLDemoDirector::ArmCrystalliserPlacement()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !bCrystalliserPlacementAvailable || Demo->GetProgress().bZoneCaptured) return;
	if (!Demo->CanAffordTerritoryBuilding(
		CrystalliserCrystalCost, CrystalliserAbyssalMaterialCost))
	{
		Demo->OpenObjectiveWindow(TEXT("seq_place_crystalliser"),
			TEXT("RESSOURCES INSUFFISANTES"),
			TEXT("Le batiment territorial ne peut pas etre construit. Consultez le rapport de mission."),
			TEXT("REESSAYER"), true);
		return;
	}
	bCrystalliserPlacementArmed = true;
	Demo->SetObjective(TEXT("Cliquez sur l'emplacement circulaire lumineux pour confirmer la construction"));
}

bool AWOTOLDemoDirector::TryPlaceCrystalliserAt(const FVector& ClickedWorldLocation)
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !bCrystalliserPlacementAvailable || !bCrystalliserPlacementArmed) return false;
	if (FVector::Dist2D(ClickedWorldLocation, CrystalliserPlacementLocation)
		> CrystalliserPlacementRadius)
	{
		Demo->SetObjective(TEXT("Emplacement invalide — cliquez dans le cercle lumineux"));
		return false;
	}
	if (!Demo->SpendTerritoryBuildingCost(
		CrystalliserCrystalCost, CrystalliserAbyssalMaterialCost))
	{
		Demo->OpenObjectiveWindow(TEXT("seq_place_crystalliser"),
			TEXT("RESSOURCES INSUFFISANTES"),
			TEXT("La construction a ete annulee : le cout complet n'est plus disponible."),
			TEXT("REESSAYER"), true);
		return false;
	}

	CompleteCrystalliserPlacement();
	return true;
}

void AWOTOLDemoDirector::CompleteCrystalliserPlacement()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	bCrystalliserPlacementAvailable = false;
	bCrystalliserPlacementArmed = false;
	ClearCrystalliserPlacementMarkers();
	SpawnCaptureObjectAt(CachedPlayerFaction,
		FVector(CrystalliserPlacementLocation.X, CrystalliserPlacementLocation.Y,
			GetActorLocation().Z + 200.f));
	if (!CaptureObject) return;

	// ClaimZone est exécuté par SpawnCaptureObjectAt : le territoire appartient déjà au
	// joueur. Mais le bâtiment ne doit pas apparaître fini instantanément -> animation de
	// construction (montée en échelle), puis seulement là les récompenses sont révélées.
	CaptureObject->BeginConstruction(CrystalliserConstructionSeconds);
	Demo->SetObjective(FString::Printf(TEXT("Construction du %s en cours..."),
		*BuildingDisplayName(CachedPlayerFaction)));
	GetWorldTimerManager().SetTimer(CrystalliserConstructionHandle, this,
		&AWOTOLDemoDirector::FinishCrystalliserConstruction,
		CrystalliserConstructionSeconds, false);
}

void AWOTOLDemoDirector::FinishCrystalliserConstruction()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !CaptureObject) return;

	// Bâtiment achevé mais encore sans défense : on le dit clairement avant de révéler la
	// récompense. La pose effective des tourelles se fait depuis la vue Territoire
	// (bouton « Installer une défense »), accessible dès maintenant et avant la contre-attaque.
	Demo->OpenObjectiveWindow(TEXT("seq_defense_prompt"),
		TEXT("BATIMENT ACHEVE — ZONE EXPOSEE"),
		FString::Printf(TEXT(
			"Le %s est construit : le territoire vous appartient desormais et vous octroie\n"
			"bonus d'attaque/defense et capacite d'armee accrue.\n"
			"Il reste sans defense active : pensez a installer une tourelle/sentinelle\n"
			"(vue Territoire) avant que la faction rivale ne riposte."),
			*BuildingDisplayName(CachedPlayerFaction)),
		TEXT("COMPRIS"));
}

void AWOTOLDemoDirector::CreateCrystalliserPlacementMarkers()
{
	ClearCrystalliserPlacementMarkers();
	UWorld* W = GetWorld();
	if (!W) return;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube) return;

	const FLinearColor Col = FFactionColors::Get(CachedPlayerFaction) * 3.2f;
	constexpr int32 Segments = 16;
	for (int32 i = 0; i < Segments; ++i)
	{
		const float A = 2.f * PI * static_cast<float>(i) / static_cast<float>(Segments);
		const FVector Loc = CrystalliserPlacementLocation + FVector(
			FMath::Cos(A) * CrystalliserPlacementRadius,
			FMath::Sin(A) * CrystalliserPlacementRadius, 0.f);
		FActorSpawnParameters P;
		P.Owner = this;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Marker = W->SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(), Loc,
			FRotator(0.f, FMath::RadiansToDegrees(A) + 90.f, 0.f), P);
		if (!Marker) continue;
		UStaticMeshComponent* Mesh = Marker->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetWorldScale3D(FVector(0.85f, 0.12f, 0.07f));
		if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(Marker, Col))
			Mesh->SetMaterial(0, MID);
		CrystalliserPlacementMarkers.Add(Marker);
	}

	// Aperçu HOLOGRAPHIQUE : la même forme que le vrai bâtiment (kitbash identique via
	// AWOTOLCaptureObject), en coquille translucide pulsante — le joueur voit exactement
	// ce qu'il va poser, pas juste un cercle générique.
	if (CaptureObjectClass)
	{
		const FTransform TM(FRotator::ZeroRotator, CrystalliserPlacementLocation + FVector(0.f, 0.f, 18.f));
		AWOTOLCaptureObject* Ghost = W->SpawnActorDeferred<AWOTOLCaptureObject>(
			CaptureObjectClass, TM, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Ghost)
		{
			Ghost->OwnerFaction = CachedPlayerFaction;
			Ghost->SetGhostPreviewMode(true);
			UGameplayStatics::FinishSpawningActor(Ghost, TM);
			CrystalliserGhost = Ghost;
		}
	}
}

void AWOTOLDemoDirector::ClearCrystalliserPlacementMarkers()
{
	for (TObjectPtr<AActor>& Marker : CrystalliserPlacementMarkers)
	{
		if (Marker) Marker->Destroy();
	}
	CrystalliserPlacementMarkers.Empty();

	if (CrystalliserGhost)
	{
		CrystalliserGhost->Destroy();
		CrystalliserGhost = nullptr;
	}
}

void AWOTOLDemoDirector::NotifyRangedProductionObjectiveComplete()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo || !Demo->IsRangedProductionObjectiveComplete() || Demo->WasRivalAlertShown()) return;

	Demo->MarkRivalAlertShown();
	Demo->SetPhase(EDemoPhase::Exploration_Rival);
	const FString RivalName = CachedRivalFaction == EFactionID::Noxeens
		? TEXT("NOXEENNE") : TEXT("AQUILORIS");
	Demo->OpenObjectiveWindow(TEXT("city_nox_alert"),
		FString::Printf(TEXT("ALERTE — CONTRE-ATTAQUE %s"), *RivalName),
		FString::Printf(TEXT("Des forces rivales convergent vers le territoire que vous venez d'acquerir.\n"
			"Leur objectif est votre %s. Preparez vos troupes puis partez defendre la zone."),
			*BuildingDisplayName(CachedPlayerFaction)),
		TEXT("PREPARER LA DEFENSE"));
}

AWOTOLRewardActor* AWOTOLDemoDirector::SpawnReward(EWOTOLRewardType Type, const FVector& Loc)
{
	UWorld* W = GetWorld();
	if (!W) return nullptr;
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLRewardActor* R = W->SpawnActor<AWOTOLRewardActor>(
		AWOTOLRewardActor::StaticClass(), Loc, FRotator::ZeroRotator, P);
	if (R)
	{
		R->RewardType = Type;
		R->OnRewardCollected.AddDynamic(this, &AWOTOLDemoDirector::HandleRewardCollected);
		ActiveReward = R;
	}
	return R;
}

void AWOTOLDemoDirector::HandleRewardCollected(EWOTOLRewardType Type)
{
	// Récupération par PROXIMITÉ = équivalent au clic « Continuer » de la fenêtre en cours.
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;
	if (Demo->IsObjectiveWindowOpen())
	{
		Demo->ConfirmObjectiveWindow(); // déclenche HandleObjectiveConfirmed avec l'étape courante
	}
}

void AWOTOLDemoDirector::HandleObjectiveConfirmed(FName StepId)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	const FVector Center = GetActorLocation();

	if (StepId == TEXT("intro_begin_exploration"))
	{
		// Le clic ferme l'introduction ; la souris est reprise par la caméra 3e personne.
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
		Demo->SetObjective(TEXT("Explorez la zone — approchez-vous du Kraken (5 a 10 m)"));
	}
	else if (StepId == TEXT("seq_place_crystalliser"))
	{
		// Le bouton de la fenêtre n'achète plus automatiquement le bâtiment : il ouvre le vrai
		// mode de placement (inventaire -> cible 3D -> clic -> paiement).
		BeginCrystalliserPlacement();
	}
	else if (StepId == TEXT("seq_defense_prompt"))
	{
		SpawnReward(EWOTOLRewardType::HeartShard,
			CrystalliserPlacementLocation + FVector(350.f, 0.f, 102.f));
		Demo->OpenObjectiveWindow(TEXT("seq_collect_heart"),
			TEXT("ZONE ACQUISE — COEUR-ECLAT"),
			FString::Printf(TEXT("Le %s terraforme maintenant ce territoire et renforce vos troupes locales.\n"
				"Un Coeur-Eclat a surgi a proximite : recuperez-le."),
				*BuildingDisplayName(CachedPlayerFaction)),
			TEXT("RECUPERER"));
	}
	else if (StepId == TEXT("seq_collect_heart"))
	{
		if (ActiveReward) { ActiveReward->Collect(); ActiveReward = nullptr; }
		// L'œuf du mythique apparaît (récompense finale de la conquête). BUG CORRIGE (retour
		// terrain 31/07/2026) : le titre/texte était codé en dur sur "Leviaphenix" quel que
		// soit la faction -> les joueurs Noxéens recevaient un texte Aquiloris. MythicDisplayName
		// (déjà utilisé ailleurs, cf. BuildingDisplayName juste au-dessus) fournit le bon nom.
		SpawnReward(EWOTOLRewardType::LeviaphenixEgg, Center + FVector(-350.f, 0.f, 60.f));
		Demo->DiscoverMythic();
		const FString MythicName = MythicDisplayName(CachedPlayerFaction);
		Demo->OpenObjectiveWindow(TEXT("seq_collect_egg"),
			FString::Printf(TEXT("OEUF DE %s"), *MythicName.ToUpper()),
			FString::Printf(TEXT("Un oeuf de %s vous attend.\nRecuperez-le : ce sera votre allie mythique."), *MythicName),
			TEXT("Recuperer l'oeuf"));
	}
	else if (StepId == TEXT("seq_collect_egg"))
	{
		if (ActiveReward) { ActiveReward->Collect(); ActiveReward = nullptr; }
		// Récompense : mythique débloqué, puis retour à la cité. Les ressources ont déjà été
		// créditées et affichées sur le rapport de bataille du Kraken.
		Demo->UnlockRangedUnit();
		Demo->SetPhase(EDemoPhase::City_Unlock);
		Demo->OpenObjectiveWindow(TEXT("seq_return_city"),
			TEXT("RETOUR A LA CITE"),
			TEXT("Rapportez l'oeuf a Aquilor.\nProduisez des renforts avant la contre-attaque noxeenne."),
			TEXT("Retour a la cite"));
	}
	else if (StepId == TEXT("seq_return_city"))
	{
		PossessBattleCamera();
		DestroyExplorationHero();
		if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
		{
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI Mode;
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
		}
		Demo->SetObjective(TEXT("Construisez le batiment a distance puis produisez 10 unites"));
		Demo->SetScreen(EDemoScreen::City);
	}
	else if (StepId == TEXT("city_nox_alert"))
	{
		Demo->SetDefenseMissionReady(true);
		Demo->SetObjective(TEXT("Defendez le Cristalliseur contre la contre-attaque noxeenne"));
		Demo->SetScreen(EDemoScreen::City);
	}
	// (Les autres étapes du flux 13 phases seront ajoutées au module 10.)
}

void AWOTOLDemoDirector::SiegeTick()
{
	if (bBattleConcluded || !CaptureObject) return;
	UWorld* W = GetWorld();
	if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;

	const FVector BuildingLoc = CaptureObject->GetActorLocation();
	const float   SiegeRange  = 500.f;  // seulement les unités VRAIMENT au contact
	int32 Attackers = 0;
	for (AUnitBase* U : Reg->GetUnitsForFaction(CachedRivalFaction))
	{
		if (!U || !U->IsAlive()) continue;
		if (FVector::Dist2D(U->GetActorLocation(), BuildingLoc) <= SiegeRange)
		{
			++Attackers;
		}
	}
	if (Attackers > 0)
	{
		// 3 PV/s par assiégeant au contact, PLAFONNÉ à 24/s. Avec l'objectif renforcé
		// (16000 PV), même sous siège TOTAL non contré il tient ~660 s > chrono (600 s) :
		// l'objectif est DÉFENDABLE. Les défenseurs qui écartent des assiégeants le
		// sauvent largement. [Réglable : cap 24 = équilibré, plus haut = plus dur]
		const float Damage = FMath::Min(Attackers * 3.f, 24.f);
		CaptureObject->ApplyDamage(Damage);
		// NB : la destruction (PV <= 0) déclenche OnCaptureDestroyed -> HandleCaptureDestroyed,
		// qui applique la DÉFAITE IMMÉDIATE (§16). Rien à faire de plus ici.
	}
}

FVector AWOTOLDemoDirector::FactionCentroid(EFactionID Faction) const
{
	FVector C = FVector::ZeroVector; int32 N = 0;
	if (UWorld* W = GetWorld())
	{
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction(Faction))
			{
				if (!U || !U->IsAlive()) continue;
				C += U->GetActorLocation(); ++N;
			}
		}
	}
	return (N > 0) ? C / N : GetActorLocation();
}

// Ré-évaluation TACTIQUE (toutes ~3,5 s) : chaque camp poursuit l'adversaire, étage ses
// unités sur les couches verticales selon le rôle, et envoie ~1/3 en contournement de
// flanc (en passant par une couche haute). -> les armées manœuvrent au lieu de rester figées.
void AWOTOLDemoDirector::TacticalTick()
{
	if (bBattleConcluded) return;
	UWorld* W = GetWorld();
	if (!W) return;
	UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Reg) return;

	const FVector PlayerC = FactionCentroid(CachedPlayerFaction);
	const FVector RivalC  = FactionCentroid(CachedRivalFaction);
	const float Now = W->GetTimeSeconds();
	UOceanCurrentSubsystem* Cur = W->GetSubsystem<UOceanCurrentSubsystem>();

	// OBJECTIF de mission (phase 2) : le Cristalliseur/Abyssalyseur à défendre.
	// S'il existe, l'IA adapte tout son comportement autour de lui (défense / assaut).
	const bool    bHasObj = (CaptureObject != nullptr);
	const FVector ObjLoc  = bHasObj ? CaptureObject->GetActorLocation() : GetActorLocation();

	// PHASE 1 : détecte le boss (Kraken) — l'objectif est de l'ANÉANTIR. Les unités du
	// joueur l'ASSAILLENT (mêlée au contact, distance au large, montures en charge).
	bool    bHasBoss = false;
	FVector BossLoc  = GetActorLocation();
	if (!bHasObj)
	{
		for (AWOTOLDemoUnit* U : SpawnedUnits)
		{
			if (U && U->bCreatureBrain && U->IsAlive())
			{
				bHasBoss = true; BossLoc = U->GetActorLocation(); break;
			}
		}
	}

	// Centre de gravité des unités d'un rôle donné (pour cibler la ligne arrière adverse).
	auto RoleCentroid = [&](EFactionID F, EUnitRole Want, const FVector& Fallback) -> FVector
	{
		FVector C = FVector::ZeroVector; int32 N = 0;
		for (AUnitBase* U : Reg->GetUnitsForFaction(F))
		{
			if (!U || !U->IsAlive() || !U->GetUnitData()) continue;
			if (U->GetUnitData()->Role != Want) continue;
			C += U->GetActorLocation(); ++N;
		}
		return (N > 0) ? C / N : Fallback;
	};

	// Unité ennemie qui MENACE le plus l'objectif : celle (de préférence à distance)
	// la plus proche du Cristalliseur. Les défenseurs la prennent pour cible prioritaire.
	auto ObjectiveThreat = [&](EFactionID EnemyFac, bool bPreferRanged) -> AUnitBase*
	{
		AUnitBase* Best = nullptr; float BestScore = TNumericLimits<float>::Max();
		for (AUnitBase* U : Reg->GetUnitsForFaction(EnemyFac))
		{
			if (!U || !U->IsAlive() || !U->GetUnitData()) continue;
			float Score = FVector::Dist2D(U->GetActorLocation(), ObjLoc);
			// Un tireur qui canarde le bâtiment est plus dangereux qu'un mêlée équidistant.
			if (bPreferRanged && U->GetUnitData()->Role == EUnitRole::Distance) Score *= 0.5f;
			if (Score < BestScore) { BestScore = Score; Best = U; }
		}
		return Best;
	};

	auto CommandArmy = [&](EFactionID Fac, EFactionID EnemyFac, const FVector& OwnC,
		const FVector& EnemyC, bool bIsPlayer, bool bDefendObj)
	{
		const FVector Fwd     = (EnemyC - OwnC).GetSafeNormal2D();            // vers l'ennemi
		const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
		const FVector Front   = OwnC + (EnemyC - OwnC) * 0.35f;               // ligne de front (côté allié)
		// Ligne arrière adverse (unités à distance) = cible prioritaire des flanqueurs.
		const FVector EnemyRangedC = RoleCentroid(EnemyFac, EUnitRole::Distance, EnemyC);
		const FVector ToEnemyFromObj = (EnemyC - ObjLoc).GetSafeNormal2D();
		int32 idx = 0, infCol = 0, disCol = 0, monCol = 0;                    // colonnes / anneau par rôle
		// MUR DE BOUCLIERS : nombre de porte-boucliers vivants (pour CENTRER la ligne de
		// tortue et la garder serrée quelles que soient les pertes).
		int32 ShieldCount = 0;
		for (AUnitBase* U : Reg->GetUnitsForFaction(Fac))
			if (AWOTOLDemoUnit* D = Cast<AWOTOLDemoUnit>(U))
				if (D->IsAlive() && D->HasShield()) ++ShieldCount;
		for (AUnitBase* U : Reg->GetUnitsForFaction(Fac))
		{
			if (!U || !U->IsAlive()) continue;
			AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U);
			if (DU && DU->bCreatureBrain) continue; // le boss a son propre cerveau
			// L'ORDRE DU JOUEUR PRIME : on ne touche pas une unité qui vient de recevoir un
			// ordre (fenêtre de 8 s) NI une unité en train d'EXÉCUTER un déplacement ordonné
			// (bFollowingPlayerOrder) -> elle va au bout de son ordre, l'IA ne la détourne pas.
			if (bIsPlayer && DU)
			{
				const UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>();
				const bool bFollowing = St && St->bFollowingPlayerOrder;
				if (bFollowing || (Now - DU->LastPlayerOrderTime) < 12.f) { ++idx; continue; }
			}

			AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(U->GetController());
			if (!AIC) { ++idx; continue; }

			const UUnitDataAsset* Data = U->GetUnitData();
			const EUnitRole R = Data ? Data->Role : EUnitRole::Infanterie;
			const bool bCanLayer = Data ? Data->Stats.bCanChangeLayer : true;

			// ── COMPORTEMENT + FORMATION selon le RÔLE (synergie de faction) ──
			float   Layer = 0.f;
			FVector Dest  = EnemyC;

			// ── AQUILOMBRES (assassin fragile — réserve phase 3) : se tient en ARRIÈRE-LIGNE,
			// protégée par les lignes avant. Elle ne charge JAMAIS ; c'est sa compétence
			// (téléport dans le dos) qui frappe. Quelle que soit la phase. ──
			if (DU && Data && Data->GetFName() == TEXT("Aquilombres"))
			{
				const FVector Back = OwnC - Fwd * 500.f + Lateral * ((float)(idx % 3 - 1) * 220.f);
				DU->SetDesiredZ(0.f);
				AIC->ActivateRTSBehavior();
				AIC->IssueOrder_AttackMove(Back);
				if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
					St->SightRange = 900.f; // reste à l'arrière, n'engage que ce qui la menace
				++idx;
				continue;
			}

			// ═══ PHASE 1 : ASSAUT DU BOSS (Kraken) ═══ objectif = l'anéantir.
			if (bHasBoss && bIsPlayer)
			{
				const FVector ToBoss = (BossLoc - OwnC).GetSafeNormal2D();
				const FVector Side   = FVector::CrossProduct(FVector::UpVector, ToBoss).GetSafeNormal();
				switch (R)
				{
					case EUnitRole::Distance:
						// Canarde le Kraken en restant à distance (ne se jette pas dessus).
						Dest  = BossLoc - ToBoss * 900.f + Side * ((float)(disCol++ - 1) * 300.f);
						Layer = bCanLayer ? 1400.f : 0.f;
						break;
					case EUnitRole::Montee:
					{
						// Aquilances : rapides et mobiles -> ENCERCLENT. Un groupe part à
						// GAUCHE, l'autre à DROITE (voire par l'ARRIÈRE) pour désorienter le
						// Kraken et le prendre à flanc/dos à découvert. Elles varient aussi la
						// VERTICALITÉ (angle d'attaque haut/bas) au lieu de rester au sol.
						const int32 c = monCol++;
						const float SideSign = ((c + TacticalVariant) % 2 == 0) ? 1.f : -1.f;
						const bool  bRear    = ((c + TacticalVariant) % 3 == 0); // axe différent à chaque tentative
						const float Speed = Data ? Data->Stats.MovementSpeed : 1.f;
						const float Cycle = FMath::Max(5.f, 11.f - Speed * 3.f);
						const bool  bCharge = FMath::Fmod(Now + TacticalPhaseOffset
							+ idx * 1.3f, Cycle) < 4.f;
						const FVector FlankPos = bRear
							? (BossLoc + ToBoss * 520.f)                        // derrière le Kraken
							: (BossLoc - ToBoss * 500.f + Side * SideSign * 560.f); // large sur un flanc
						Dest  = bCharge ? (BossLoc + Side * SideSign * 150.f + (bRear ? ToBoss * 180.f : FVector::ZeroVector))
										: FlankPos;
						// Variation d'angle vertical : alternance sol / couche intermédiaire.
						Layer = (bCanLayer && (c % 2 == 1)) ? 900.f : 0.f;
						break;
					}
					case EUnitRole::Chef:
						// Le chef reste un peu en retrait (survivre pour placer sa compétence).
						Dest  = BossLoc - ToBoss * 650.f;
						Layer = 0.f;
						break;
					default: // Infanterie / Mythique / Spéciale : au CONTACT, encerclent le boss
					{
						const int32 c = infCol++;
						const float Ang = (2.f * PI) * ((float)c / 6.f);
						Dest  = BossLoc + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.f) * 240.f;
						Layer = 0.f;
						break;
					}
				}
				if (DU) DU->SetDesiredZ(Layer);
				AIC->ActivateRTSBehavior();
				AIC->IssueOrder_AttackMove(Dest);
				if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
					St->SightRange = 60000.f;
				++idx;
				continue;
			}

			// ═══ MODE OBJECTIF (phase 2) : l'IA sert l'objectif de mission ═══
			if (bHasObj && bDefendObj)
			{
				// DÉFENSE du Cristalliseur : mur-bouclier autour, tireurs qui visent la
				// ligne arrière adverse, montures qui foncent désorganiser les tireurs.
				switch (R)
				{
					case EUnitRole::Infanterie:
					{
						// ANNEAU DE BOUCLIERS (tortue autour du bâtiment), ANCRÉ AU SOL : bloque
						// le corps-à-corps ET les tirs venant de dehors. Pour un rempart dense,
						// les porte-boucliers s'étagent sur 2 rangs (intérieur/extérieur décalés)
						// -> pas de trou dans la muraille. Toujours couche 0 (blocage optimal).
						const int32 c = infCol++;
						if (DU && DU->HasShield())
						{
							const int32 Rank = (c % 2);                  // 0 = rang extérieur, 1 = rang intérieur
							const float Radius = Rank ? 300.f : 380.f;
							const float Ang = (2.f * PI) * ((float)(c / 2) / FMath::Max(1.f, ShieldCount * 0.5f))
								+ (Rank ? (PI / FMath::Max(1.f, ShieldCount)) : 0.f); // rang intérieur décalé (couvre les jointures)
							const FVector RD(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
							Dest  = ObjLoc + RD * Radius;
						}
						else
						{
							const float Ang = (2.f * PI) * ((float)c / 8.f);
							const FVector RD(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
							Dest  = ObjLoc + RD * 360.f;
						}
						Layer = 0.f;
						break;
					}
					case EUnitRole::Distance:
					{
						// Juste derrière l'anneau, en hauteur : CANARDE les tireurs adverses
						// pour les empêcher d'endommager l'objectif.
						Dest  = ObjLoc + (EnemyRangedC - ObjLoc).GetSafeNormal2D() * 500.f;
						Layer = bCanLayer ? 1400.f : 0.f;
						break;
					}
					case EUnitRole::Montee:
					{
						// FONCE sur la ligne arrière ennemie (tireurs) pour la désorganiser
						// et gagner un répit — hit-and-run temporel.
						const float Speed = Data ? Data->Stats.MovementSpeed : 1.f;
						const float Cycle = FMath::Max(6.f, 12.f - Speed * 3.f);
						// HIVE-MIND : toutes les Aquilances chargent EN MÊME TEMPS (vague unique,
						// pas de décalage par unité) -> percée coordonnée puis repli groupé.
						const bool  bCharge = FMath::Fmod(Now + TacticalPhaseOffset, Cycle) < 4.f;
						Dest  = bCharge ? EnemyRangedC : (ObjLoc + Fwd * 520.f);
						Layer = 0.f;
						break;
					}
					case EUnitRole::Chef:
						// Protégé DANS l'anneau (survit pour placer sa compétence).
						Dest  = ObjLoc - ToEnemyFromObj * 120.f;
						Layer = 0.f;
						break;
					default: // Mythique / Spéciale : tiennent le front côté ennemi de l'anneau
						Dest  = ObjLoc + ToEnemyFromObj * 340.f;
						Layer = bCanLayer ? 900.f : 0.f;
						break;
				}
				if (DU) DU->SetDesiredZ(Layer);
				AIC->ActivateRTSBehavior();
				AIC->IssueOrder_AttackMove(Dest);
				if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
				{
					// Porte-boucliers : tiennent l'anneau (vue réduite) ; les autres, vue large.
					St->SightRange = (DU && DU->HasShield()) ? 1100.f : 60000.f;
					// Tireurs et montures VERROUILLENT le tireur ennemi qui menace le
					// bâtiment (cible imposée) au lieu de taper le mêlée le plus proche
					// -> ils contrent réellement ce qui fait baisser les PV du Cristalliseur.
					if (R == EUnitRole::Distance || R == EUnitRole::Montee)
						St->ForceTarget = ObjectiveThreat(EnemyFac, /*bPreferRanged=*/true);
					else
						St->ForceTarget = nullptr;
				}
				++idx;
				continue;
			}
			if (bHasObj && !bDefendObj)
			{
				// ASSAUT : l'armée ATTAQUANTE partage son effort entre DEUX buts —
				// (1) détruire le Cristalliseur, (2) anéantir l'armée qui le défend.
				// ~55% ASSIÈGENT l'objectif, ~45% CHASSENT les défenseurs (sinon elle se
				// rue en masse sur le bâtiment et le détruit sans jamais combattre l'armée,
				// ne laissant aucune chance à la défense). EnemyC = centre de l'armée adverse.
				const bool bSiegeDuty = (((idx + TacticalVariant * 5) % 20) < 9);
				if (!bSiegeDuty)
				{
					// CHASSE l'armée ennemie : engage les défenseurs pour les réduire.
					switch (R)
					{
						case EUnitRole::Distance:
							Dest  = EnemyC - ToEnemyFromObj * 200.f; // canarde les défenseurs au large
							Layer = bCanLayer ? 1400.f : 0.f;
							break;
						case EUnitRole::Montee:
						case EUnitRole::Speciale:
							Dest  = EnemyC;                          // charge les défenseurs
							Layer = 0.f;
							break;
						default:
							Dest  = EnemyC;                          // mêlée / chef sur l'armée
							Layer = bCanLayer ? 300.f : 0.f;
							break;
					}
				}
				else
				switch (R)
				{
					case EUnitRole::Distance:
						// Canarde l'objectif à distance (reste au large de l'anneau).
						Dest  = ObjLoc - ToEnemyFromObj * 850.f;
						Layer = bCanLayer ? 1400.f : 0.f;
						break;
					case EUnitRole::Montee:
					case EUnitRole::Speciale:
					{
						// BRISEURS : chargent l'anneau défensif pour l'ouvrir. Cherchent
						// une ouverture en changeant de verticalité (haut/bas alterné).
						Dest  = ObjLoc;
						Layer = bCanLayer ? ((idx % 2 == 0) ? 1600.f : 0.f) : 0.f;
						break;
					}
					case EUnitRole::Infanterie:
						Dest  = ObjLoc; // siège au corps-à-corps
						Layer = 0.f;
						break;
					default: // Chef / Mythique : poussent sur l'objectif en hauteur
						Dest  = ObjLoc;
						Layer = bCanLayer ? 1200.f : 0.f;
						break;
				}
				// Anticipation du courant conservée plus bas.
				if (Cur && Cur->IsActive() && Layer > 500.f)
				{
					const FVector DirToDest = (Dest - U->GetActorLocation()).GetSafeNormal2D();
					if (FVector::DotProduct(Cur->GetDirection(), DirToDest) < -0.35f
						&& Cur->GetFactorAt(Layer) > 0.4f) Layer = 0.f;
				}
				if (DU) DU->SetDesiredZ(Layer);
				AIC->ActivateRTSBehavior();
				AIC->IssueOrder_AttackMove(Dest);
				if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
					St->SightRange = 60000.f;
				++idx;
				continue;
			}

			switch (R)
			{
				case EUnitRole::Infanterie:
				{
					const int32 c = infCol++;
					if (DU && DU->HasShield())
					{
						// ── MUR DE BOUCLIERS (TORTUE) : ligne SERRÉE et ANCRÉE AU SOL, placée
						// DEVANT les lignes arrière et FACE à l'ennemi. Reste au sol (couche 0)
						// car le blocage du bouclier est BIEN plus efficace ancré au sol qu'en
						// hauteur. Ligne centrée + 2e rang pour un bloc dense et impénétrable. ──
						const float Center = (float)(ShieldCount - 1) * 0.5f;
						const int32 Rank   = (c % 2);                    // 0 = 1er rang, 1 = 2e rang (tortue)
						const int32 Col    = c / 2;
						const float LateralOff = ((float)Col - Center * 0.5f) * 200.f
							+ (Rank ? 100.f : 0.f);                       // 2e rang décalé (couvre les jointures)
						const FVector WallLine = Front - Fwd * (Rank ? 130.f : 0.f); // 2e rang un peu en retrait
						Dest  = WallLine + Lateral * LateralOff;
						Layer = 0.f;                                     // TORTUE toujours ancrée au sol
					}
					else
					{
						// Infanterie SANS bouclier (ex. Noxeflare) : mur classique sur 2 couches.
						Dest  = Front + Lateral * ((float)(c / 2 - 2) * 240.f);
						Layer = (bCanLayer && (c % 2 == 1)) ? 800.f : 0.f;
					}
					break;
				}
				case EUnitRole::Distance:
				{
					// En RETRAIT derrière le mur + en HAUTEUR : canarde sans s'exposer.
					const int32 c = disCol++;
					const float Personality = DU ? DU->TacticalPersonality : 0.5f;
					const float BackDistance = FMath::Lerp(760.f, 1160.f, 1.f - Personality)
						+ TacticalVariant * 35.f;
					Dest  = Front - Fwd * BackDistance + Lateral * ((float)(c - 1) * 300.f);
					Layer = bCanLayer
						? FMath::Lerp(1200.f, 1900.f, Personality) : 0.f;
					break;
				}
				case EUnitRole::Montee:
				{
					// CHARGE hit-and-run (ancrée au sol) : fonce briser les lignes, puis
					// revient se repositionner (cycle temporel). Rapide = charge plus souvent.
					const float Speed = Data ? Data->Stats.MovementSpeed : 1.f;
					const float Cycle = FMath::Max(6.f, 12.f - Speed * 3.f); // rapide -> cycle court
					// HIVE-MIND : percée COORDONNÉE (toutes chargent ensemble), puis repli en
					// ligne à leur position -> on lit une vague de lances, pas des charges éparses.
					const bool  bCharge = FMath::Fmod(Now + TacticalPhaseOffset, Cycle) < 3.5f;
					const int32 c = monCol++;
					const float FlankSign = ((c + TacticalVariant) % 2 == 0) ? 1.f : -1.f;
					const FVector ChargeTarget = (TacticalVariant == 0)
						? EnemyC : EnemyC + Lateral * FlankSign * (320.f + TacticalVariant * 110.f);
					Dest  = bCharge ? ChargeTarget
						: (Front - Fwd * 250.f + Lateral * ((float)(c - 1) * 320.f));
					Layer = 0.f; // monture : sol / 1re couche uniquement
					break;
				}
				case EUnitRole::Chef:
					// Légèrement EN RETRAIT derrière le mur : survit assez pour placer sa
					// compétence (ex. Lame Photonique) au lieu de mourir en première ligne.
					Dest  = Front - Fwd * 400.f;
					Layer = bCanLayer ? 800.f : 0.f;
					break;
				default: // Mythique / Spéciale : avancent sur l'ennemi, en hauteur si possible
					Dest  = EnemyC;
					Layer = bCanLayer ? 1200.f : 0.f;
					break;
			}

			// ANTICIPATION DU COURANT : ne pas monter si un courant fort repousse du but.
			if (Cur && Cur->IsActive() && Layer > 500.f)
			{
				const FVector DirToDest = (Dest - U->GetActorLocation()).GetSafeNormal2D();
				const float Along = FVector::DotProduct(Cur->GetDirection(), DirToDest);
				if (Along < -0.35f && Cur->GetFactorAt(Layer) > 0.4f)
				{
					Layer = 0.f; // courant défavorable en hauteur -> passe par le bas
				}
			}

			if (DU) DU->SetDesiredZ(Layer);
			AIC->ActivateRTSBehavior();
			AIC->IssueOrder_AttackMove(Dest);
			if (UUnitAIStateComponent* St = U->FindComponentByClass<UUnitAIStateComponent>())
			{
				// MUR DE BOUCLIERS : les porte-boucliers TIENNENT la ligne (portée de vue
				// réduite) -> ils n'abandonnent pas la formation pour poursuivre au loin ;
				// ils n'engagent que les menaces au contact. Les autres gardent la vue large.
				St->SightRange = (DU && DU->HasShield()) ? 1100.f : 60000.f;
			}
			++idx;
		}
	};

	// Le joueur DÉFEND son objectif (Cristalliseur) ; le rival l'ASSAILLE.
	CommandArmy(CachedPlayerFaction, CachedRivalFaction, PlayerC, RivalC, /*bIsPlayer=*/true,  /*bDefendObj=*/true);
	CommandArmy(CachedRivalFaction, CachedPlayerFaction, RivalC, PlayerC, /*bIsPlayer=*/false, /*bDefendObj=*/false);
}

void AWOTOLDemoDirector::HandleCaptureDestroyed()
{
	if (bBattleConcluded) return;
	Say(FString::Printf(TEXT("Le %s est detruit — objectif perdu !"),
		*BuildingDisplayName(CachedPlayerFaction)));
	bBattleConcluded = true;
	GetWorldTimerManager().ClearTimer(BattleCheckHandle);
	GetWorldTimerManager().ClearTimer(SiegeHandle);
	GetWorldTimerManager().ClearTimer(TacticalHandle);

	// §17 (v0.8) : après la destruction du bâtiment défendu, la ZONE REDEVIENT NEUTRE.
	// Le Noxéen NE POSE PAS son propre bâtiment (§18) -> on retire juste la capture.
	if (CaptureObject)
	{
		if (UWorld* W = GetWorld())
			if (UTerritoryStateManager* Terr = W->GetSubsystem<UTerritoryStateManager>())
				Terr->NeutralizeZone(CaptureObject->ZoneID);
		if (UDemoFlowSubsystem* Demo = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr)
		{
			Demo->MarkZoneLost();
		}
	}

	OnPlayerDefeat();
}

void AWOTOLDemoDirector::StartRivalDefense()
{
	// Passe en phase 2 puis REPART EN PRÉPARATION : le joueur replace ses unités et
	// clique lui-même "Lancer la bataille" (comme la phase 1). Pas de lancement d'office.
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetPhase(EDemoPhase::Battle_Rival);
		}
	}
	BeginPreparation();
}

void AWOTOLDemoDirector::EndDemo(bool bPlayerWon)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->bDemoVictory = bPlayerWon;
			Demo->SetPhase(EDemoPhase::DemoEnd);
		}
	}

	if (bPlayerWon)
	{
		Say(TEXT("VICTOIRE — Zone tenue. Le conflit Aquiloris / Noxéens ne fait que commencer."));
	}
	else
	{
		Say(TEXT("DÉFAITE — Vos forces sont anéanties. Relancez la bataille pour réessayer."));
	}
}

void AWOTOLDemoDirector::FocusCameraOnPlayer()
{
	UWorld* W = GetWorld();
	if (!W) return;
	const FVector PlayerOrigin = GetActorLocation() + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
	const FVector Focus = PlayerOrigin + FVector(700.f, 0.f, 150.f);
	for (TActorIterator<AWOTOLBattleCamera> It(W); It; ++It)
	{
		It->SetInitialView(Focus, 0.f, -45.f, 2600.f); // yaw 0 = regard vers l'ennemi (+X)
		break;
	}
}

void AWOTOLDemoDirector::SpawnPlacementBoundary()
{
	ClearPlacementBoundary();
	UWorld* W = GetWorld();
	if (!W) return;

	const FLinearColor Col = FFactionColors::Get(CachedPlayerFaction); // bleu / vert selon faction
	const float BX = GetPlacementBoundaryWorldX();
	const float CY = GetActorLocation().Y;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

	// LIGNE DE LIMITE DE PLACEMENT : un BARREAU LUMINEUX qui court sur TOUTE la largeur de la
	// carte (en Y), SURÉLEVÉ et ÉPAIS pour rester visible MÊME AU CENTRE (au-dessus du tapis
	// de combat surélevé qui masquait un simple trait au sol) + ÉMISSIF (bloom) à la couleur du
	// camp. Repère clair : « voilà la limite de mon premier tiers ».
	const float HalfLen = 7200.f;           // demi-longueur -> couvre toute la largeur jouable
	const float BarZ    = 90.f;             // surélevé : passe AU-DESSUS du tapis central
	const float BarH    = 180.f;            // hauteur du barreau (bien visible de profil)

	FActorSpawnParameters P; P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Line = W->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), FVector(BX, CY, BarZ), FRotator::ZeroRotator, P);
	if (Line)
	{
		if (UStaticMeshComponent* C = Line->GetStaticMeshComponent())
		{
			C->SetMobility(EComponentMobility::Movable);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCanEverAffectNavigation(false);
			if (Cube) C->SetStaticMesh(Cube);
			// Épais (X), TRÈS long (Y = toute la largeur), haut (Z) = barre lumineuse verticale.
			Line->SetActorScale3D(FVector(0.5f, HalfLen * 2.f / 100.f, BarH / 100.f));
			// ÉMISSIF survolté (>1.2) -> déclenche le BLOOM : la barre RAYONNE à la couleur du camp.
			if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(Line,
					FLinearColor(Col.R * 2.6f + 0.3f, Col.G * 2.6f + 0.3f, Col.B * 2.6f + 0.3f, 1.f)))
				C->SetMaterial(0, MID);
		}
		PlacementMarkers.Add(Line);
	}

	// LAMPES réparties le long du trait -> halo lumineux continu sur toute la largeur, la
	// limite se DISTINGUE nettement même au centre et dans l'ambiance sombre du fond.
	const FLinearColor LCol(FMath::Min(1.f, Col.R + 0.25f), FMath::Min(1.f, Col.G + 0.25f), FMath::Min(1.f, Col.B + 0.25f));
	const int32 Lamps = 9;
	for (int32 i = 0; i < Lamps; ++i)
	{
		const float y = CY - HalfLen + (2.f * HalfLen) * (i / float(Lamps - 1));
		AActor* LampA = W->SpawnActor<AActor>(AActor::StaticClass(), FVector(BX, y, BarZ + 40.f), FRotator::ZeroRotator, P);
		if (!LampA) continue;
		USceneComponent* Root = NewObject<USceneComponent>(LampA);
		Root->RegisterComponent(); LampA->SetRootComponent(Root);
		if (UPointLightComponent* PL = NewObject<UPointLightComponent>(LampA))
		{
			PL->SetupAttachment(Root); PL->RegisterComponent();
			PL->SetLightColor(LCol);
			PL->SetIntensity(2600.f);
			PL->SetAttenuationRadius(1400.f);
			PL->SetCastShadows(false);
		}
		PlacementMarkers.Add(LampA);
	}
}

void AWOTOLDemoDirector::SpawnCoverStructures()
{
	ClearCoverStructures();
	UWorld* W = GetWorld();
	if (!W) return;
	const FVector C = GetActorLocation();

	// Quelques ruines Éthériennes réparties AUTOUR DU CENTRE (là où l'action se concentre).
	// Mix : un grand pilier INDESTRUCTIBLE (couverture fiable) + des ruines DESTRUCTIBLES.
	struct FCover { FVector Off; int32 Variant; bool bIndestructible; float HP; };
	// NB : on évite le centre exact (0,0) — l'objet de capture (phase 2) y est posé.
	const FCover Layout[] = {
		{ FVector(  650.f,  650.f, 0.f), 0, true,  0.f    }, // grand pilier : INDESTRUCTIBLE
		{ FVector(  850.f, -1200.f, 0.f), 1, false, 1100.f}, // pan de mur (destructible)
		{ FVector(-1000.f, -900.f, 0.f), 2, false, 1400.f}, // arche brisée (destructible)
		{ FVector(-1100.f, 1100.f, 0.f), 0, false, 1300.f}, // pilier (destructible)
		// Éléments SUPPLÉMENTAIRES répartis plus large : surtout DESTRUCTIBLES (couverture
		// que le joueur/l'IA peut faire tomber sur l'ennemi), + rares indestructibles.
		{ FVector( 2100.f, -300.f, 0.f), 1, false, 1200.f}, // pan de mur (destructible)
		{ FVector(-2200.f,  400.f, 0.f), 2, false, 1400.f}, // arche (destructible)
		{ FVector(  300.f, 2200.f, 0.f), 0, false, 1200.f}, // pilier (destructible)
		{ FVector( -400.f,-2200.f, 0.f), 1, false, 1500.f}, // pan de mur (destructible)
		{ FVector( 1700.f, 1700.f, 0.f), 2, false, 1300.f}, // arche (destructible)
		{ FVector(-1800.f,-1700.f, 0.f), 0, false, 1300.f}, // pilier (destructible)
		{ FVector( 2300.f, 1400.f, 0.f), 0, false, 1100.f}, // pilier (destructible)
		{ FVector(-1500.f, 2100.f, 0.f), 1, false, 1400.f}, // pan de mur (destructible)
	};
	// PHASE 3 : décor DIFFÉRENT — bouts de BÂTIMENTS en ruine (var. 3), DÉCOMBRES (var. 4),
	// DALLES penchées (var. 5) + murs/arches, répartis LARGE sur l'arène agrandie.
	const FCover LayoutGrand[] = {
		{ FVector( 1000.f,  1200.f, 0.f), 3, false, 1500.f}, // fragment de bâtiment
		{ FVector(-1200.f,  -900.f, 0.f), 3, false, 1500.f}, // fragment de bâtiment
		{ FVector(  900.f, -1400.f, 0.f), 5, false, 1300.f}, // dalle penchée
		{ FVector(-1400.f,  1300.f, 0.f), 5, false, 1300.f}, // dalle penchée
		{ FVector(  200.f,  2600.f, 0.f), 4, false,  900.f}, // décombres
		{ FVector( -300.f, -2600.f, 0.f), 4, false,  900.f}, // décombres
		{ FVector( 2900.f,   400.f, 0.f), 1, false, 1400.f}, // pan de mur
		{ FVector(-2900.f,  -500.f, 0.f), 2, false, 1500.f}, // arche
		{ FVector( 2600.f, -1800.f, 0.f), 3, false, 1500.f}, // fragment de bâtiment
		{ FVector(-2600.f,  1800.f, 0.f), 5, false, 1300.f}, // dalle penchée
		{ FVector( 1800.f,  2400.f, 0.f), 4, false,  900.f}, // décombres
		{ FVector(-1800.f, -2400.f, 0.f), 1, false, 1400.f}, // pan de mur
		{ FVector(  700.f,   700.f, 0.f), 0, true,   0.f   }, // pilier INDESTRUCTIBLE (repère)
		{ FVector( 3300.f,  2100.f, 0.f), 2, false, 1500.f}, // arche
		{ FVector(-3300.f, -2100.f, 0.f), 3, false, 1500.f}, // fragment de bâtiment
		{ FVector(    0.f,  3300.f, 0.f), 5, false, 1300.f}, // dalle penchée
	};
	const FCover* Use = bGrandBattle ? LayoutGrand : Layout;
	const int32 UseN  = bGrandBattle ? UE_ARRAY_COUNT(LayoutGrand) : UE_ARRAY_COUNT(Layout);
	for (int32 li = 0; li < UseN; ++li)
	{
		const FCover& S = Use[li];
		const FTransform TM(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), C + S.Off);
		AWOTOLCoverStructure* Cov = W->SpawnActorDeferred<AWOTOLCoverStructure>(
			AWOTOLCoverStructure::StaticClass(), TM, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Cov) continue;
		Cov->Variant = S.Variant;                      // AVANT BeginPlay -> bonne forme
		Cov->bIndestructible = S.bIndestructible;
		if (S.HP > 0.f) { Cov->MaxHealth = S.HP; }
		UGameplayStatics::FinishSpawningActor(Cov, TM);
		CoverStructures.Add(Cov);
	}
}

void AWOTOLDemoDirector::ClearCoverStructures()
{
	for (TObjectPtr<AWOTOLCoverStructure>& C : CoverStructures)
		if (C) C->Destroy();
	CoverStructures.Empty();
}

// (Phase 2) CRISTAUX DE TERRAFORMATION : amas de cristaux disséminés sur toute la carte,
// nés de la pose du Cristalliseur -> marquent la ZONE ACQUISE par le joueur (avantage
// défensif). Purement décoratifs (la collision reste légère), lumineux à la couleur du camp.
void AWOTOLDemoDirector::SpawnZoneCrystals()
{
	ClearZoneCrystals();
	UWorld* W = GetWorld();
	if (!W) return;
	const FVector C = GetActorLocation();

	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	// (Cristaux désormais ÉMISSIFS via WOTOLGlow -> plus besoin du BasicShapeMaterial ici.)
	// Couleur du camp qui possède la zone (le joueur défenseur).
	const FLinearColor Col = FFactionColors::Get(CachedPlayerFaction);

	const int32 Clusters = 14;
	for (int32 i = 0; i < Clusters; ++i)
	{
		// Répartis sur toute la carte, en évitant le centre exact (objectif) + un peu d'aléa.
		const float ang = FMath::FRandRange(0.f, 2.f * PI);
		const float rad = FMath::FRandRange(900.f, 3800.f);
		const FVector Base = C + FVector(FMath::Cos(ang) * rad, FMath::Sin(ang) * rad, 0.f);

		FActorSpawnParameters P; P.Owner = this;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Cluster = W->SpawnActor<AActor>(AActor::StaticClass(), Base, FRotator::ZeroRotator, P);
		if (!Cluster) continue;
		USceneComponent* Root = NewObject<USceneComponent>(Cluster);
		Root->RegisterComponent();
		Cluster->SetRootComponent(Root);

		// 3-5 pointes de cristal de tailles variées (amas).
		const int32 Shards = FMath::RandRange(3, 5);
		for (int32 s = 0; s < Shards; ++s)
		{
			UStaticMeshComponent* M = NewObject<UStaticMeshComponent>(Cluster);
			if (!M) continue;
			M->SetupAttachment(Root);
			M->RegisterComponent();
			if (Cone) M->SetStaticMesh(Cone);
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			M->SetCanEverAffectNavigation(false);
			const float h = FMath::FRandRange(1.2f, 3.2f);
			const float w = FMath::FRandRange(0.25f, 0.5f);
			M->SetRelativeScale3D(FVector(w, w, h));
			M->SetRelativeLocation(FVector(FMath::FRandRange(-80.f, 80.f), FMath::FRandRange(-80.f, 80.f), 0.f));
			M->SetRelativeRotation(FRotator(FMath::FRandRange(-12.f, 12.f), 0.f, FMath::FRandRange(-12.f, 12.f)));
			// CRISTAUX ÉMISSIFS = énergie BLEUE qui RAYONNE (c'est le cristal qui brille,
			// pas le sol autour).
			if (UMaterialInstanceDynamic* MID = WOTOLGlow::MakeGlow(Cluster, FLinearColor(0.4f * 3.f, 1.2f * 3.f, 2.4f * 3.f, 1.f)))
				M->SetMaterial(0, MID);
		}
		// Lumière BLEUE accrochée à l'amas (le cristal illumine son environnement).
		if (UPointLightComponent* PC = NewObject<UPointLightComponent>(Cluster))
		{
			PC->SetupAttachment(Root); PC->RegisterComponent();
			PC->SetRelativeLocation(FVector(0, 0, 140.f));
			PC->SetLightColor(FLinearColor(0.30f, 0.65f, 1.0f));
			PC->SetIntensity(1600.f);
			PC->SetAttenuationRadius(420.f);
			PC->SetCastShadows(false);
		}
		ZoneCrystals.Add(Cluster);
	}
}

void AWOTOLDemoDirector::ClearZoneCrystals()
{
	for (TObjectPtr<AActor>& A : ZoneCrystals)
		if (A) A->Destroy();
	ZoneCrystals.Empty();
}

void AWOTOLDemoDirector::ClearPlacementBoundary()
{
	for (TObjectPtr<AActor>& M : PlacementMarkers)
	{
		if (M) M->Destroy();
	}
	PlacementMarkers.Empty();
}

void AWOTOLDemoDirector::Say(const FString& Message)
{
	OnDemoMessage.Broadcast(Message);

	// Publie au HUD (affichage centré)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetMessage(Message);
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan, Message);
	}
}
