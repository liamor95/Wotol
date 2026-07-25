#include "UnitDataRegistrySubsystem.h"
#include "UnitDataLibrary.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Data/WOTOLTypes.h"

void UUnitDataRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Charge les 12 unités de démo depuis la bibliothèque statique
	for (const FName& ID : UUnitDataLibrary::GetAllDemoUnitIDs())
	{
		UUnitDataAsset* Asset = UUnitDataLibrary::CreateUnitDataAsset(ID, this);
		if (Asset)
		{
			Registry.Add(ID, Asset);
		}
	}
}

UUnitDataAsset* UUnitDataRegistrySubsystem::GetUnitData(FName UnitID) const
{
	const TObjectPtr<UUnitDataAsset>* Found = Registry.Find(UnitID);
	return Found ? Found->Get() : nullptr;
}

TArray<UUnitDataAsset*> UUnitDataRegistrySubsystem::GetUnitsForFaction(EFactionID Faction) const
{
	TArray<UUnitDataAsset*> Result;
	for (const auto& Pair : Registry)
	{
		if (Pair.Value && Pair.Value->Faction == Faction)
		{
			Result.Add(Pair.Value.Get());
		}
	}
	return Result;
}

TArray<UUnitDataAsset*> UUnitDataRegistrySubsystem::GetAllDemoUnits() const
{
	TArray<UUnitDataAsset*> Result;
	for (const auto& Pair : Registry)
	{
		if (Pair.Value) Result.Add(Pair.Value.Get());
	}
	return Result;
}
