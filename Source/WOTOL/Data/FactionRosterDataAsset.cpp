#include "FactionRosterDataAsset.h"

int32 UFactionRosterDataAsset::GetMaxCountForRole(EUnitRole Role) const
{
	for (const FUnitRosterEntry& Entry : AvailableUnits)
	{
		if (Entry.Role == Role) return Entry.MaxCount;
	}
	return 0;
}
