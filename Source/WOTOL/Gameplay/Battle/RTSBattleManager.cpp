#include "RTSBattleManager.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Buildings/BuildingManagerSubsystem.h"

void URTSBattleManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void URTSBattleManager::Deinitialize()
{
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BattleTickHandle);
	}
	Super::Deinitialize();
}

void URTSBattleManager::StartDeploymentPhase()
{
	bBattleEnded = false;
	SetPhase(EBattlePhase::Deployment);
}

void URTSBattleManager::StartBattlePhase(float BattleDurationSeconds)
{
	if (CurrentPhase == EBattlePhase::Tactical) return;

	TimeRemaining = FMath::Max(BattleDurationSeconds, 60.f);
	bBattleEnded  = false;

	SetPhase(EBattlePhase::Tactical);

	// Activer toutes les IA ennemies simultanément — c'est un RTS
	ActivateAllEnemyAI();

	// Démarrer le tick temps réel (1s interval)
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(
			BattleTickHandle, this, &URTSBattleManager::BattleTick, 1.f, true, 1.f);
	}
}

void URTSBattleManager::EndBattle(EFactionID Winner, EBattleResult Result)
{
	if (bBattleEnded) return;
	bBattleEnded = true;

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(BattleTickHandle);
	}

	SetPhase(EBattlePhase::Resolution);
	OnBattleEnded.Broadcast(Winner, Result);
}

FText URTSBattleManager::GetFormattedTime() const
{
	const int32 TotalSec = FMath::Max(0, static_cast<int32>(TimeRemaining));
	const int32 Minutes  = TotalSec / 60;
	const int32 Seconds  = TotalSec % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

void URTSBattleManager::BattleTick()
{
	if (bBattleEnded) return;

	TimeRemaining = FMath::Max(0.f, TimeRemaining - 1.f);
	OnBattleTimerTick.Broadcast(TimeRemaining);

	// Tick de production des bâtiments
	if (UBuildingManagerSubsystem* BuildMgr =
			GetWorld()->GetSubsystem<UBuildingManagerSubsystem>())
	{
		BuildMgr->TickProduction(1.f);
		BuildMgr->TickConstruction(1.f);
	}

	// Temps écoulé → match nul (possession de zones détermine vainqueur en BP)
	if (TimeRemaining <= 0.f)
	{
		EndBattle(EFactionID::None, EBattleResult::Draw);
	}
}

void URTSBattleManager::SetPhase(EBattlePhase NewPhase)
{
	if (CurrentPhase == NewPhase) return;
	CurrentPhase = NewPhase;
	OnBattlePhaseChanged.Broadcast(NewPhase);
}

void URTSBattleManager::ActivateAllEnemyAI()
{
	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return;

	// Activer l'IA pour toutes les factions non-joueur
	// Le GameMode filtre quelle faction est IA via SetPlayerFaction()
	for (uint8 i = 1; i <= static_cast<uint8>(EFactionID::PiratesAbyssaux); ++i)
	{
		const EFactionID FID = static_cast<EFactionID>(i);
		for (AUnitBase* Unit : Registry->GetUnitsForFaction(FID))
		{
			if (!Unit) continue;
			if (AAIAdaptiveController* AIC =
					Cast<AAIAdaptiveController>(Unit->GetController()))
			{
				AIC->ActivateRTSBehavior();
			}
		}
	}
}
