#include "WOTOLGreyboxArena.h"
#include "WOTOLGreyboxUnit.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Battle/RTSBattleManager.h"
#include "Data/UnitDataRegistrySubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AWOTOLGreyboxArena::AWOTOLGreyboxArena()
{
	PrimaryActorTick.bCanEverTick = false;
	GreyboxUnitClass = AWOTOLGreyboxUnit::StaticClass();
}

void AWOTOLGreyboxArena::BeginPlay()
{
	Super::BeginPlay();

	const FVector Center = GetActorLocation();

	// Armée Aquiloris à gauche (face à droite), Noxéens à droite (face à gauche)
	SpawnArmy(EFactionID::Aquiloris,
		Center + FVector(-ArmySeparation * 0.5f, 0.f, 0.f), FRotator(0.f, 0.f, 0.f));
	SpawnArmy(EFactionID::Noxeens,
		Center + FVector(ArmySeparation * 0.5f, 0.f, 0.f), FRotator(0.f, 180.f, 0.f));

	// Démarrer la bataille RTS après un court délai (spawn + navmesh prêts)
	FTimerHandle TH;
	GetWorldTimerManager().SetTimer(
		TH, this, &AWOTOLGreyboxArena::LaunchBattle, FMath::Max(0.1f, BattleStartDelay), false);
}

void AWOTOLGreyboxArena::SpawnArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing)
{
	if (!GreyboxUnitClass) return;

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (!GI) return;

	UUnitDataRegistrySubsystem* Registry = GI->GetSubsystem<UUnitDataRegistrySubsystem>();
	if (!Registry) return;

	TArray<UUnitDataAsset*> Units = Registry->GetUnitsForFaction(Faction);
	if (Units.Num() == 0) return;

	for (int32 i = 0; i < UnitsPerFaction; ++i)
	{
		UUnitDataAsset* Data = Units[i % Units.Num()];
		if (!Data) continue;

		const FVector Loc = Origin +
			FVector(0.f, (i - UnitsPerFaction * 0.5f) * UnitSpacing, 100.f);
		const FTransform SpawnTM(Facing, Loc);

		// Spawn différé : on assigne UnitData AVANT BeginPlay pour que
		// AUnitBase::InitFromDataAsset lise bien les stats et la faction.
		AWOTOLGreyboxUnit* Unit = GetWorld()->SpawnActorDeferred<AWOTOLGreyboxUnit>(
			GreyboxUnitClass, SpawnTM, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);

		if (!Unit) continue;

		Unit->UnitData = Data;
		UGameplayStatics::FinishSpawningActor(Unit, SpawnTM);
	}
}

void AWOTOLGreyboxArena::LaunchBattle()
{
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		// Active l'IA de toutes les unités → les deux armées s'engagent
		RTS->StartBattlePhase(600.f);
	}
}
