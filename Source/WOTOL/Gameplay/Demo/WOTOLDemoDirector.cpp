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

void AWOTOLDemoDirector::BeginPreparation()
{
	// La faction a pu être choisie à l'écran : on relit + recalcule le rival.
	CachedPlayerFaction = ResolvePlayerFaction();
	CachedRivalFaction  = RivalOf(CachedPlayerFaction);

	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (Demo) Demo->SetPhase(EDemoPhase::Battle_Creature);

	// Monte les armées SANS lancer le combat (placement libre par le joueur).
	bBattleConcluded = false;
	const FVector Center = GetActorLocation();
	const FVector PlayerOrigin = Center + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
	const FVector EnemyOrigin  = Center + FVector( ArmySeparation * 0.5f, 0.f, 0.f);
	SpawnPlayerArmy(CachedPlayerFaction, PlayerOrigin, FRotator(0.f, 0.f, 0.f));
	SpawnEnemyForCreature(CachedRivalFaction, EnemyOrigin, FRotator(0.f, 180.f, 0.f));

	FocusCameraOnPlayer();
	if (Demo)
	{
		Demo->SetScreen(EDemoScreen::Prepare);
		Demo->SetObjective(TEXT("Vaincre la creature — le KRAKEN"));
	}
	Say(TEXT("Préparez vos troupes : clic gauche = sélection, clic droit = déplacer. Puis lancez la bataille."));
}

void AWOTOLDemoDirector::StartBattleNow()
{
	UDemoFlowSubsystem* Demo = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	// Active le cerveau autonome du boss (il était en attente pendant la prépa)
	if (Demo)
	{
		if (AWOTOLDemoUnit* Boss = Cast<AWOTOLDemoUnit>(Demo->GetBoss()))
		{
			Boss->bCreatureBrain = true;
		}
		Demo->SetScreen(EDemoScreen::Playing);
	}

	Say(TEXT("Phase 1 — Bataille contre la créature. Anéantissez-la !"));
	LaunchBattle();
}

EFactionID AWOTOLDemoDirector::ResolvePlayerFaction() const
{
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
	const float Depth = UnitSpacing * 1.7f;   // espacement entre rangées (X)
	const float GroundZ = 100.f;

	auto Place = [&](FName UnitID, const FVector& Offset)
	{
		if (UnitID.IsNone()) return;
		SpawnUnit(UnitID, Origin + Offset + FVector(0.f, 0.f, GroundZ), Facing, 1.f);
	};

	// Chef en pointe
	Place(Demo->GetUnitID(Faction, EDemoUnitCategory::Chef), FVector(Depth, 0.f, 0.f));

	// Infanterie : deux paquets de 5 alignés, séparés au centre
	const FName InfID = Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie);
	const int32 Half  = FMath::Max(1, InfantryCount / 2);
	for (int32 i = 0; i < InfantryCount; ++i)
	{
		const bool  bRight = (i >= Half);
		const int32 k      = bRight ? (i - Half) : i;
		const float Side   = bRight ? 1.f : -1.f;
		const float Y      = Side * ((k + 0.5f) * Lat + Lat * 0.8f); // gap central
		Place(InfID, FVector(0.f, Y, 0.f));
	}

	// Montée : rangée alignée derrière l'infanterie
	const FName MntID = Demo->GetUnitID(Faction, EDemoUnitCategory::Montee);
	for (int32 i = 0; i < MountedCount; ++i)
	{
		const float Y = (i - (MountedCount - 1) * 0.5f) * Lat * 1.4f;
		Place(MntID, FVector(-Depth, Y, 0.f));
	}

	// Distance : rangée alignée tout à l'arrière (si débloquée)
	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		const FName RngID = Demo->GetUnitID(Faction, EDemoUnitCategory::Distance);
		for (int32 i = 0; i < RangedCount; ++i)
		{
			const float Y = (i - (RangedCount - 1) * 0.5f) * Lat * 1.4f;
			Place(RngID, FVector(-Depth * 2.f, Y, 0.f));
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
	if (AWOTOLDemoUnit* Creature = SpawnUnit(
			CreatureID, Origin + FVector(0.f, 0.f, 80.f), Facing, 1.5f, CreatureHealthScale))
	{
		// bCreatureBrain reste FAUX pendant la préparation : le boss attend.
		// Il est activé par StartBattleNow() au lancement de la bataille.
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

	// L'IA (Lia) déploie son armée RÉPARTIE SUR 3 COUCHES verticales : mêlée en bas,
	// chef/montée au milieu, distance en haut (elle tire à travers les niveaux).
	const float L0 = 200.f, L1 = 900.f, L2 = 1600.f;
	auto SetLayer = [](AWOTOLDemoUnit* U, float Z) { if (U) U->SetDesiredZ(Z); };

	SetLayer(SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef),
		Origin + FVector(0.f, 0.f, 100.f), Facing, 1.f), L1);

	const FName InfID = Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Infanterie);
	for (int32 i = 0; i < FMath::Max(1, InfantryCount / 2); ++i)
	{
		SetLayer(SpawnUnit(InfID, Origin + FVector(-UnitSpacing, (i - 2.5f) * UnitSpacing, 100.f), Facing, 1.f), L0);
	}

	const FName MntID = Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Montee);
	for (int32 i = 0; i < FMath::Max(1, MountedCount / 2); ++i)
	{
		SetLayer(SpawnUnit(MntID, Origin + FVector(-UnitSpacing * 2.f, (i - 1.f) * UnitSpacing, 100.f), Facing, 1.f), L1);
	}

	// Unités à distance en HAUTEUR (tirent vers le bas à travers les couches)
	const FName RngID = Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Distance);
	for (int32 i = 0; i < FMath::Max(1, RangedCount / 2); ++i)
	{
		SetLayer(SpawnUnit(RngID, Origin + FVector(-UnitSpacing * 3.f, (i - 1.5f) * UnitSpacing, 100.f), Facing, 1.f), L2);
	}
}

