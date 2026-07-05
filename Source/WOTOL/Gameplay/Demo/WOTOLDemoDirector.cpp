#include "WOTOLDemoDirector.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
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
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Data/UnitDataRegistrySubsystem.h"
#include "Core/WOTOLGameInstance.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
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

// Monte les armées en PRÉPARATION (placement libre), SANS lancer le combat.
// Fonctionne pour LES DEUX phases (créature ou défense rivale) selon la phase courante.
void AWOTOLDemoDirector::BeginPreparation()
{
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

	// Phase 2 (défense rivale) : GRANDE bataille — plus d'unités des deux côtés.
	// Valeurs volontairement mesurées : ~26 vs 26 unités entièrement riggées, pour
	// rester fluide/stable sur un portable (évite les surcharges mémoire/GPU).
	if (BT == EBattleType::RivalDefense)
	{
		InfantryCount = 12; MountedCount = 6; RangedCount = 8;
	}

	CleanupUnits(); // repart d'une armée propre (utile en phase 2)
	bBattleConcluded = false;

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
	FocusCameraOnPlayer();
	if (Demo)
	{
		Demo->SetScreen(EDemoScreen::Prepare);
		Demo->SetObjective(BT == EBattleType::RivalDefense
			? FString::Printf(TEXT("Proteger le %s — ne le laissez pas tomber a 0"),
				*BuildingDisplayName(CachedPlayerFaction))
			: FString(TEXT("Vaincre la creature — le KRAKEN")));
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

	// Place un groupe en rangées (se replie sur plusieurs lignes vers l'arrière -X).
	auto PlaceRows = [&](FName Id, int32 Count, float BackStart, int32 PerRow)
	{
		if (Id.IsNone()) return;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow, Col = i % PerRow;
			const float Y = (Col - (PerRow - 1) * 0.5f) * Lat;
			SpawnUnit(Id, Origin + FVector(-BackStart - Row * Depth, Y, GroundZ), Facing, 1.f);
		}
	};

	// Chef en pointe
	SpawnUnit(Demo->GetUnitID(Faction, EDemoUnitCategory::Chef),
		Origin + FVector(Depth, 0.f, GroundZ), Facing, 1.f);

	PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie), InfantryCount, 0.f, 8);
	PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Montee), MountedCount, Depth * 3.f, 6);
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		PlaceRows(Demo->GetUnitID(Faction, EDemoUnitCategory::Distance), RangedCount, Depth * 5.f, 8);
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

	// PLACEMENT SYMÉTRIQUE : l'ennemi doit respecter le MÊME espace neutre au centre que
	// le joueur (dont la limite de placement est à Center + PlacementBoundaryOffsetX).
	// On décale toute la formation vers l'arrière si sa ligne avant dépasserait la limite
	// MIROIR (Center - PlacementBoundaryOffsetX), pour un même écart des deux côtés.
	const float MirrorX    = GetActorLocation().X - PlacementBoundaryOffsetX; // ex. Center + 1200
	const float FrontReach = Depth * 5.f;                                     // avancée max (distance)
	const float ShiftX     = FMath::Max(0.f, MirrorX - (Origin.X - FrontReach));
	const FVector O        = Origin + FVector(ShiftX, 0.f, 0.f);

	// Rangée compacte (colonnes de PerRow, se replie sur plusieurs lignes)
	auto PlaceRows = [&](FName Id, int32 Count, float BackStart, float Layer, int32 PerRow)
	{
		if (Id.IsNone()) return;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow;
			const int32 Col = i % PerRow;
			const float Y = (Col - (PerRow - 1) * 0.5f) * Lat;
			const FVector Loc = O + FVector(-BackStart - Row * Depth, Y, 100.f);
			SetLayer(SpawnUnit(Id, Loc, Facing, 1.f), Layer);
		}
	};

	SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef),
		O + FVector(0.f, 0.f, 100.f), Facing, 1.f), L1);

	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Infanterie), InfantryCount, Depth, L0, 8);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Montee), MountedCount, Depth * 3.f, L1, 6);
	PlaceRows(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Distance), RangedCount, Depth * 5.f, L2, 8);
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
			const bool bObjectiveHeld = (CaptureObject != nullptr); // bâtiment encore debout
			if (bObjectiveHeld) OnPlayerVictory();
			else                OnPlayerDefeat();
		}
	}
}

void AWOTOLDemoDirector::OnPlayerVictory()
{
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
		// Résumé FINAL de démo (victoire) : boutons Rejouer / Changer de faction.
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		BuildBattleSummary(true, /*bFinal=*/true, TEXT("VICTOIRE"));
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

	const FString Lore = FString::Printf(TEXT(
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

	Demo->SetInterludeText(Lore);
	Demo->SetScreen(EDemoScreen::Interlude);
}

void AWOTOLDemoDirector::ContinueToPhase2()
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (Demo)
	{
		Demo->UnlockRangedUnit();  // distance débloquée pour la phase 2
		Demo->DiscoverMythic();
	}
	SpawnCaptureObject(CachedPlayerFaction); // objet à défendre (visible en phase 2)
	StartRivalDefense();                     // -> phase 2 en PRÉPARATION
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
	// Roster de phase 1 (les valeurs phase 2 sont réappliquées par BeginPreparation)
	InfantryCount = 10; MountedCount = 5; RangedCount = 5;

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
						// Anneau de boucliers TOUT AUTOUR du bâtiment (dos au centre) :
						// bloque le corps-à-corps ET les tirs à distance venant de dehors.
						const int32 c = infCol++;
						const float Ang = (2.f * PI) * ((float)c / 8.f);
						const FVector RD(FMath::Cos(Ang), FMath::Sin(Ang), 0.f);
						Dest  = ObjLoc + RD * 360.f;
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
						const bool  bCharge = FMath::Fmod(Now + idx * 1.7f, Cycle) < 4.f;
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
					St->SightRange = 60000.f;
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
				// ASSAUT sur le Cristalliseur : lire la défense adverse et la briser.
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
					// MUR défensif sur 2 couches (sol + 1re hauteur) au front : bloque et
					// protège les lignes arrière. Se tient en ligne, ne charge pas.
					const int32 c = infCol++;
					Dest  = Front + Lateral * ((float)(c / 2 - 2) * 240.f);
					Layer = (bCanLayer && (c % 2 == 1)) ? 800.f : 0.f;
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
					const bool  bCharge = FMath::Fmod(Now + idx * 1.7f, Cycle) < 3.5f;
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
				St->SightRange = 60000.f;
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
