#include "UnitMoraleComponent.h"

UUnitMoraleComponent::UUnitMoraleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUnitMoraleComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentMorale = StartingMorale;
}

void UUnitMoraleComponent::ApplyMoraleHit(float Amount)
{
	if (bIsRouting || Amount <= 0.f) return;

	CurrentMorale = FMath::Max(0.f, CurrentMorale - Amount);
	OnMoraleChanged.Broadcast(CurrentMorale);
	CheckRoutingState();
}

void UUnitMoraleComponent::RestoreMorale(float Amount)
{
	if (Amount <= 0.f) return;

	const float Previous = CurrentMorale;
	CurrentMorale = FMath::Min(100.f, CurrentMorale + Amount);
	OnMoraleChanged.Broadcast(CurrentMorale);

	if (bIsRouting && CurrentMorale > 20.f)
	{
		bIsRouting = false;
		OnMoraleRestored.Broadcast();
	}
}

float UUnitMoraleComponent::GetMoralePercent() const
{
	return CurrentMorale / 100.f;
}

void UUnitMoraleComponent::OnCommanderDied()
{
	ApplyMoraleHit(CommanderDeathPenalty);
}

void UUnitMoraleComponent::OnNearbyAllyDied()
{
	ApplyMoraleHit(AllyDeathMoralePenalty);
}

void UUnitMoraleComponent::SetCommanderAura(bool bActive, float AuraBonus)
{
	ActiveCommanderAuraBonus = bActive ? AuraBonus : 0.f;
	if (bActive)
	{
		// Tick de régénération passive via l'aura du commandant
		RestoreMorale(AuraBonus * 0.1f);
	}
}

void UUnitMoraleComponent::CheckRoutingState()
{
	if (!bIsRouting && CurrentMorale <= 0.f)
	{
		bIsRouting = true;
		OnUnitRouting.Broadcast();
	}
}