AWOTOLDemoUnit* AWOTOLDemoDirector::SpawnUnit(FName UnitID, const FVector& Loc, const FRotator& Facing,
	float ScaleBoost, float HealthScale)
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

	// JOUABILITÉ : tes unités gardent leur IA ACTIVE (donc elles attaquent), mais on
	// les met en "Tenir la position" -> elles n'avancent pas toutes seules et attendent
	// tes ordres (clic droit = bouger/attaquer). L'ennemi reste en attaque automatique.
	if (UWorld* W = GetWorld())
	{
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction(CachedPlayerFaction))
			{
				if (!U) continue;
				if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(U->GetController()))
				{
					AIC->ActivateRTSBehavior();   // IA active = les attaques fonctionnent
					AIC->IssueOrder_HoldPosition(); // mais elles attendent tes ordres
				}
			}
		}
	}

	// IA ENNEMIE : armée rivale STRUCTURÉE et OFFENSIVE — elle avance droit sur
	// l'armée du joueur et engage (plus d'errance/patrouille passive au spawn).
	if (UWorld* W = GetWorld())
	{
		const FVector PlayerCenter = GetActorLocation() + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
		if (UFactionRegistrySubsystem* Reg = W->GetSubsystem<UFactionRegistrySubsystem>())
		{
			for (AUnitBase* U : Reg->GetUnitsForFaction(CachedRivalFaction))
			{
				if (!U) continue;
				AWOTOLDemoUnit* DU = Cast<AWOTOLDemoUnit>(U);
				if (DU && DU->bCreatureBrain) continue; // le boss a son propre cerveau
				if (AAIAdaptiveController* AIC = Cast<AAIAdaptiveController>(U->GetController()))
				{
					AIC->ActivateRTSBehavior();
					AIC->IssueOrder_AttackMove(PlayerCenter); // marche + attaque en chemin
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
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->EndBattle(CachedPlayerFaction, EBattleResult::Victory);
	}

	const EDemoPhase Phase = Demo ? Demo->GetPhase() : EDemoPhase::None;

	if (Phase == EDemoPhase::Battle_Creature)
	{
		Say(TEXT("OBJECTIF REMPLI : le Kraken est vaincu ! Distance débloquée."));
		if (Demo)
		{
			Demo->UnlockRangedUnit();
			Demo->DiscoverMythic();
			Demo->SetObjective(TEXT("Deployer le Cristalliseur et defendre la zone"));
		}
		SpawnCaptureObject(CachedPlayerFaction);
		Say(TEXT("Cristalliseur deploye — Zone capturee (Grade 1). Preparez la defense..."));

		GetWorldTimerManager().SetTimer(
			PhaseHandle, this, &AWOTOLDemoDirector::StartRivalDefense,
			FMath::Max(0.5f, PhaseTransitionDelay), false);
	}
	else if (Phase == EDemoPhase::Battle_Rival)
	{
		Say(TEXT("Objectif atteint ! Zone défendue. Réparation de l'objet de capture..."));
		if (CaptureObject)
		{
			CaptureObject->ApplyDamage(CaptureObject->MaxHealth * 0.4f); // endommagé
			CaptureObject->Repair(CaptureObject->MaxHealth);             // puis réparé
		}
		EndDemo(true);
	}
}

void AWOTOLDemoDirector::OnPlayerDefeat()
{
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->EndBattle(CachedRivalFaction, EBattleResult::Defeat);
	}
	Say(TEXT("Défaite... La démo se relance bientôt."));
	EndDemo(false);
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
}

void AWOTOLDemoDirector::StartRivalDefense()
{
	CleanupUnits();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetPhase(EDemoPhase::Battle_Rival);
		}
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* D = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			D->SetObjective(TEXT("Defendre le Cristalliseur contre la faction rivale"));
		}
	}
	Say(TEXT("Phase 2 — La faction rivale attaque votre zone ! Défendez-la !"));
	StartCurrentBattle();
	FocusCameraOnPlayer(); // recadre derrière l'armée pour la nouvelle phase
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
