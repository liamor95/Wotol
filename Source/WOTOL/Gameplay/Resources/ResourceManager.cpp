#include "ResourceManager.h"

void UResourceManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UResourceManager::EnsureFactionExists(EFactionID Faction)
{
	if (!Resources.Contains(Faction))
	{
		Resources.Add(Faction, TMap<EResourceType, int32>());
		EnergyCaps.Add(Faction, DefaultEnergyCap);
	}
}

void UResourceManager::AddResource(EFactionID Faction, EResourceType Resource, int32 Amount)
{
	if (Amount <= 0 || Faction == EFactionID::None) return;

	EnsureFactionExists(Faction);
	int32& Current = Resources[Faction].FindOrAdd(Resource);
	Current += Amount;

	// Appliquer le cap pour EnergieOceanique
	if (Resource == EResourceType::EnergieOceanique)
	{
		Current = FMath::Min(Current, EnergyCaps[Faction]);
	}

	OnResourceChanged.Broadcast(Faction, Resource, Current);
}

bool UResourceManager::SpendResource(EFactionID Faction, EResourceType Resource, int32 Amount)
{
	if (Amount <= 0) return true;
	if (!HasEnough(Faction, Resource, Amount)) return false;

	Resources[Faction][Resource] -= Amount;
	OnResourceChanged.Broadcast(Faction, Resource, Resources[Faction][Resource]);
	return true;
}

int32 UResourceManager::GetResource(EFactionID Faction, EResourceType Resource) const
{
	if (const TMap<EResourceType, int32>* FactionRes = Resources.Find(Faction))
	{
		if (const int32* Val = FactionRes->Find(Resource))
		{
			return *Val;
		}
	}
	return 0;
}

int32 UResourceManager::GetResourceCap(EFactionID Faction, EResourceType Resource) const
{
	if (Resource == EResourceType::EnergieOceanique)
	{
		if (const int32* Cap = EnergyCaps.Find(Faction))
		{
			return *Cap;
		}
		return DefaultEnergyCap;
	}
	return INT32_MAX;
}

void UResourceManager::SetEnergyCap(EFactionID Faction, int32 Cap)
{
	EnsureFactionExists(Faction);
	EnergyCaps[Faction] = FMath::Max(0, Cap);

	// Tronquer la valeur actuelle si elle dépasse le nouveau cap
	if (int32* Current = Resources[Faction].Find(EResourceType::EnergieOceanique))
	{
		*Current = FMath::Min(*Current, Cap);
	}
}

bool UResourceManager::HasEnough(EFactionID Faction, EResourceType Resource, int32 Amount) const
{
	return GetResource(Faction, Resource) >= Amount;
}
