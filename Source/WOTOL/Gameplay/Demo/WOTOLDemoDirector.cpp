#include "WOTOLDemoDirector.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLGlow.h"
#include "OceanCurrentSubsystem.h"
#include "WOTOLCoverStructure.h"
#include "WOTOLCurrentField.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Gameplay/Battle/UnitSelectionManager.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/Battle/WOTOLBattleCamera.h"
#include "Core/FactionRegistrySubsystem.h"
#include "EngineUtils.h"
#include "WOTOLGreyboxEnvironment.h"
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

// NORMALISATEUR DE PUISSANCE PAR FACTION : dans le miroir 80v80 (phase 3), les Noxéens
// (DPS + compétences AoE plus fortes) ecrasaient les Aquiloris 80-0 QUEL QUE SOIT le camp du
// joueur -> pur desequilibre de faction. On rehausse les degats AQUILORIS (plus lents/tanky)
// pour rapprocher les deux factions d'une puissance de combat equivalente. S'applique a toute
// unite Aquiloris (joueur comme rivale), toutes phases.
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
// Levier unique de difficulté = k (appliqué À L'ENNEMI seulement, sur PV ET dégâts) :
//     R = 1 / k²   ->   k = 1/√R.
// Le joueur, lui, est INDÉPENDANT de la difficulté (baseline stable) : on ne fait que
// renforcer/affaiblir l'ADVERSAIRE. Monotone par construction (kFacile < kNormal < kDifficile).
static float EnemyDiffK(EDemoDifficulty D)
{
	switch (D)
	{
		case EDemoDifficulty::Facile:    return 0.72f; // ennemi affaibli -> R≈1.93 (facile)
		case EDemoDifficulty::Difficile: return 0.95f; // ennemi presque a parite -> R≈1.11 (dur)
		default:                         return 0.845f; // Normal -> R≈1.40
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
		}
	}

	// MUSIQUE PILOTÉE PAR L'ÉCRAN : on sonde l'écran courant ~4 fois/s et on change de piste
	// UNIQUEMENT quand la musique cible change (menu -> bataille -> résumé). Léger et robuste.
	UpdateMusicForScreen();
	GetWorldTimerManager().SetTimer(MusicPollHandle, this,
		&AWOTOLDemoDirector::UpdateMusicForScreen, 0.25f, /*bLoop=*/true);
}

