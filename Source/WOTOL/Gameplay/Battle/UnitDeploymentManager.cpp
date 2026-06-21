#include "UnitDeploymentManager.h"
#include "HexGridManager.h"
#include "Gameplay/Units/UnitBase.h"

UHexGridManager* UUnitDeploymentManager::GetHexGrid() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UHexGridManager>() : nullptr;
}

void UUnitDeploymentManager::BeginDeployment(EFactionID Faction, const TArray<AUnitBase*>& UnitsToPlace)
{
	DeploymentFaction = Faction;
	UnplacedUnits.Reset();
	PlacedUnits.Reset();
	SelectedUnit = nullptr;

	for (AUnitBase* Unit : UnitsToPlace)
	{
		if (Unit) UnplacedUnits.Add(Unit);
	}
}

void UUnitDeploymentManager::SelectUnitForPlacement(AUnitBase* Unit)
{
	SelectedUnit = Unit;
}

bool UUnitDeploymentManager::PlaceSelectedUnit(FHexCoord Cell)
{
	AUnitBase* Unit = SelectedUnit.Get();
	if (!Unit) return false;

	UHexGridManager* Grid = GetHexGrid();
	if (!Grid) return false;

	if (!Grid->PlaceUnit(Unit, Cell, DeploymentFaction)) return false;

	UnplacedUnits.RemoveAll([Unit](const TWeakObjectPtr<AUnitBase>& W) { return W.Get() == Unit; });
	PlacedUnits.Add(Cell, Unit);
	SelectedUnit = nullptr;

	OnUnitPlaced.Broadcast(Unit, Cell);
	return true;
}

bool UUnitDeploymentManager::UnplaceUnit(FHexCoord Cell)
{
	TWeakObjectPtr<AUnitBase>* Ptr = PlacedUnits.Find(Cell);
	if (!Ptr || !Ptr->IsValid()) return false;

	AUnitBase* Unit = Ptr->Get();
	PlacedUnits.Remove(Cell);
	UnplacedUnits.Add(Unit);

	if (UHexGridManager* Grid = GetHexGrid())
	{
		Grid->RemoveUnit(Cell);
	}

	OnUnitUnplaced.Broadcast(Cell);
	return true;
}

bool UUnitDeploymentManager::IsDeploymentComplete() const
{
	return UnplacedUnits.IsEmpty();
}

void UUnitDeploymentManager::ConfirmDeployment()
{
	OnDeploymentConfirmed.Broadcast();
}

TArray<AUnitBase*> UUnitDeploymentManager::GetUnplacedUnits() const
{
	TArray<AUnitBase*> Result;
	for (const TWeakObjectPtr<AUnitBase>& W : UnplacedUnits)
	{
		if (W.IsValid()) Result.Add(W.Get());
	}
	return Result;
}
