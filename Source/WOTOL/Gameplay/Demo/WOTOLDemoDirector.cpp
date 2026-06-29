#include "WOTOLDemoDirector.h"
#include "WOTOLDemoUnit.h"
#include "WOTOLCaptureObject.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Core/FactionRegistrySubsystem.h"
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

	// Démarre la démo à la première bataille (créature)
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetPhase(EDemoPhase::Battle_Creature);
		}
	}

	Say(TEXT("Phase 1 — Bataille contre la créature. Anéantissez-la !"));
	StartCurrentBattle();
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

	int32 Row = 0;
	auto PlaceLine = [&](FName UnitID, int32 Count)
	{
		if (UnitID.IsNone() || Count <= 0) return;
		for (int32 i = 0; i < Count; ++i)
		{
			const FVector Loc = Origin +
				FVector(Row * -UnitSpacing, (i - Count * 0.5f) * UnitSpacing, 100.f);
			SpawnUnit(UnitID, Loc, Facing, 1.f);
		}
		++Row;
	};

	PlaceLine(Demo->GetUnitID(Faction, EDemoUnitCategory::Chef), 1);
	PlaceLine(Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie), InfantryCount);
	PlaceLine(Demo->GetUnitID(Faction, EDemoUnitCategory::Montee), MountedCount);

	if (Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		PlaceLine(Demo->GetUnitID(Faction, EDemoUnitCategory::Distance), RangedCount);
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
		Creature->bCreatureBrain = true; // boss autonome (avance + attaque)
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
			{
				Demo->SetBoss(Creature);
			}
		}
	}
}

void AWOTOLDemoDirector::SpawnRivalSquad(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	SpawnUnit(Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Chef),
		Origin + FVector(0.f, 0.f, 100.f), Facing, 1.f);

	const FName InfID = Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Infanterie);
	for (int32 i = 0; i < FMath::Max(1, InfantryCount / 2); ++i)
	{
		SpawnUnit(InfID, Origin + FVector(-UnitSpacing, (i - 2.5f) * UnitSpacing, 100.f), Facing, 1.f);
	}

	const FName MntID = Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Montee);
	for (int32 i = 0; i < FMath::Max(1, MountedCount / 2); ++i)
	{
		SpawnUnit(MntID, Origin + FVector(-UnitSpacing * 2.f, (i - 1.f) * UnitSpacing, 100.f), Facing, 1.f);
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
		Say(TEXT("VICTOIRE ! Créature vaincue. Distance débloquée, mythique juvénile découvert."));
		if (Demo)
		{
			Demo->UnlockRangedUnit();
			Demo->DiscoverMythic();
		}
		SpawnCaptureObject(CachedPlayerFaction);
		Say(TEXT("Zone capturée — Grade 1. Préparez la défense..."));

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

	Say(TEXT("Phase 2 — La faction rivale attaque votre zone ! Défendez-la !"));
	StartCurrentBattle();
}

void AWOTOLDemoDirector::EndDemo(bool bPlayerWon)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
		{
			Demo->SetPhase(EDemoPhase::DemoEnd);
		}
	}

	if (bPlayerWon)
	{
		Say(TEXT("FIN DE DÉMO — Le conflit Aquiloris / Noxéens ne fait que commencer. La suite éveillera le mythique."));
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
