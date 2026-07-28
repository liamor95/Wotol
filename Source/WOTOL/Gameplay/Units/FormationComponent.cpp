#include "FormationComponent.h"
#include "UnitBase.h"
#include "Gameplay/AI/AIAdaptiveController.h"

UFormationComponent::UFormationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UFormationComponent::SetFormation(EFormationType NewFormation)
{
	CurrentFormation = NewFormation;
}

void UFormationComponent::AddUnitToFormation(AUnitBase* Unit)
{
	if (!Unit) return;
	FormationUnits.AddUnique(TWeakObjectPtr<AUnitBase>(Unit));
}

void UFormationComponent::RemoveUnitFromFormation(AUnitBase* Unit)
{
	FormationUnits.RemoveAll([Unit](const TWeakObjectPtr<AUnitBase>& W)
	{
		return W.Get() == Unit;
	});
}

void UFormationComponent::ClearFormation()
{
	FormationUnits.Empty();
	CachedSlots.Empty();
}

void UFormationComponent::UpdateFormationPositions(
	FVector LeaderDestination, FRotator LeaderFacing)
{
	const int32 Count = FormationUnits.Num();
	if (Count == 0) return;

	switch (CurrentFormation)
	{
		case EFormationType::Line:
			CachedSlots = ComputeLineSlots(LeaderDestination, LeaderFacing, Count);
			break;
		case EFormationType::Wedge:
			CachedSlots = ComputeWedgeSlots(LeaderDestination, LeaderFacing, Count);
			break;
		case EFormationType::DefensiveSquare:
			CachedSlots = ComputeSquareSlots(LeaderDestination, LeaderFacing, Count);
			break;
		case EFormationType::Loose:
			CachedSlots = ComputeLooseSlots(LeaderDestination, LeaderFacing, Count);
			break;
		case EFormationType::Column:
			CachedSlots = ComputeColumnSlots(LeaderDestination, LeaderFacing, Count);
			break;
		default:
			CachedSlots.SetNum(Count);
			for (int32 i = 0; i < Count; ++i) CachedSlots[i] = LeaderDestination;
			break;
	}

	// Envoyer chaque unité vers son slot
	for (int32 i = 0; i < FormationUnits.Num(); ++i)
	{
		AUnitBase* Unit = FormationUnits[i].Get();
		if (!Unit || !Unit->IsAlive()) continue;

		if (i < CachedSlots.Num())
		{
			if (AAIAdaptiveController* AIC =
					Cast<AAIAdaptiveController>(Unit->GetController()))
			{
				AIC->IssueOrder_Move(CachedSlots[i]);
			}
		}
	}
}

FVector UFormationComponent::GetSlotDestination(int32 UnitIndex) const
{
	if (CachedSlots.IsValidIndex(UnitIndex))
	{
		return CachedSlots[UnitIndex];
	}
	return FVector::ZeroVector;
}

float UFormationComponent::GetFormationDefenseBonus() const
{
	if (!IsFormationIntact()) return 0.f;

	switch (CurrentFormation)
	{
		case EFormationType::Line:            return 5.f;
		case EFormationType::DefensiveSquare: return 20.f;
		case EFormationType::Wedge:           return 0.f;
		case EFormationType::Loose:           return -5.f;  // moins de DEF, moins d'AoE
		case EFormationType::Column:          return -10.f;
		default:                              return 0.f;
	}
}

float UFormationComponent::GetFormationSpeedMultiplier() const
{
	switch (CurrentFormation)
	{
		case EFormationType::Column:          return 1.3f;
		case EFormationType::Loose:           return 1.1f;
		case EFormationType::Line:            return 1.0f;
		case EFormationType::Wedge:           return 0.9f;
		case EFormationType::DefensiveSquare: return 0.7f;
		default:                              return 1.0f;
	}
}

float UFormationComponent::GetAoEDamageReduction() const
{
	return (CurrentFormation == EFormationType::Loose) ? 0.3f : 0.f;
}

bool UFormationComponent::IsFormationIntact() const
{
	for (int32 i = 0; i < FormationUnits.Num(); ++i)
	{
		const AUnitBase* Unit = FormationUnits[i].Get();
		if (!Unit || !Unit->IsAlive()) continue;

		if (!CachedSlots.IsValidIndex(i)) return false;

		const float Dist = FVector::Dist2D(Unit->GetActorLocation(), CachedSlots[i]);
		if (Dist > SlotTolerance) return false;
	}
	return true;
}

// ─── Intégration contrôleur (26/07/2026) ─────────────────────────────────────
// Wrappers purs : réutilisent les MÊMES fonctions Compute*Slots que UpdateFormationPositions
// (aucune duplication de la géométrie), juste sans l'orchestration d'ordres/état de composant.

