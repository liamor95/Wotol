#include "AbilityBase.h"
#include "UnitBase.h"
#include "Engine/World.h"

bool UAbilityBase::CanActivate(AUnitBase* Caster) const
{
	if (!Caster || !Caster->IsAlive()) return false;
	return !IsOnCooldown();
}

bool UAbilityBase::Activate(AUnitBase* Caster, FVector TargetLocation, AUnitBase* TargetUnit)
{
	if (!CanActivate(Caster)) return false;

	if (UWorld* World = GetWorld())
	{
		LastActivationTime = World->GetTimeSeconds();
	}

	ExecuteAbility(Caster, TargetLocation, TargetUnit);
	OnAbilityActivated(Caster, TargetLocation, TargetUnit);
	return true;
}

float UAbilityBase::GetCooldownRemaining() const
{
	if (UWorld* World = GetWorld())
	{
		const float Elapsed = World->GetTimeSeconds() - LastActivationTime;
		return FMath::Max(0.f, Cooldown - Elapsed);
	}
	return 0.f;
}

void UAbilityBase::ExecuteAbility(AUnitBase* Caster, FVector TargetLocation, AUnitBase* TargetUnit)
{
	// Comportement de base : dégâts directs + soin
	if (TargetUnit && Damage > 0.f)
	{
		TargetUnit->TakeDamageFromUnit(Damage, Caster);
	}

	if (Caster && HealAmount > 0.f)
	{
		// Le heal passe par une valeur négative de dégâts (convention interne)
		Caster->TakeDamageFromUnit(-HealAmount, nullptr);
	}
}

UWorld* UAbilityBase::GetWorld() const
{
	if (UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}
