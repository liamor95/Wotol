#include "TerritoryStateManager.h"

const FZoneState UTerritoryStateManager::EmptyZone = FZoneState();

void UTerritoryStateManager::RegisterZone(FName ZoneID, const FZoneState& InitialState)
{
	Zones.Add(ZoneID, InitialState);
}

void UTerritoryStateManager::UnregisterZone(FName ZoneID)
{
	Zones.Remove(ZoneID);
}

void UTerritoryStateManager::TickZoneCapture(
	FName ZoneID, EFactionID ControllingFaction, float DeltaSeconds)
{
	FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone) return;

	if (ControllingFaction == EFactionID::None)
	{
		// Aucune présence — décroissance lente vers grade 0
		if (Zone->CaptureProgress > 0.f)
		{
			Zone->CaptureProgress = FMath::Max(0.f,
				Zone->CaptureProgress - Zone->BaseCaptureRate * 0.5f * DeltaSeconds);
			OnCaptureProgressChanged.Broadcast(ZoneID, Zone->Owner, Zone->CaptureProgress);
		}
		return;
	}

	// Faction différente du propriétaire actuel = contestation
	if (ControllingFaction != Zone->Owner && Zone->Owner != EFactionID::None)
	{
		// Décroissance du grade du propriétaire avant de pouvoir recapturer
		Zone->CaptureProgress = FMath::Max(0.f,
			Zone->CaptureProgress - Zone->BaseCaptureRate * DeltaSeconds);

		OnCaptureProgressChanged.Broadcast(ZoneID, Zone->Owner, Zone->CaptureProgress);

		if (Zone->CaptureProgress <= 0.f && Zone->Grade > 0)
		{
			Zone->Grade--;
			if (Zone->Grade == 0)
			{
				// Zone perdue — neutre
				const EFactionID Previous = Zone->Owner;
				Zone->Owner = EFactionID::None;
				Zone->CapturingFaction = ControllingFaction;
				OnZoneLost.Broadcast(ZoneID, Previous);
			}
			else
			{
				// Grade réduit — toujours au propriétaire mais affaibli
				Zone->CaptureProgress = 100.f;
				OnZoneGradeChanged.Broadcast(ZoneID, Zone->Owner, Zone->Grade);
			}
		}
		return;
	}

	// Même faction (ou zone neutre) — progression
	HandleFactionChange(ZoneID, *Zone, ControllingFaction);

	const float Rate = ComputeEffectiveCaptureRate(ZoneID, *Zone, ControllingFaction);
	Zone->CaptureProgress = FMath::Min(100.f, Zone->CaptureProgress + Rate * DeltaSeconds);
	OnCaptureProgressChanged.Broadcast(ZoneID, ControllingFaction, Zone->CaptureProgress);

	if (Zone->CaptureProgress >= 100.f)
	{
		AdvanceGrade(ZoneID, *Zone, ControllingFaction);
	}
}

void UTerritoryStateManager::HandleFactionChange(
	FName ZoneID, FZoneState& Zone, EFactionID NewFaction)
{
	if (Zone.CapturingFaction == NewFaction) return;

	// Nouvelle faction commence à capturer — on préserve le grade existant
	// mais on repart de 0 dans la progression vers le prochain grade
	Zone.CapturingFaction = NewFaction;
	if (Zone.Owner == EFactionID::None)
	{
		Zone.CaptureProgress = 0.f;
	}
}

void UTerritoryStateManager::AdvanceGrade(
	FName ZoneID, FZoneState& Zone, EFactionID Faction)
{
	const int32 MaxGrade = 5;
	if (Zone.Grade >= MaxGrade) return;

	Zone.Grade++;
	Zone.Owner = Faction;
	Zone.CaptureProgress = 0.f;

	OnZoneGradeChanged.Broadcast(ZoneID, Faction, Zone.Grade);
}

