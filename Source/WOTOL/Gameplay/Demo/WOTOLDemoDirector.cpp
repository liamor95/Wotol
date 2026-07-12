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
	// automatiquement un son portant le bon NOM dans le dossier Content/Audio.
	// -> il suffit d'importer tes musiques dans Content/Audio et de les nommer
	//    exactement : PreparationMusic, BattleMusic, VictoryMusic, DefeatMusic.
	auto TryLoadMusic = [](TObjectPtr<USoundBase>& Slot, const TCHAR* AssetName)
	{
		if (Slot) return; // déjà assigné dans l'éditeur -> on n'écrase pas
		const FString Path = FString::Printf(TEXT("/Game/Audio/%s.%s"), AssetName, AssetName);
		Slot = LoadObject<USoundBase>(nullptr, *Path);
	};
	TryLoadMusic(PreparationMusic, TEXT("PreparationMusic"));
	TryLoadMusic(BattleMusic,      TEXT("BattleMusic"));
	TryLoadMusic(VictoryMusic,     TEXT("VictoryMusic"));
	TryLoadMusic(DefeatMusic,      TEXT("DefeatMusic"));

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
	// Musique de PRÉPARATION (boucle) dès qu'on entre en placement.
	PlayMusic(PreparationMusic, /*bLoop=*/true);

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
		// PHASE 3 : ~80 unités/faction (1 chef + 1 mythique + 6 spéciales + 72 réparties).
		// Armée plus RÉSISTANTE -> la bataille DURE (vise ~15 min, pas 3). Formation ÉTALÉE.
		InfantryCount = 34; MountedCount = 18; RangedCount = 20; SpecialCount = 6;
		ArmyHealthScale = 4.0f;
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
		SpecialCount = 3; ArmyHealthScale = 2.2f;
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
	const FVector PlayerOrigin = Center + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
	const FVector EnemyOrigin  = Center + FVector( ArmySeparation * 0.5f, 0.f, 0.f);
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
			? FString(TEXT("PHASE 3 — Remportez la bataille : mettez la rivale en DEROUTE pour conquerir la nouvelle zone"))
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

	Say(BT == EBattleType::RivalDefense
		? TEXT("Phase 2 — La faction rivale attaque ! Defendez la zone !")
		: TEXT("Phase 1 — Bataille contre le Kraken. Aneantissez-le !"));
	LaunchBattle();
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
	const float Depth = UnitSpacing * 1.5f;   // espacement entre rangées (X)
	const float GroundZ = 100.f;

	// Place un groupe en rangées (se replie sur plusieurs lignes vers l'arrière -X). En
	// phase 3, les rangées sont bien plus LARGES -> la ligne s'étale sur la largeur du tiers
	// (fini l'empilement). Toutes les unités reçoivent l'échelle de PV de la bataille.
	auto PlaceRows = [&](FName Id, int32 Count, float BackStart, int32 PerRow)
	{
		if (Id.IsNone()) return;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow, Col = i % PerRow;
			const float Y = (Col - (PerRow - 1) * 0.5f) * Lat;
			SpawnUnit(Id, Origin + FVector(-BackStart - Row * Depth, Y, GroundZ), Facing, 1.f, ArmyHealthScale);
		}
	};
	const int32 PRinf = bGrandBattle ? 18 : 8;
	const int32 PRmon = bGrandBattle ? 12 : 6;
	const int32 PRdis = bGrandBattle ? 16 : 8;
	const int32 PRspe = bGrandBattle ? 6  : 3;

	// Chef en pointe
	SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Chef),
		Origin + FVector(Depth, 0.f, GroundZ), Facing, 1.f, ArmyHealthScale);

	PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie), InfantryCount, 0.f, PRinf);
	PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Montee), MountedCount, Depth * 3.f, PRmon);
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Distance), RangedCount, Depth * 5.f, PRdis);
	}
	// PHASE 3 : SPÉCIALE (arrière-ligne) + MYTHIQUE (soutien) débloquées.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Speciale))
	{
		PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Speciale), SpecialCount, Depth * 6.f, PRspe);
	}
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Mythique))
	{
		// ScaleBoost = 1.0 : le mythique est DÉJÀ grand ; le surdimensionner (1.6) gonflait
		// aussi sa CAPSULE de collision -> il restait BLOQUÉ dans le décor (impossible à
		// déplacer). À l'échelle 1, il bouge normalement.
		SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Mythique),
			Origin + FVector(-Depth * 7.f, 0.f, GroundZ), Facing, /*ScaleBoost=*/1.0f, /*HealthScale=*/3.0f);
	}
}

