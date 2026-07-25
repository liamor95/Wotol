#include "FactionRegistrySubsystem.h"
#include "Gameplay/Units/UnitBase.h"

void UFactionRegistrySubsystem::RegisterUnit(AUnitBase* Unit, EFactionID Faction)
{
	if (!Unit) return;

	TArray<TWeakObjectPtr<AUnitBase>>& List = Registry.FindOrAdd(Faction);
	List.AddUnique(Unit);
	OnUnitRegistered.Broadcast(Unit, Faction);
}

void UFactionRegistrySubsystem::UnregisterUnit(AUnitBase* Unit, EFactionID Faction)
{
	if (!Unit) return;

	if (TArray<TWeakObjectPtr<AUnitBase>>* List = Registry.Find(Faction))
	{
		List->RemoveAll([Unit](const TWeakObjectPtr<AUnitBase>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == Unit;
		});
	}

	OnUnitUnregistered.Broadcast(Unit, Faction);
}

TArray<AUnitBase*> UFactionRegistrySubsystem::GetUnitsForFaction(EFactionID Faction) const
{
	TArray<AUnitBase*> Result;

	if (const TArray<TWeakObjectPtr<AUnitBase>>* List = Registry.Find(Faction))
	{
		for (const TWeakObjectPtr<AUnitBase>& Ptr : *List)
		{
			if (Ptr.IsValid())
			{
				Result.Add(Ptr.Get());
			}
		}
	}

	return Result;
}

int32 UFactionRegistrySubsystem::GetUnitCountForFaction(EFactionID Faction) const
{
	return GetUnitsForFaction(Faction).Num();
}

bool UFactionRegistrySubsystem::IsFactionEliminated(EFactionID Faction) const
{
	return GetUnitCountForFaction(Faction) == 0;
}