uint8 AWOTOLDemoDirector::MusicCatForScreen(uint8 Screen) const
{
	switch (static_cast<EDemoScreen>(Screen))
	{
	case EDemoScreen::MainMenu:
	case EDemoScreen::FactionSelect: return 1; // menu
	case EDemoScreen::Prepare:
	case EDemoScreen::Playing:       return 2; // bataille
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

// Monte les armées en PRÉPARATION (placement libre), SANS lancer le combat.
// Fonctionne pour LES DEUX phases (créature ou défense rivale) selon la phase courante.
void AWOTOLDemoDirector::BeginPreparation()
{
	// (La musique est gérée automatiquement par l'écran -> BattleMusic dès l'écran Prepare.)
	CachedPlayerFaction = ResolvePlayerFaction();
	CachedRivalFaction  = RivalOf(CachedPlayerFaction);

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	// Première prépa (menu) : on est sur la phase créature.
	if (Demo && Demo->GetPhase() == EDemoPhase::None)
	{
		Demo->SetPhase(EDemoPhase::Battle_Creature);
	}
	const EBattleType BT = Demo ? Demo->GetCurrentBattleType() : EBattleType::CreatureEncounter;
	const bool bGrand = Demo && Demo->GetPhase() == EDemoPhase::Battle_Grand; // phase 3, zone neutre

	// Phase 2 (défense rivale) : GRANDE bataille — plus d'unités des deux côtés.
	// Valeurs volontairement mesurées : ~26 vs 26 unités entièrement riggées, pour
	// rester fluide/stable sur un portable (évite les surcharges mémoire/GPU).
	bGrandBattle = bGrand;
	if (bGrand)
	{
		// PHASE 3 — 80 UNITÉS AU TOTAL par faction : moins d'Aquiloryons, plus d'Aquilombres.
		//   1 chef + 26 inf + 18 montées + 24 distance + 10 spéciales + 1 mythique = 80.
		// Armée plus RÉSISTANTE -> la bataille DURE (~15 min). Formation ÉTALÉE.
		InfantryCount = 26; MountedCount = 18; RangedCount = 24; SpecialCount = 10;
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
		Demo->SetObjective(bGrand
			? FString(TEXT("PHASE 3 — Mettez la faction rivale en DEROUTE"))
			: (BT == EBattleType::RivalDefense
				? FString::Printf(TEXT("Proteger le %s — ne le laissez pas tomber a 0"),
					*BuildingDisplayName(CachedPlayerFaction))
				: FString(TEXT("Vaincre la creature — le KRAKEN"))));
	}
	Say(TEXT("PREPARATION : placez vos unites dans VOTRE zone (barriere coloree), puis lancez."));
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
		* PhaseEvoHP(Demo->GetPhase());

	// Place un groupe en rangées (se replie sur plusieurs lignes vers l'arrière -X). En
	// phase 3, les rangées sont bien plus LARGES -> la ligne s'étale sur la largeur du tiers
	// (fini l'empilement). Toutes les unités reçoivent l'échelle de PV de la bataille.
	// Curseur de PROFONDEUR partagé : chaque CATÉGORIE occupe SA/SES propre(s) rangée(s) et
	// on avance le curseur du NOMBRE RÉEL de rangées qu'elle utilise + un espace de séparation.
	// => JAMAIS deux types d'unités différents sur la même ligne (fini le chevauchement des
	// Aquilances sur la rangée des Aquisphères vu en phase 2).
	float BackCursor = 0.f;                                  // profondeur (en -X) de la prochaine catégorie
	const float GroupGap = Depth * (bGrandBattle ? 0.5f : 1.0f); // couloir vide entre deux catégories

	// DÉPLOIEMENT PAR BLOCS DE ~5 (style Total War) : chaque catégorie est découpée en petits
	// groupes de 5 qui forment un mini-carré (2-1-2, l'unité CENTRALE porte l'étiquette) ou une
	// LIGNE (si <5). Les blocs sont TUILÉS sur la LARGEUR (Y) puis sur la PROFONDEUR (X) -> des
	// groupes bien distincts, espacés, sur la largeur ET la longueur de la zone de placement.
	const float IntraY = Lat * 0.6f;      // écart latéral DANS un bloc
	const float IntraX = Depth * 0.55f;   // écart de profondeur DANS un bloc
	const float BlockStepY = Lat * 2.4f;  // pas entre blocs (Y)
	const float BlockStepX = Depth * 2.4f;// pas entre bandes de blocs (X)
	auto PlaceBlocks = [&](FName Id, int32 Count, int32 BlocksPerBand)
	{
		if (Id.IsNone() || Count <= 0) return;
		BlocksPerBand = FMath::Max(1, BlocksPerBand);
		const int32 NumBlocks = (Count + 4) / 5;
		int32 BandsUsed = 0;
		for (int32 b = 0; b < NumBlocks; ++b)
		{
			const int32 Band = b / BlocksPerBand, ColB = b % BlocksPerBand;
			BandsUsed = FMath::Max(BandsUsed, Band + 1);
			const int32 InThisBand = FMath::Min(BlocksPerBand, NumBlocks - Band * BlocksPerBand);
			const float BlockY = (ColB - (InThisBand - 1) * 0.5f) * BlockStepY;
			const float BlockX = -BackCursor - Band * BlockStepX;
			const int32 N = FMath::Min(5, Count - b * 5);
			const int32 Gid = NextFormationGroupId++;
			for (int32 s = 0; s < N; ++s)
			{
				FVector2D Slot; bool bCenter = false;
				if (N == 5)
				{
					// Carré 2-1-2 : 2 devant, 1 au centre (étiquette), 2 derrière.
					switch (s)
					{
						case 0: Slot = FVector2D( IntraX, -IntraY); break; // avant-gauche
						case 1: Slot = FVector2D( IntraX,  IntraY); break; // avant-droite
						case 2: Slot = FVector2D( 0.f,    0.f);     bCenter = true; break; // centre
						case 3: Slot = FVector2D(-IntraX, -IntraY); break; // arrière-gauche
						default:Slot = FVector2D(-IntraX,  IntraY); break; // arrière-droite
					}
				}
				else
				{
					// LIGNE centrée (blocs incomplets) ; l'unité du milieu porte l'étiquette.
					Slot = FVector2D(0.f, (s - (N - 1) * 0.5f) * IntraY * 1.6f);
					bCenter = (s == N / 2);
				}
				const FVector Loc = Origin + FVector(BlockX + Slot.X, BlockY + Slot.Y, GroundZ);
				if (AWOTOLDemoUnit* U = SpawnUnit(Id, Loc, Facing, 1.f, PScale))
				{
					U->SetFormation(Gid, Slot, bCenter);
				}
			}
		}
		BackCursor += BandsUsed * BlockStepX + GroupGap; // réserve la place de CETTE catégorie
	};

	// Nombre de blocs alignés sur la LARGEUR avant de passer à la bande suivante (profondeur).
	const int32 BPBinf = bGrandBattle ? 6 : 3;
	const int32 BPBmon = bGrandBattle ? 4 : 2;
	const int32 BPBdis = bGrandBattle ? 5 : 3;
	const int32 BPBspe = bGrandBattle ? 3 : 2;

	// Chef en pointe (devant l'infanterie, centré) — son propre "groupe" solo.
	if (AWOTOLDemoUnit* Chef = SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Chef),
			Origin + FVector(Depth, 0.f, GroundZ), Facing, 1.f, PScale))
	{
		Chef->SetFormation(NextFormationGroupId++, FVector2D::ZeroVector, true);
	}

	// Blocs empilés de l'avant vers l'arrière, chaque catégorie sur ses propres bandes.
	PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie), InfantryCount, BPBinf);
	PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Montee), MountedCount, BPBmon);
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Distance), RangedCount, BPBdis);
	}
	// PHASE 3 : SPÉCIALE (arrière-ligne) + MYTHIQUE (soutien) débloquées.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Speciale))
	{
		PlaceBlocks(Demo->GetUnitID(Faction, EDemoUnitCategory::Speciale), SpecialCount, BPBspe);
	}
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Mythique))
	{
		// ScaleBoost = 1.0 (le mythique est déjà grand ; le gonfler bloquait sa capsule).
		// Placé DERRIÈRE la dernière catégorie via le curseur -> bien DANS l'arène, jamais
		// sur une rangée occupée ni enterré dans les montagnes.
		SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Mythique),
			Origin + FVector(-BackCursor, 0.f, GroundZ), Facing, /*ScaleBoost=*/1.0f, /*HealthScale=*/3.0f * FactionSurvivability(Faction));
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
	const float KrakenHP = CreatureHealthScale * (Demo ? EnemyDiffK(Demo->GetDifficulty()) : 0.845f);
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
		* PhaseEvoHP(Demo->GetPhase()) * EnemyDiffK(Demo->GetDifficulty());

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
	auto PlaceRows = [&](FName Id, EDemoUnitCategory Cat, int32 Count, int32 PerRow)
	{
		if (Id.IsNone() || Count <= 0 || PerRow <= 0) return;
		const int32 Rows = (Count + PerRow - 1) / PerRow;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow;
			const int32 Col = i % PerRow;
			const int32 InThisRow = FMath::Min(PerRow, Count - Row * PerRow);
			const float Y = (Col - (InThisRow - 1) * 0.5f) * Lat;
			FVector Loc = O + FVector(BackCursor + Row * Depth, Y, 100.f);
			Loc.X = FMath::Max(Loc.X, MirrorX); // ne pas franchir la limite de son tiers
			SetLayer(SpawnUnit(Id, Loc, Facing, 1.f, RivalScale), PickLayer(Cat));
		}
		BackCursor += Rows * Depth + GroupGap;
	};
	const int32 PRinf = bGrandBattle ? 18 : 8;
	const int32 PRmon = bGrandBattle ? 12 : 6;
	const int32 PRdis = bGrandBattle ? 16 : 8;
	const int32 PRspe = bGrandBattle ? 6  : 3;

	FVector ChefLoc = O + FVector(-Depth, 0.f, 100.f);
	ChefLoc.X = FMath::Max(ChefLoc.X, MirrorX);
	SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef), ChefLoc, Facing, 1.f, RivalScale),
		PickLayer(EDemoUnitCategory::Chef));

	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Infanterie), EDemoUnitCategory::Infanterie, InfantryCount, PRinf);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Montee), EDemoUnitCategory::Montee, MountedCount, PRmon);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Distance), EDemoUnitCategory::Distance, RangedCount, PRdis);
	// PHASE 3 : la rivale déploie AUSSI sa spéciale + son mythique.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Speciale))
	{
		PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Speciale), EDemoUnitCategory::Speciale, SpecialCount, PRspe);
	}
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Mythique))
	{
		FVector MLoc = O + FVector(BackCursor, 0.f, 100.f);
		MLoc.X = FMath::Max(MLoc.X, MirrorX);
		SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Mythique), MLoc, Facing, /*ScaleBoost=*/1.0f, RivalScale),
			PickLayer(EDemoUnitCategory::Mythique));
	}
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
		const float MaxR = 4200.f; // marge devant le mur de montagnes (~4700)
		if (Flat.Size() > MaxR) SafeLoc = Ctr + Flat.GetSafeNormal() * MaxR + FVector(0.f, 0.f, SafeLoc.Z - Ctr.Z);
	}

	const FTransform SpawnTM(Facing, SafeLoc, FVector(ScaleBoost));

	AWOTOLDemoUnit* Unit = GetWorld()->SpawnActorDeferred<AWOTOLDemoUnit>(
		DemoUnitClass, SpawnTM, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Unit) return nullptr;

	Unit->UnitData    = Data;
	Unit->HealthScale = HealthScale;   // appliqué dans BeginPlay (avant FinishSpawning)
	Unit->bIsBoss     = bAsBoss;       // AVANT FinishSpawning -> silhouette Kraken forcée
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
		float M = FactionDamage(Unit->GetFaction()) * PhaseEvoDMG(Phase);
		if (!bPlayerSide) M *= EnemyDiffK(Diff);
		Unit->BalanceDamageMult = M;
	}

	SpawnedUnits.Add(Unit);
	return Unit;
}