float UTerritoryStateManager::ComputeEffectiveCaptureRate(
	FName ZoneID, const FZoneState& Zone, EFactionID Faction) const
{
	float Rate = Zone.BaseCaptureRate * Zone.TerrainCaptureModifier;

	// Appliquer les modificateurs de terrain actifs
	for (const FTerrainModifier& Mod : TerrainModifiers)
	{
		if (!Mod.AffectedZones.Contains(ZoneID)) continue;
		if (Mod.BenefitsFaction != EFactionID::None && Mod.BenefitsFaction != Faction) continue;
		Rate *= Mod.CaptureRateMult;
	}

	return Rate;
}

const FZoneState& UTerritoryStateManager::GetZoneState(FName ZoneID) const
{
	const FZoneState* Zone = Zones.Find(ZoneID);
	return Zone ? *Zone : EmptyZone;
}

int32 UTerritoryStateManager::GetZoneGrade(FName ZoneID) const
{
	const FZoneState* Zone = Zones.Find(ZoneID);
	return Zone ? Zone->Grade : 0;
}

EFactionID UTerritoryStateManager::GetZoneOwner(FName ZoneID) const
{
	const FZoneState* Zone = Zones.Find(ZoneID);
	return Zone ? Zone->Owner : EFactionID::None;
}

float UTerritoryStateManager::GetCaptureProgress(FName ZoneID) const
{
	const FZoneState* Zone = Zones.Find(ZoneID);
	return Zone ? Zone->CaptureProgress : 0.f;
}

int32 UTerritoryStateManager::GetZoneResourceOutput(FName ZoneID) const
{
	const FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone || Zone->Grade == 0 || Zone->ResourcePerGrade.IsEmpty()) return 0;

	const int32 GradeIdx = FMath::Clamp(Zone->Grade - 1, 0, Zone->ResourcePerGrade.Num() - 1);
	return Zone->ResourcePerGrade[GradeIdx];
}

TArray<FName> UTerritoryStateManager::GetAllZonesOwnedBy(EFactionID Faction) const
{
	TArray<FName> Result;
	for (const auto& Pair : Zones)
	{
		if (Pair.Value.Owner == Faction)
		{
			Result.Add(Pair.Key);
		}
	}
	return Result;
}

void UTerritoryStateManager::ConnectZones(FName ZoneA, FName ZoneB)
{
	if (ZoneA.IsNone() || ZoneB.IsNone() || ZoneA == ZoneB) return;
	FZoneState* A = Zones.Find(ZoneA);
	FZoneState* B = Zones.Find(ZoneB);
	if (!A || !B) return;
	A->AdjacentZoneIDs.AddUnique(ZoneB);
	B->AdjacentZoneIDs.AddUnique(ZoneA);
}

bool UTerritoryStateManager::AreZonesAdjacent(FName ZoneA, FName ZoneB) const
{
	const FZoneState* A = Zones.Find(ZoneA);
	return A && A->AdjacentZoneIDs.Contains(ZoneB);
}

bool UTerritoryStateManager::CanFactionContestZone(FName TargetZoneID, EFactionID Faction) const
{
	if (Faction == EFactionID::None) return false;
	const FZoneState* Target = Zones.Find(TargetZoneID);
	if (!Target) return false;
	if (Target->Owner == Faction) return true;
	for (const FName AdjacentID : Target->AdjacentZoneIDs)
	{
		const FZoneState* Adjacent = Zones.Find(AdjacentID);
		if (Adjacent && Adjacent->Owner == Faction) return true;
	}
	return false;
}

bool UTerritoryStateManager::CompleteConquestObjective(FName ZoneID, EFactionID Faction)
{
	FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone || !CanFactionContestZone(ZoneID, Faction)) return false;
	Zone->bConquestObjectiveCompleted = true;
	Zone->CapturingFaction = Faction;
	Zone->StrategicStatus = EZoneStrategicStatus::Contested;
	return true;
}