void AWOTOLDemoDirector::SpawnEnemyForCreature(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const FName CreatureID = Demo
		? Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Mythique)
		: NAME_None;
	if (AWOTOLDemoUnit* Creature = SpawnUnit(
			CreatureID, Origin + FVector(0.f, 0.f, 80.f), Facing, 1.5f, CreatureHealthScale, /*bAsBoss=*/true))
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
			Data->Stats.DefensePercent = FMath::Min(Data->Stats.DefensePercent, 18.f); // encaisse bien moins
			Data->Stats.BlockChance    = FMath::Min(Data->Stats.BlockChance, 10.f);     // pare rarement
			Data->Stats.DodgeChance    = FMath::Min(Data->Stats.DodgeChance, 2.f);
			// Frappe forte mais plus soutenable pour un petit groupe (était 420).
			Data->Stats.AttackDPS      = FMath::Clamp(Data->Stats.AttackDPS, 240.f, 300.f);
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
	const float Lat = UnitSpacing, Depth = UnitSpacing * 1.4f;
	auto SetLayer = [](AWOTOLDemoUnit* U, float Z) { if (U) U->SetDesiredZ(Z); };

	// SEULE CONTRAINTE : l'ennemi est LIMITÉ À SON TIERS (comme le joueur au sien). On
	// borne donc chaque unité pour qu'elle ne franchisse pas la limite MIROIR (côté ennemi
	// = Center - PlacementBoundaryOffsetX). À l'intérieur, il place ses unités LIBREMENT.
	const float MirrorX = GetActorLocation().X - PlacementBoundaryOffsetX; // limite du tiers ennemi
	const FVector O = Origin;

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

	auto PlaceRows = [&](FName Id, EDemoUnitCategory Cat, int32 Count, float BackStart, int32 PerRow)
	{
		if (Id.IsNone()) return;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow;
			const int32 Col = i % PerRow;
			const float Y = (Col - (PerRow - 1) * 0.5f) * Lat;
			FVector Loc = O + FVector(BackStart + Row * Depth, Y, 100.f);
			Loc.X = FMath::Max(Loc.X, MirrorX); // ne pas franchir la limite de son tiers
			SetLayer(SpawnUnit(Id, Loc, Facing, 1.f, ArmyHealthScale), PickLayer(Cat));
		}
	};
	const int32 PRinf = bGrandBattle ? 18 : 8;
	const int32 PRmon = bGrandBattle ? 12 : 6;
	const int32 PRdis = bGrandBattle ? 16 : 8;
	const int32 PRspe = bGrandBattle ? 6  : 3;

	FVector ChefLoc = O + FVector(-Depth, 0.f, 100.f);
	ChefLoc.X = FMath::Max(ChefLoc.X, MirrorX);
	SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef), ChefLoc, Facing, 1.f, ArmyHealthScale),
		PickLayer(EDemoUnitCategory::Chef));

	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Infanterie), EDemoUnitCategory::Infanterie, InfantryCount, 0.f, PRinf);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Montee), EDemoUnitCategory::Montee, MountedCount, Depth * 3.f, PRmon);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Distance), EDemoUnitCategory::Distance, RangedCount, Depth * 5.f, PRdis);
	// PHASE 3 : la rivale déploie AUSSI sa spéciale + son mythique.
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Speciale))
	{
		PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Speciale), EDemoUnitCategory::Speciale, SpecialCount, Depth * 6.f, PRspe);
	}
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Mythique))
	{
		FVector MLoc = O + FVector(Depth * 7.f, 0.f, 100.f);
		MLoc.X = FMath::Max(MLoc.X, MirrorX);
		SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Mythique), MLoc, Facing, /*ScaleBoost=*/1.0f, 3.0f),
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

	const FTransform SpawnTM(Facing, Loc, FVector(ScaleBoost));

	AWOTOLDemoUnit* Unit = GetWorld()->SpawnActorDeferred<AWOTOLDemoUnit>(
		DemoUnitClass, SpawnTM, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Unit) return nullptr;

	Unit->UnitData    = Data;
	Unit->HealthScale = HealthScale;   // appliqué dans BeginPlay (avant FinishSpawning)
	Unit->bIsBoss     = bAsBoss;       // AVANT FinishSpawning -> silhouette Kraken forcée
	UGameplayStatics::FinishSpawningActor(Unit, SpawnTM);

	SpawnedUnits.Add(Unit);
	return Unit;
}

