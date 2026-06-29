#include "WOTOLDemoDirector.h"
#include "WOTOLDemoUnit.h"
#include "DemoFlowSubsystem.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Data/UnitDataRegistrySubsystem.h"
#include "Core/WOTOLGameInstance.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AWOTOLDemoDirector::AWOTOLDemoDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	DemoUnitClass = AWOTOLDemoUnit::StaticClass();
}

void AWOTOLDemoDirector::BeginPlay()
{
	Super::BeginPlay();
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

void AWOTOLDemoDirector::StartCurrentBattle()
{
	const EFactionID Player = ResolvePlayerFaction();
	const EFactionID Rival  = RivalOf(Player);
	const FVector Center    = GetActorLocation();

	const FVector PlayerOrigin = Center + FVector(-ArmySeparation * 0.5f, 0.f, 0.f);
	const FVector EnemyOrigin   = Center + FVector( ArmySeparation * 0.5f, 0.f, 0.f);
	const FRotator FaceRight(0.f, 0.f, 0.f);
	const FRotator FaceLeft(0.f, 180.f, 0.f);

	SpawnPlayerArmy(Player, PlayerOrigin, FaceRight);

	// Type de bataille selon la phase de démo
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
		SpawnRivalSquad(Rival, EnemyOrigin, FaceLeft);
	}
	else
	{
		SpawnEnemyForCreature(Rival, EnemyOrigin, FaceLeft);
	}

	FTimerHandle TH;
	GetWorldTimerManager().SetTimer(
		TH, this, &AWOTOLDemoDirector::LaunchBattle, FMath::Max(0.1f, BattleStartDelay), false);
}

void AWOTOLDemoDirector::SpawnPlayerArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;

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

	// Chef (toujours) + infanterie + montée
	PlaceLine(Demo ? Demo->GetUnitID(Faction, EDemoUnitCategory::Chef) : NAME_None, 1);
	PlaceLine(Demo ? Demo->GetUnitID(Faction, EDemoUnitCategory::Infanterie) : NAME_None, InfantryCount);
	PlaceLine(Demo ? Demo->GetUnitID(Faction, EDemoUnitCategory::Montee) : NAME_None, MountedCount);

	// Distance UNIQUEMENT si débloquée (après la 1ère victoire)
	if (Demo && Demo->IsCategoryUnlocked(EDemoUnitCategory::Distance))
	{
		PlaceLine(Demo->GetUnitID(Faction, EDemoUnitCategory::Distance), RangedCount);
	}
}

void AWOTOLDemoDirector::SpawnEnemyForCreature(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing)
{
	// Créature massive : réutilise les stats du mythique rival (gros PV existants),
	// agrandie pour la lisibilité. Placeholder, aucune stat inventée.
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	const FName CreatureID = Demo
		? Demo->GetUnitID(RivalFaction, EDemoUnitCategory::Mythique)
		: NAME_None;
	SpawnUnit(CreatureID, Origin + FVector(0.f, 0.f, 150.f), Facing, 2.5f);
}

void AWOTOLDemoDirector::SpawnRivalSquad(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing)
{
	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return;

	// Petite escouade rivale : chef + infanterie réduite + montée réduite
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

AWOTOLDemoUnit* AWOTOLDemoDirector::SpawnUnit(FName UnitID, const FVector& Loc, const FRotator& Facing, float ScaleBoost)
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

	Unit->UnitData = Data;
	UGameplayStatics::FinishSpawningActor(Unit, SpawnTM);
	return Unit;
}

void AWOTOLDemoDirector::LaunchBattle()
{
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->StartBattlePhase(600.f);
	}
}