void AWOTOLDemoDirector::LaunchBattle()
{
	// (Musique geree par l'ecran : BattleMusic continue de Prepare a Playing, sans coupure.)
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		// Phase 3 (grande bataille) : chrono ÉTENDU à 15 min (900 s) ; sinon 10 min.
		const bool bGrand = GetGameInstance() && GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()
			&& GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()->GetPhase() == EDemoPhase::Battle_Grand;
		RTS->StartBattlePhase(bGrand ? 900.f : 600.f);
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
				if (CaptureObject != nullptr)
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
			if (CaptureObject == nullptr) // uniquement la bataille de créature
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
							? EnemyDiffK(GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>()->GetDifficulty()) : 0.845f;
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
		const bool bSiege = CaptureObject != nullptr;
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
	if (CaptureObject)
	{
		// ── ÉQUILIBRAGE ASYMÉTRIQUE de l'objectif selon l'ATTAQUANT ──
		// Les Noxéens attaquants (gros DPS) écrasaient la défense Aquiloris et détruisaient
		// le Cristalliseur. Les Aquiloris attaquants laissaient le Noxéen défenseur gagner
		// (cas "parfait" -> on n'y touche PAS). On RENFORCE donc l'objectif UNIQUEMENT quand
		// l'attaquant est Noxéen, pour que la défense Aquiloris soit tenable jusqu'au chrono.
		const float ObjHP = (CachedRivalFaction == EFactionID::Noxeens) ? 26000.f : 16000.f;
		CaptureObject->MaxHealth     = ObjHP;
		CaptureObject->CurrentHealth = ObjHP;
		GetWorldTimerManager().SetTimer(
			SiegeHandle, this, &AWOTOLDemoDirector::SiegeTick, 1.f, true);
	}

	// Cerveau tactique : ré-évalue les manœuvres des 2 armées toutes les 3,5 s.
	GetWorldTimerManager().SetTimer(
		TacticalHandle, this, &AWOTOLDemoDirector::TacticalTick, 2.0f, true, 2.0f);

	BattleStartTime = GetWorld()->GetTimeSeconds(); // pour la durée du résumé
}

