#include "BattleStateObserver.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Units/UnitBase.h"

ABattleStateObserver::ABattleStateObserver()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABattleStateObserver::BeginPlay()
{
	Super::BeginPlay();
}

void ABattleStateObserver::BeginObserving()
{
	if (UFactionRegistrySubsystem* Registry =
			GetWorld()->GetSubsystem<UFactionRegistrySubsystem>())
	{
		Registry->OnUnitUnregistered.AddDynamic(
			this, &ABattleStateObserver::OnUnitUnregistered);
	}
}

void ABattleStateObserver::StopObserving()
{
	if (UFactionRegistrySubsystem* Registry =
			GetWorld()->GetSubsystem<UFactionRegistrySubsystem>())
	{
		Registry->OnUnitUnregistered.RemoveDynamic(
			this, &ABattleStateObserver::OnUnitUnregistered);
	}
}

void ABattleStateObserver::SetBattlePhase(EBattlePhase NewPhase)
{
	if (NewPhase == CurrentPhase) return;
	CurrentPhase = NewPhase;
	OnBattleStateChanged.Broadcast(NewPhase);
}

void ABattleStateObserver::OnUnitUnregistered(AUnitBase* /*Unit*/, EFactionID /*Faction*/)
{
	if (CurrentPhase == EBattlePhase::Tactical)
	{
		CheckVictoryConditions();
	}
}

void ABattleStateObserver::CheckVictoryConditions()
{
	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return;

	TArray<EFactionID> Survivors;
	for (EFactionID FID : ActiveFactions)
	{
		if (!Registry->IsFactionEliminated(FID))
		{
			Survivors.Add(FID);
		}
	}

	if (Survivors.Num() == 1)
	{
		SetBattlePhase(EBattlePhase::Resolution);
		OnVictoryConditionMet.Broadcast(Survivors[0], EBattleResult::Victory);
	}
	else if (Survivors.IsEmpty())
	{
		SetBattlePhase(EBattlePhase::Resolution);
		OnVictoryConditionMet.Broadcast(EFactionID::None, EBattleResult::Draw);
	}
}
