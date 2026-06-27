#include "FactionSynergySubsystem.h"
#include "Gameplay/Units/UnitBase.h"
#include "Core/FactionRegistrySubsystem.h"

FSynergyBonus UFactionSynergySubsystem::ComputeSynergyBonus(AUnitBase* Unit) const
{
	if (!Unit) return FSynergyBonus();

	switch (Unit->GetFaction())
	{
		case EFactionID::Aquiloris: return ComputeAquilorisBonus(Unit);
		case EFactionID::Noxeens:   return ComputeNoxeenBonus(Unit);
		default:                    return FSynergyBonus();
	}
}

FSynergyBonus UFactionSynergySubsystem::ComputeAquilorisBonus(AUnitBase* Unit) const
{
	FSynergyBonus Bonus;

	const int32 NearbyAllies = CountNearbyAllies(Unit, AquilorisSupportRadius);

	// Chaque allié proche apporte un bonus cumulatif (cap à 3 alliés)
	const int32 StackCount = FMath::Clamp(NearbyAllies, 0, 3);

	if (StackCount > 0)
	{
		// Coordination Aquiloris : +5% dégâts + -5% recharge par allié adjacent
		Bonus.DamageMultiplier  = 1.f + (StackCount * 0.05f);
		Bonus.CooldownReduction = StackCount * 0.05f;
		Bonus.MoraleResistance  = StackCount * 0.1f; // résistance aux malus moraux
	}

	return Bonus;
}

FSynergyBonus UFactionSynergySubsystem::ComputeNoxeenBonus(AUnitBase* Unit) const
{
	FSynergyBonus Bonus;

	if (!IsUnitInBioZone(Unit)) return Bonus;

	// Bonus zone bioluminescente Noxéens
	Bonus.DamageMultiplier  = 1.25f;  // +25% dégâts
	Bonus.SpeedMultiplier   = 1.15f;  // +15% vitesse
	Bonus.CooldownReduction = 0.1f;   // -10% cooldowns
	Bonus.MoraleResistance  = 0.2f;   // immunité partielle à la peur

	return Bonus;
}

int32 UFactionSynergySubsystem::CountNearbyAllies(AUnitBase* Unit, float Radius) const
{
	if (!Unit) return 0;

	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return 0;

	const FVector OwnLoc      = Unit->GetActorLocation();
	const float   RadiusSq    = Radius * Radius;
	const EFactionID OwnFaction = Unit->GetFaction();
	int32 Count = 0;

	for (AUnitBase* Ally : Registry->GetUnitsForFaction(OwnFaction))
	{
		if (!Ally || Ally == Unit || !Ally->IsAlive()) continue;
		if (FVector::DistSquared(OwnLoc, Ally->GetActorLocation()) <= RadiusSq)
		{
			Count++;
		}
	}

	return Count;
}

bool UFactionSynergySubsystem::IsUnitInBioZone(AUnitBase* Unit) const
{
	// Pour la démo : présence dans une zone bioluminescente active
	// La position exacte dans la zone est gérée par BP_Zone → SetBioluminescentZoneActive
	// Ici on vérifie simplement si au moins une zone est active et que l'unité est proche
	// La vérification spatiale fine est déléguée au Blueprint (overlap)
	return !ActiveBioluminescentZones.IsEmpty();
}

void UFactionSynergySubsystem::SetBioluminescentZoneActive(FName ZoneID, bool bActive)
{
	if (bActive)
	{
		ActiveBioluminescentZones.Add(ZoneID);
	}
	else
	{
		ActiveBioluminescentZones.Remove(ZoneID);
	}
}

bool UFactionSynergySubsystem::IsInBioluminescentZone(AUnitBase* Unit) const
{
	return IsUnitInBioZone(Unit);
}