void AWOTOLDemoDirector::CheckBattleEnd()
{
	if (bBattleConcluded) return;

	const int32 PlayerAlive = CountAlive(CachedPlayerFaction);
	const int32 EnemyAlive  = CountAlive(CachedRivalFaction);

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
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->EndBattle(CachedPlayerFaction, EBattleResult::Victory);
	}

	const EDemoPhase Phase = Demo ? Demo->GetPhase() : EDemoPhase::None;

	if (Phase == EDemoPhase::Battle_Creature)
	{
		// Résumé INTERMÉDIAIRE (pertes de la bataille du Kraken), puis bouton "Continuer".
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		BuildBattleSummary(true, /*bFinal=*/false, TEXT("KRAKEN VAINCU"));
		if (Demo) Demo->SetScreen(EDemoScreen::Summary);
		Say(TEXT("Le Kraken est vaincu ! Consultez le resume, puis lancez la defense."));
	}
	else if (Phase == EDemoPhase::Battle_Rival)
	{
		// Le bâtiment a tenu : on le remet à neuf (réparation post-bataille).
		if (CaptureObject)
		{
			CaptureObject->Repair(CaptureObject->MaxHealth);
		}
		// Résumé INTERMÉDIAIRE (bFinal=false -> bouton « Continuer ») : la démo enchaîne sur
		// la PHASE 3 (grande bataille en zone neutre) au lieu de se terminer ici.
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		BuildBattleSummary(true, /*bFinal=*/false, TEXT("VICTOIRE — LA FACTION RIVALE RECULE"));
		if (Demo) Demo->SetScreen(EDemoScreen::Summary);
		Say(TEXT("La rivale est repoussee. Des heures plus tard, elle revient en force sur un autre terrain..."));
	}
	else if (Phase == EDemoPhase::Battle_Grand)
	{
		// Fin de la PHASE 3 : résumé FINAL de démo (victoire) -> Rejouer / Changer de faction.
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		BuildBattleSummary(true, /*bFinal=*/true, TEXT("VICTOIRE TOTALE"));
		if (Demo)
		{
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
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->EndBattle(CachedRivalFaction, EBattleResult::Defeat);
	}
	GetWorldTimerManager().ClearTimer(BattleCheckHandle);
	BuildBattleSummary(false, /*bFinal=*/true, TEXT("DEFAITE"));
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			// MÉMORISE la phase perdue AVANT de basculer sur DemoEnd -> « Rejouer » la relance.
			ReplayPhase = Demo->GetPhase();
			Demo->bDemoVictory = false;
			Demo->SetPhase(EDemoPhase::DemoEnd);
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
	Demo->SummaryDurationSeconds = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - BattleStartTime);
}

