#include "OceanCurrentSubsystem.h"

void UOceanCurrentSubsystem::Regenerate()
{
	// Sens ALÉATOIRE (n'importe quelle direction horizontale) + intensité aléatoire.
	// Intensité volontairement JOUABLE : assez forte pour se sentir, pas au point de
	// balayer instantanément une unité placée en couche haute (elle peut résister/avancer).
	const float Yaw = FMath::FRandRange(0.f, 360.f);
	Direction = FRotator(0.f, Yaw, 0.f).Vector();
	Strength  = FMath::FRandRange(60.f, 135.f); // unités/s au sommet
}

float UOceanCurrentSubsystem::GetFactorAt(float LayerZ) const
{
	if (Strength <= 1.f) return 0.f;
	return FMath::Clamp((LayerZ - LowZ) / (TopZ - LowZ), 0.f, 1.f);
}

FVector UOceanCurrentSubsystem::GetDriftAt(float LayerZ) const
{
	const float F = GetFactorAt(LayerZ);
	if (F <= 0.f) return FVector::ZeroVector;
	return Direction * (Strength * F);
}