void UTerritoryStateManager::SetZoneThreat(
	FName ZoneID, EFactionID Attacker, float ReactionWindowSeconds)
{
	FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone || Zone->Owner == EFactionID::None || Attacker == EFactionID::None) return;
	Zone->StrategicStatus = EZoneStrategicStatus::Threatened;
	Zone->ThreateningFaction = Attacker;
	Zone->DefenseWindowRemainingSeconds = FMath::Max(0.f, ReactionWindowSeconds);
}

bool UTerritoryStateManager::TickZoneThreat(FName ZoneID, float DeltaSeconds)
{
	FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone || Zone->StrategicStatus != EZoneStrategicStatus::Threatened) return false;
	Zone->DefenseWindowRemainingSeconds = FMath::Max(0.f,
		Zone->DefenseWindowRemainingSeconds - FMath::Max(0.f, DeltaSeconds));
	if (Zone->DefenseWindowRemainingSeconds > 0.f) return false;
	NeutralizeZone(ZoneID);
	return true;
}

void UTerritoryStateManager::NeutralizeZone(FName ZoneID)
{
	FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone) return;
	const EFactionID Previous = Zone->Owner;
	Zone->Grade = 0;
	Zone->Owner = EFactionID::None;
	Zone->CapturingFaction = EFactionID::None;
	Zone->CaptureProgress = 0.f;
	Zone->StrategicStatus = EZoneStrategicStatus::Lost;
	Zone->ThreateningFaction = EFactionID::None;
	Zone->DefenseWindowRemainingSeconds = 0.f;
	Zone->bConquestObjectiveCompleted = false;
	if (Previous != EFactionID::None) OnZoneLost.Broadcast(ZoneID, Previous);
}

void UTerritoryStateManager::SetZoneFortification(FName ZoneID, int32 DefenseCount,
	int32 InDefenseTechnologyLevel, int32 InGarrisonUnits, int32 InGarrisonCapacity)
{
	FZoneState* Zone = Zones.Find(ZoneID);
	if (!Zone) return;
	Zone->InstalledDefenseCount = FMath::Clamp(DefenseCount, 0, 5);
	Zone->DefenseTechnologyLevel = FMath::Clamp(InDefenseTechnologyLevel, 1, 3);
	Zone->GarrisonCapacity = FMath::Max(0, InGarrisonCapacity);
	Zone->GarrisonUnits = FMath::Clamp(InGarrisonUnits, 0, Zone->GarrisonCapacity);
}

void UTerritoryStateManager::AddTerrainModifier(const FTerrainModifier& Modifier)
{
	// Remplace si le même ID existe déjà
	TerrainModifiers.RemoveAll([&](const FTerrainModifier& M)
	{
		return M.ModifierID == Modifier.ModifierID;
	});
	TerrainModifiers.Add(Modifier);

	// Mettre à jour le modificateur terrain sur les zones affectées
	for (FName ZoneID : Modifier.AffectedZones)
	{
		if (FZoneState* Zone = Zones.Find(ZoneID))
		{
			Zone->TerrainCaptureModifier = Modifier.CaptureRateMult;
		}
	}
}

void UTerritoryStateManager::RemoveTerrainModifier(FName ModifierID)
{
	TerrainModifiers.RemoveAll([&](const FTerrainModifier& M)
	{
		return M.ModifierID == ModifierID;
	});
}

float UTerritoryStateManager::GetMovementModifier(FName ZoneID, EFactionID Faction) const
{
	float Mult = 1.f;
	for (const FTerrainModifier& Mod : TerrainModifiers)
	{
		if (!Mod.AffectedZones.Contains(ZoneID)) continue;
		if (Mod.BenefitsFaction != EFactionID::None && Mod.BenefitsFaction != Faction) continue;
		Mult *= Mod.MovementSpeedMult;
	}
	return Mult;
}

void UTerritoryStateManager::TickTerrainModifiers(float DeltaSeconds)
{
	TerrainModifiers.RemoveAll([&](FTerrainModifier& Mod)
	{
		if (Mod.RemainingDuration < 0.f) return false; // permanent
		Mod.RemainingDuration -= DeltaSeconds;
		return Mod.RemainingDuration <= 0.f;
	});
}