void AWOTOLDemoDirector::LaunchBattle()
{
	// Musique de COMBAT (boucle) : remplace la musique de préparation en fondu.
	PlayMusic(BattleMusic, /*bLoop=*/true);

	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->StartBattlePhase(600.f);
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
						// PHASE 1 : 0.42 = VICTOIRE garantie (valeur éprouvée). Les pertes viennent
						// de l'ÉCRASEMENT de zone du Kraken (modéré), pas d'une surenchère de PV
						// (0.50 le rendait imbattable -> défaite). On gagne AVEC quelques pertes.
						const float TargetHP = FMath::Clamp(ArmyHP * 0.42f, 12000.f, 34000.f);
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
	PlayMusic(VictoryMusic, /*bLoop=*/false); // stinger de victoire (coupe la musique de combat)
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
		BuildBattleSummary(true, /*bFinal=*/false, TEXT("VICTOIRE — LA RIVALE RECULE"));
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
			Demo->bDemoVictory = true;
			Demo->SetPhase(EDemoPhase::DemoEnd);
			Demo->SetScreen(EDemoScreen::Summary);
		}
	}
}

void AWOTOLDemoDirector::OnPlayerDefeat()
{
	PlayMusic(DefeatMusic, /*bLoop=*/false); // stinger de défaite (coupe la musique de combat)
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

	// Arène plus VASTE (autre terrain, sensation épique) + nouveau courant océanique.
	ArmySeparation = 7000.f;
	if (UWorld* W = GetWorld())
		if (UOceanCurrentSubsystem* Cur = W->GetSubsystem<UOceanCurrentSubsystem>())
			Cur->Regenerate();

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
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	// UNE SEULE bande lumineuse AU SOL le long de la limite (en Y). Pas de cubes, pas de
	// marqueurs verticaux : juste une ligne. Le "mur invisible" est le clamp de déplacement
	// (IssueCommandToSelection) qui empêche de placer/déplacer au-delà du premier tiers.
	FActorSpawnParameters P; P.Owner = this;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Line = W->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), FVector(BX, GetActorLocation().Y, 12.f), FRotator::ZeroRotator, P);
	if (Line)
	{
		if (UStaticMeshComponent* C = Line->GetStaticMeshComponent())
		{
			C->SetMobility(EComponentMobility::Movable);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCanEverAffectNavigation(false);
			if (Cube) C->SetStaticMesh(Cube);
			// Fine (X), très longue (Y), plate (Z) = trait lumineux posé au sol.
			Line->SetActorScale3D(FVector(0.15f, 66.f, 0.06f));
			if (BaseMat)
			{
				if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, Line))
				{
					MID->SetVectorParameterValue(TEXT("Color"), Col);
					C->SetMaterial(0, MID);
				}
			}
		}
		PlacementMarkers.Add(Line);
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
	for (const FCover& S : Layout)
	{
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
