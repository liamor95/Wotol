#include "BattleTimerManager.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UBattleTimerManager::StartTimer(float DurationSeconds)
{
	RemainingSeconds = DurationSeconds;
	bRunning = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(TickHandle, this, &UBattleTimerManager::Tick, 1.f, true);
	}
}

void UBattleTimerManager::PauseTimer()
{
	bRunning = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().PauseTimer(TickHandle);
	}
}

void UBattleTimerManager::ResumeTimer()
{
	bRunning = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().UnPauseTimer(TickHandle);
	}
}

void UBattleTimerManager::StopTimer()
{
	bRunning = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickHandle);
	}
}

void UBattleTimerManager::Tick()
{
	if (!bRunning) return;

	RemainingSeconds = FMath::Max(0.f, RemainingSeconds - 1.f);
	OnTimeChanged.Broadcast(RemainingSeconds);

	if (RemainingSeconds <= 0.f)
	{
		StopTimer();
		OnTimeExpired.Broadcast();
	}
}

FText UBattleTimerManager::GetFormattedTime() const
{
	const int32 Total = FMath::FloorToInt(RemainingSeconds);
	const int32 Minutes = Total / 60;
	const int32 Seconds = Total % 60;
	return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds));
}

void UBattleTimerManager::Deinitialize()
{
	StopTimer();
	Super::Deinitialize();
}
