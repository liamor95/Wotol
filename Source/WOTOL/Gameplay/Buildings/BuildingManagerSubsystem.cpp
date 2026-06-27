#include "BuildingManagerSubsystem.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Gameplay/Battle/TerritoryStateManager.h"

void UBuildingManagerSubsystem::Deinitialize()
{
	Buildings.Empty();
	Super::Deinitialize();
}

bool UBuildingManagerSubsystem::CanBuild(
	EFactionID Faction, UBuildingDataAsset* Data, FName ZoneID) const
{
	if (!Data) return false;

	UResourceManager* ResMgr = GetWorld()->GetSubsystem<UResourceManager>();
	if (!ResMgr) return false;

	// Vérifier les ressources
	for (const FBuildingCost& Cost : Data->ConstructionCost)
	{
		if (!ResMgr->HasEnough(Faction, Cost.ResourceType, Cost.Amount))
		{
			return false;
		}
	}

	// Vérifier le grade de territoire
	if (Data->RequiredTerritoryGrade > 0)
	{
		UTerritoryStateManager* TerMgr =
			GetWorld()->GetSubsystem<UTerritoryStateManager>();
		if (!TerMgr || TerMgr->GetZoneGrade(ZoneID) < Data->RequiredTerritoryGrade)
		{
			return false;
		}
	}

	// Vérifier les bâtiments prérequis
	for (const TSoftObjectPtr<UBuildingDataAsset>& Req : Data->RequiredBuildings)
	{
		UBuildingDataAsset* ReqData = Req.Get();
		if (!ReqData) continue;
		if (!HasBuildingOfType(Faction, ReqData)) return false;
	}

	return true;
}

FName UBuildingManagerSubsystem::StartConstruction(
	EFactionID Faction, UBuildingDataAsset* Data, FName ZoneID)
{
	if (!CanBuild(Faction, Data, ZoneID)) return NAME_None;

	UResourceManager* ResMgr = GetWorld()->GetSubsystem<UResourceManager>();
	if (!ResMgr) return NAME_None;

	// Déduire les ressources
	for (const FBuildingCost& Cost : Data->ConstructionCost)
	{
		ResMgr->SpendResource(Faction, Cost.ResourceType, Cost.Amount);
	}

	FName ID = GenerateBuildingID(Data);

	FBuildingInstance Instance;
	Instance.BuildingID           = ID;
	Instance.Data                 = Data;
	Instance.Owner                = Faction;
	Instance.CurrentHealth        = static_cast<float>(Data->MaxHealth);
	Instance.bIsConstructed       = (Data->ConstructionTime <= 0.f);
	Instance.ConstructionProgress = Instance.bIsConstructed ? 100.f : 0.f;

	Buildings.Add(ID, Instance);

	if (Instance.bIsConstructed)
	{
		OnBuildingConstructed.Broadcast(ID, Faction, Data);
	}

	return ID;
}

void UBuildingManagerSubsystem::TickConstruction(float DeltaSeconds)
{
	for (auto& Pair : Buildings)
	{
		FBuildingInstance& B = Pair.Value;
		if (B.bIsConstructed || !B.Data) continue;

		const float Rate = 100.f / FMath::Max(B.Data->ConstructionTime, 0.1f);
		B.ConstructionProgress = FMath::Min(100.f,
			B.ConstructionProgress + Rate * DeltaSeconds);

		if (B.ConstructionProgress >= 100.f)
		{
			CompleteConstruction(Pair.Key);
		}
	}
}

void UBuildingManagerSubsystem::TickProduction(float DeltaSeconds)
{
	UResourceManager* ResMgr = GetWorld()->GetSubsystem<UResourceManager>();
	if (!ResMgr) return;

	for (auto& Pair : Buildings)
	{
		FBuildingInstance& B = Pair.Value;
		if (!B.bIsConstructed || !B.Data || B.Data->ProductionPerTick <= 0) continue;
		if (B.CurrentHealth <= 0.f) continue;

		B.ProductionAccumulator += DeltaSeconds;
		const float Interval = FMath::Max(B.Data->ProductionInterval, 1.f);

		if (B.ProductionAccumulator >= Interval)
		{
			B.ProductionAccumulator -= Interval;
			ResMgr->AddResource(B.Owner, B.Data->ProducedResource, B.Data->ProductionPerTick);
			OnBuildingResourceTick.Broadcast(
				Pair.Key, B.Data->ProducedResource, B.Data->ProductionPerTick);
		}
	}
}

void UBuildingManagerSubsystem::ApplyDamageToBuilding(FName BuildingID, float Damage)
{
	FBuildingInstance* B = Buildings.Find(BuildingID);
	if (!B || Damage <= 0.f) return;

	B->CurrentHealth = FMath::Max(0.f, B->CurrentHealth - Damage);

	if (B->CurrentHealth <= 0.f)
	{
		DestroyBuilding(BuildingID);
	}
}

const FBuildingInstance* UBuildingManagerSubsystem::GetBuilding(FName BuildingID) const
{
	return Buildings.Find(BuildingID);
}

TArray<FName> UBuildingManagerSubsystem::GetBuildingsForFaction(EFactionID Faction) const
{
	TArray<FName> Result;
	for (const auto& Pair : Buildings)
	{
		if (Pair.Value.Owner == Faction)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

bool UBuildingManagerSubsystem::HasBuildingOfType(
	EFactionID Faction, UBuildingDataAsset* Data) const
{
	for (const auto& Pair : Buildings)
	{
		if (Pair.Value.Owner == Faction
			&& Pair.Value.Data == Data
			&& Pair.Value.bIsConstructed)
		{
			return true;
		}
	}
	return false;
}

float UBuildingManagerSubsystem::GetConstructionProgress(FName BuildingID) const
{
	const FBuildingInstance* B = Buildings.Find(BuildingID);
	return B ? B->ConstructionProgress : 0.f;
}

FName UBuildingManagerSubsystem::GenerateBuildingID(UBuildingDataAsset* Data)
{
	return FName(*FString::Printf(TEXT("Building_%s_%d"),
		Data ? *Data->DisplayName.ToString() : TEXT("Unknown"),
		NextBuildingIndex++));
}

void UBuildingManagerSubsystem::CompleteConstruction(FName BuildingID)
{
	FBuildingInstance* B = Buildings.Find(BuildingID);
	if (!B) return;

	B->bIsConstructed       = true;
	B->ConstructionProgress = 100.f;
	OnBuildingConstructed.Broadcast(BuildingID, B->Owner, B->Data);
}

void UBuildingManagerSubsystem::DestroyBuilding(FName BuildingID)
{
	FBuildingInstance* B = Buildings.Find(BuildingID);
	if (!B) return;

	OnBuildingDestroyed.Broadcast(BuildingID, B->Owner);
	Buildings.Remove(BuildingID);
}
