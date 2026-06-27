#include "TacticalPhaseManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UTacticalPhaseManager::Deinitialize()
{
	StopPhase();
	Super::Deinitialize();
}

void UTacticalPhaseManager::StartPhase(
	const TArray<EFactionID>& TurnOrder, float WindowDurationSeconds)
{
	if (TurnOrder.IsEmpty()) return;

	CurrentTurnOrder = TurnOrder;
	WindowDuration   = FMath::Max(WindowDurationSeconds, 1.f);
	TurnIndex        = 0;
	bRunning         = true;

	AdvanceTurn();
}

void UTacticalPhaseManager::StopPhase()
{
	if (!bRunning) return;

	bRunning = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TurnTimer);
	}

	if (ActiveFaction != EFactionID::None)
	{
		OnTacticalWindowClosed.Broadcast(ActiveFaction);
		ActiveFaction = EFactionID::None;
	}
}

void UTacticalPhaseManager::AdvanceTurn()
{
	if (!bRunning || CurrentTurnOrder.IsEmpty()) return;

	if (ActiveFaction != EFactionID::None)
	{
		OnTacticalWindowClosed.Broadcast(ActiveFaction);
	}

	ActiveFaction = CurrentTurnOrder[TurnIndex % CurrentTurnOrder.Num()];
	TurnIndex++;

	OnTacticalWindowOpened.Broadcast(ActiveFaction);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TurnTimer,
			this,
			&UTacticalPhaseManager::AdvanceTurn,
			WindowDuration,
			false);
	}
}
