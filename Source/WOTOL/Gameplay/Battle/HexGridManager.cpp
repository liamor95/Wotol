#include "HexGridManager.h"
#include "Gameplay/Units/UnitBase.h"

const FHexCoord UHexGridManager::HexDirections[6] = {
	{1, 0}, {1, -1}, {0, -1}, {-1, 0}, {-1, 1}, {0, 1}
};

FVector UHexGridManager::HexToWorld(FHexCoord Hex) const
{
	// Flat-top hex layout
	const float X = HexSize * (1.5f * Hex.Q);
	const float Y = HexSize * (FMath::Sqrt(3.f) * (Hex.R + Hex.Q * 0.5f));
	return GridOrigin + FVector(X, Y, 0.f);
}

FHexCoord UHexGridManager::WorldToHex(FVector WorldPos) const
{
	const FVector Local = WorldPos - GridOrigin;
	const float Q = (2.f / 3.f * Local.X) / HexSize;
	const float R = (-1.f / 3.f * Local.X + FMath::Sqrt(3.f) / 3.f * Local.Y) / HexSize;

	// Cube rounding
	float S = -Q - R;
	int32 RQ = FMath::RoundToInt(Q);
	int32 RR = FMath::RoundToInt(R);
	int32 RS = FMath::RoundToInt(S);

	const float DQ = FMath::Abs(RQ - Q);
	const float DR = FMath::Abs(RR - R);
	const float DS = FMath::Abs(RS - S);

	if (DQ > DR && DQ > DS) RQ = -RR - RS;
	else if (DR > DS)        RR = -RQ - RS;

	return FHexCoord(RQ, RR);
}

void UHexGridManager::SetDeploymentZone(EFactionID Faction, const TArray<FHexCoord>& Cells)
{
	DeploymentZones.Add(Faction, Cells);
}

bool UHexGridManager::IsInDeploymentZone(EFactionID Faction, FHexCoord Cell) const
{
	if (const TArray<FHexCoord>* Zone = DeploymentZones.Find(Faction))
	{
		return Zone->Contains(Cell);
	}
	return false;
}

TArray<FHexCoord> UHexGridManager::GetDeploymentZone(EFactionID Faction) const
{
	if (const TArray<FHexCoord>* Zone = DeploymentZones.Find(Faction))
	{
		return *Zone;
	}
	return {};
}

bool UHexGridManager::PlaceUnit(AUnitBase* Unit, FHexCoord Cell, EFactionID Faction)
{
	if (!Unit) return false;
	if (!IsInDeploymentZone(Faction, Cell)) return false;
	if (IsCellOccupied(Cell)) return false;

	OccupiedCells.Add(Cell, Unit);

	// Déplacer l'actor vers la position monde de la cellule
	const FVector WorldPos = HexToWorld(Cell);
	Unit->SetActorLocation(WorldPos);

	return true;
}

void UHexGridManager::RemoveUnit(FHexCoord Cell)
{
	OccupiedCells.Remove(Cell);
}

bool UHexGridManager::IsCellOccupied(FHexCoord Cell) const
{
	const TWeakObjectPtr<AUnitBase>* Ptr = OccupiedCells.Find(Cell);
	return Ptr && Ptr->IsValid();
}

AUnitBase* UHexGridManager::GetUnitAtCell(FHexCoord Cell) const
{
	if (const TWeakObjectPtr<AUnitBase>* Ptr = OccupiedCells.Find(Cell))
	{
		return Ptr->Get();
	}
	return nullptr;
}

TArray<FHexCoord> UHexGridManager::GetNeighbors(FHexCoord Cell) const
{
	TArray<FHexCoord> Result;
	for (const FHexCoord& Dir : HexDirections)
	{
		Result.Add(FHexCoord(Cell.Q + Dir.Q, Cell.R + Dir.R));
	}
	return Result;
}

int32 UHexGridManager::HexDistance(FHexCoord A, FHexCoord B) const
{
	return (FMath::Abs(A.Q - B.Q) + FMath::Abs(A.Q + A.R - B.Q - B.R)
		+ FMath::Abs(A.R - B.R)) / 2;
}
