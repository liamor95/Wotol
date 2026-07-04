#include "WOTOLDemoDirector.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Battle/RTSBattleManager.h"
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
			Data->Stats.DefensePercent = FMath::Max(Data->Stats.DefensePercent, 55.f); // encaisse
			Data->Stats.BlockChance    = FMath::Max(Data->Stats.BlockChance, 45.f);     // pare souvent
			Data->Stats.DodgeChance    = FMath::Max(Data->Stats.DodgeChance, 10.f);
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

	// Rangée compacte (colonnes de PerRow, se replie sur plusieurs lignes)
	auto PlaceRows = [&](FName Id, int32 Count, float BackStart, float Layer, int32 PerRow)
	{
		if (Id.IsNone()) return;
		for (int32 i = 0; i < Count; ++i)
		{
			const int32 Row = i / PerRow;
			const int32 Col = i % PerRow;
			const float Y = (Col - (PerRow - 1) * 0.5f) * Lat;
			const FVector Loc = Origin + FVector(-BackStart - Row * Depth, Y, 100.f);
			SetLayer(SpawnUnit(Id, Loc, Facing, 1.f), Layer);
		}
	};

	SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef),
		Origin + FVector(0.f, 0.f, 100.f), Facing, 1.f), L1);

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
	}
	else if (PlayerAlive <= 0)
	{
		bBattleConcluded = true;
		GetWorldTimerManager().ClearTimer(BattleCheckHandle);
		OnPlayerDefeat();
	}
}

void AWOTOLDemoDirector::OnPlayerVictory()
{
	GetWorldTimerManager().ClearTimer(SiegeHandle);
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

	auto Accumulate = [](TArray<FUnitLossEntry>& Out, const FString& Name, EFactionID Fac, bool bDead)
	{
		FUnitLossEntry* E = Out.FindByPredicate([&](const FUnitLossEntry& X){ return X.UnitName == Name; });
		if (!E)
		{
			FUnitLossEntry New; New.UnitName = Name; New.Faction = Fac;
			E = &Out[Out.Add(New)];
		}
		E->Total++;
		if (bDead) E->Lost++;
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
		Accumulate(bPlayer ? Demo->PlayerLosses : Demo->EnemyLosses,
			Name, U->GetFaction(), !U->IsAlive());
	}

	Demo->SummaryTitle    = Title;
	Demo->bSummaryVictory = bVictory;
	Demo->bSummaryIsFinal = bFinal;
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

	const FString Lore = FString::Printf(TEXT(
		"Apres votre victoire sur le Kraken, un oeuf a emerge du Coeur-Eclat du %s que vous\n"
		"avez depose pour capturer la zone. Vous l'avez ramene jusqu'a votre cite.\n\n"
		"Cette decouverte vous a apporte l'experience necessaire pour eriger un NOUVEAU\n"
		"batiment et former une nouvelle categorie d'unites : les %s (unites a distance).\n\n"
		"Mais la faction rivale a repere votre %s et lance l'assaut pour s'emparer de la zone.\n"
		"Deployez vos forces — distance comprise — et PROTEGEZ le batiment a tout prix."),
		*Building, *Ranged, *Building);

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
	CleanupUnits();
	ClearPlacementBoundary();
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
	const float   SiegeRange  = 700.f;
	float TotalDamage = 0.f;
	for (AUnitBase* U : Reg->GetUnitsForFaction(CachedRivalFaction))
	{
		if (!U || !U->IsAlive()) continue;
		if (FVector::Dist2D(U->GetActorLocation(), BuildingLoc) <= SiegeRange)
		{
			TotalDamage += 10.f; // 10 PV/s par assiégeant proche
		}
	}
	if (TotalDamage > 0.f)
	{
		CaptureObject->ApplyDamage(TotalDamage);
	}
}

void AWOTOLDemoDirector::HandleCaptureDestroyed()
{
	if (bBattleConcluded) return;
	Say(FString::Printf(TEXT("Le %s est detruit — objectif perdu !"),
		*BuildingDisplayName(CachedPlayerFaction)));
	bBattleConcluded = true;
	GetWorldTimerManager().ClearTimer(BattleCheckHandle);
	GetWorldTimerManager().ClearTimer(SiegeHandle);
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

	// Ligne de petits cubes le long de la limite (en Y), à 3 hauteurs (verticalité).
	const float Heights[3] = { 60.f, 900.f, 1600.f };
	for (float Y = -3200.f; Y <= 3200.f; Y += 380.f)
	{
		for (float Z : Heights)
		{
			FActorSpawnParameters P; P.Owner = this;
			P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AStaticMeshActor* M = W->SpawnActor<AStaticMeshActor>(
				AStaticMeshActor::StaticClass(), FVector(BX, GetActorLocation().Y + Y, Z), FRotator::ZeroRotator, P);
			if (!M) continue;
			if (UStaticMeshComponent* C = M->GetStaticMeshComponent())
			{
				C->SetMobility(EComponentMobility::Movable);
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				C->SetCanEverAffectNavigation(false);
				if (Cube) C->SetStaticMesh(Cube);
				M->SetActorScale3D(FVector(0.4f, 1.4f, 1.4f));
				if (BaseMat)
				{
					if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, M))
					{
						MID->SetVectorParameterValue(TEXT("Color"), Col);
						C->SetMaterial(0, MID);
					}
				}
			}
			PlacementMarkers.Add(M);
		}
	}
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