// Écran de TRANSITION narrative (hors-champ) — appelé depuis le bouton du résumé phase 1.
// Raconte ce qui s'est passé entre les deux batailles et le déblocage de la distance,
// avec les NOMS propres à la faction jouée (œuf du Cœur-Éclat -> cité -> nouveau bâtiment).
void AWOTOLDemoDirector::ShowInterlude()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

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
	// mène à la PHASE 3 (grande bataille neutre) au lieu de re-lancer la phase 2.
	if (Demo && Demo->GetPhase() == EDemoPhase::Battle_Rival)
	{
		StartGrandBattle();
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

// PHASE 3 — grande bataille rangée en ZONE NEUTRE : on débloque TOUT le roster (spéciale +
// mythique), on RETIRE l'objectif (pas de Cristalliseur -> pas d'avantage de zone : les
// deux camps n'ont que leurs bonus de faction), et on AGRANDIT l'arène (bataille épique).
void AWOTOLDemoDirector::StartGrandBattle()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (Demo) Demo->UnlockAll(); // spéciale (Aquilombres/Noxéons) + mythique (Léviaphénix/Noxedrake)

	// ZONE NEUTRE : plus aucun objet de capture -> le bloc d'avantage de zone (gaté sur
	// CaptureObject) est automatiquement ignoré. Bataille purement arme contre arme.
	if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
	if (Demo) Demo->SetCaptureObject(nullptr);
	ClearZoneCrystals();

	// Arène un peu plus large que la phase 2 MAIS qui tient DANS l'enceinte de montagnes
	// (mur de collision ~4700) : au-delà, les unités du fond spawnaient DANS les montagnes et
	// restaient bloquées (mythique enterré/invisible). 5000 + placement rapproché = OK.
	ArmySeparation = 5000.f;
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
	CleanupUnits();
	ClearPlacementBoundary();
	ClearCoverStructures();
	if (CaptureObject) { CaptureObject->Destroy(); CaptureObject = nullptr; }
	bBattleConcluded = false;
	// Roster + arène de phase 1 (les valeurs phase 2/3 sont réappliquées à leur lancement)
	InfantryCount = 10; MountedCount = 5; RangedCount = 5;
	ArmySeparation = 4500.f; // réinitialise l'arène (la phase 3 l'agrandit à 7000)
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
	CleanupUnits();
	ClearPlacementBoundary();
	ClearCoverStructures();
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
	for (TObjectPtr<AWOTOLDemoUnit>& U : SpawnedUnits)
	{
		if (U) U->Destroy();
	}
	SpawnedUnits.Empty();
}