TArray<FVector> UFormationComponent::ComputeSlotsForType(
	EFormationType Type, FVector Origin, FRotator Facing, int32 Count) const
{
	if (Count <= 0) return TArray<FVector>();
	switch (Type)
	{
		case EFormationType::Line:            return ComputeLineSlots(Origin, Facing, Count);
		case EFormationType::Wedge:           return ComputeWedgeSlots(Origin, Facing, Count);
		case EFormationType::DefensiveSquare: return ComputeSquareSlots(Origin, Facing, Count);
		case EFormationType::Loose:           return ComputeLooseSlots(Origin, Facing, Count);
		case EFormationType::Column:          return ComputeColumnSlots(Origin, Facing, Count);
		default:
		{
			TArray<FVector> Slots; Slots.Init(Origin, Count);
			return Slots;
		}
	}
}

float UFormationComponent::GetFormationDefenseBonusForType(EFormationType Type)
{
	// Mêmes valeurs que GetFormationDefenseBonus() (qui suppose IsFormationIntact()==true) —
	// c'est à l'appelant de décider QUAND l'unité est "en formation" (cf. WOTOLDemoUnit,
	// vérifie la distance à son slot assigné avant d'appliquer ce bonus).
	switch (Type)
	{
		case EFormationType::Line:            return 5.f;
		case EFormationType::DefensiveSquare: return 20.f;
		case EFormationType::Wedge:           return 0.f;
		case EFormationType::Loose:           return -5.f;
		case EFormationType::Column:          return -10.f;
		default:                              return 0.f;
	}
}

float UFormationComponent::GetFormationSpeedMultiplierForType(EFormationType Type)
{
	switch (Type)
	{
		case EFormationType::Column:          return 1.3f;
		case EFormationType::Loose:           return 1.1f;
		case EFormationType::Line:            return 1.0f;
		case EFormationType::Wedge:           return 0.9f;
		case EFormationType::DefensiveSquare: return 0.7f;
		default:                              return 1.0f;
	}
}

// ─── Calcul des slots par formation ──────────────────────────────────────────

TArray<FVector> UFormationComponent::ComputeLineSlots(
	FVector Origin, FRotator Facing, int32 Count) const
{
	TArray<FVector> Slots;
	const FVector Right = FRotationMatrix(Facing).GetScaledAxis(EAxis::Y);
	const float HalfWidth = (Count - 1) * UnitSpacing * 0.5f;

	for (int32 i = 0; i < Count; ++i)
	{
		Slots.Add(Origin + Right * (i * UnitSpacing - HalfWidth));
	}
	return Slots;
}

TArray<FVector> UFormationComponent::ComputeWedgeSlots(
	FVector Origin, FRotator Facing, int32 Count) const
{
	// Pointe vers l'ennemi, ailes en retrait
	TArray<FVector> Slots;
	const FVector Forward = FRotationMatrix(Facing).GetScaledAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(Facing).GetScaledAxis(EAxis::Y);

	Slots.Add(Origin); // pointe
	for (int32 i = 1; i < Count; ++i)
	{
		const int32 Row  = (i + 1) / 2;
		const float Side = (i % 2 == 0) ? -1.f : 1.f;
		Slots.Add(Origin - Forward * (Row * UnitSpacing) + Right * (Side * Row * UnitSpacing));
	}
	return Slots;
}

TArray<FVector> UFormationComponent::ComputeSquareSlots(
	FVector Origin, FRotator Facing, int32 Count) const
{
	// Carré défensif : unités réparties sur 4 côtés
	TArray<FVector> Slots;
	const FVector Forward = FRotationMatrix(Facing).GetScaledAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(Facing).GetScaledAxis(EAxis::Y);
	const float Half = UnitSpacing;

	const TArray<FVector> Corners = {
		Origin + Forward * Half + Right * Half,
		Origin + Forward * Half - Right * Half,
		Origin - Forward * Half + Right * Half,
		Origin - Forward * Half - Right * Half
	};

	for (int32 i = 0; i < Count; ++i)
	{
		Slots.Add(Corners[i % Corners.Num()]);
	}
	return Slots;
}

TArray<FVector> UFormationComponent::ComputeLooseSlots(
	FVector Origin, FRotator Facing, int32 Count) const
{
	// Formation lâche : espacement doublé en diagonal
	TArray<FVector> Slots;
	const FVector Forward = FRotationMatrix(Facing).GetScaledAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(Facing).GetScaledAxis(EAxis::Y);
	const float Gap = UnitSpacing * 1.8f;

	int32 PerRow = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
	for (int32 i = 0; i < Count; ++i)
	{
		const int32 Row = i / PerRow;
		const int32 Col = i % PerRow;
		Slots.Add(Origin
			+ Forward * (Row * -Gap)
			+ Right * ((Col - PerRow * 0.5f) * Gap));
	}
	return Slots;
}

TArray<FVector> UFormationComponent::ComputeColumnSlots(
	FVector Origin, FRotator Facing, int32 Count) const
{
	TArray<FVector> Slots;
	const FVector Forward = FRotationMatrix(Facing).GetScaledAxis(EAxis::X);

	for (int32 i = 0; i < Count; ++i)
	{
		Slots.Add(Origin - Forward * (i * UnitSpacing));
	}
	return Slots;
}
