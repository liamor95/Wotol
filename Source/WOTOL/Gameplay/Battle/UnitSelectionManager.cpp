#include "UnitSelectionManager.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"
#include "GameFramework/PlayerController.h"

void UUnitSelectionManager::SelectUnit(AUnitBase* Unit, EFactionID PlayerFaction)
{
	ClearSelection();
	if (IsValidForSelection(Unit, PlayerFaction))
	{
		AddUnitInternal(Unit);
		OnSelectionChanged.Broadcast();
	}
}

void UUnitSelectionManager::AddToSelection(AUnitBase* Unit, EFactionID PlayerFaction)
{
	if (!IsValidForSelection(Unit, PlayerFaction)) return;
	if (SelectedUnits.Contains(Unit)) return;

	AddUnitInternal(Unit);
	OnSelectionChanged.Broadcast();
}

void UUnitSelectionManager::BoxSelect(APlayerController* PC,
	FVector2D ScreenStart, FVector2D ScreenEnd, EFactionID PlayerFaction)
{
	if (!PC) return;

	ClearSelection();

	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return;

	const FVector2D MinScreen(
		FMath::Min(ScreenStart.X, ScreenEnd.X),
		FMath::Min(ScreenStart.Y, ScreenEnd.Y));
	const FVector2D MaxScreen(
		FMath::Max(ScreenStart.X, ScreenEnd.X),
		FMath::Max(ScreenStart.Y, ScreenEnd.Y));

	for (AUnitBase* Unit : Registry->GetUnitsForFaction(PlayerFaction))
	{
		if (!Unit || !Unit->IsAlive()) continue;

		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(Unit->GetActorLocation(), ScreenPos, true))
		{
			if (ScreenPos.X >= MinScreen.X && ScreenPos.X <= MaxScreen.X
				&& ScreenPos.Y >= MinScreen.Y && ScreenPos.Y <= MaxScreen.Y)
			{
				AddUnitInternal(Unit);
			}
		}
	}

	if (!SelectedUnits.IsEmpty())
	{
		OnSelectionChanged.Broadcast();
	}
}

void UUnitSelectionManager::SelectAllOfFaction(EFactionID Faction)
{
	ClearSelection();

	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return;

	for (AUnitBase* Unit : Registry->GetUnitsForFaction(Faction))
	{
		if (Unit && Unit->IsAlive())
		{
			AddUnitInternal(Unit);
		}
	}

	if (!SelectedUnits.IsEmpty())
	{
		OnSelectionChanged.Broadcast();
	}
}

void UUnitSelectionManager::ClearSelection()
{
	for (AUnitBase* Unit : SelectedUnits)
	{
		if (Unit) Unit->SetSelected(false);
	}
	SelectedUnits.Empty();
}

bool UUnitSelectionManager::IsValidForSelection(
	AUnitBase* Unit, EFactionID PlayerFaction) const
{
	return Unit && Unit->IsAlive() && Unit->GetFaction() == PlayerFaction;
}

void UUnitSelectionManager::AddUnitInternal(AUnitBase* Unit)
{
	SelectedUnits.AddUnique(Unit);
	Unit->SetSelected(true);
}