void AWOTOLDemoDirector::SpawnCaptureObject(EFactionID Faction)
{
	if (!CaptureObjectClass) return;

	const FTransform TM(FRotator::ZeroRotator, GetActorLocation() + FVector(0.f, 0.f, 200.f));
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
}

// SIÈGE : périodiquement, chaque unité rivale proche du bâtiment lui inflige des dégâts
// -> la barre de vie du bâtiment descend en temps réel. Le joueur doit tuer/écarter les
// assiégeants avant qu'il ne tombe à 0.
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
						const float SideSign = (c % 2 == 0) ? 1.f : -1.f;
						const bool  bRear    = (c % 3 == 0); // un tiers tente le contournement arrière
						const float Speed = Data ? Data->Stats.MovementSpeed : 1.f;
						const float Cycle = FMath::Max(5.f, 11.f - Speed * 3.f);
						const bool  bCharge = FMath::Fmod(Now + idx * 1.3f, Cycle) < 4.f;
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
						const bool  bCharge = FMath::Fmod(Now, Cycle) < 4.f;
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
				const bool bSiegeDuty = ((idx % 20) < 9); // ~45% siège / ~55% chasse l'armée
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
					Dest  = Front - Fwd * 950.f + Lateral * ((float)(c - 1) * 300.f);
					Layer = bCanLayer ? 1600.f : 0.f;
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
					const bool  bCharge = FMath::Fmod(Now, Cycle) < 3.5f;
					const int32 c = monCol++;
					Dest  = bCharge ? EnemyC : (Front - Fwd * 250.f + Lateral * ((float)(c - 1) * 320.f));
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
