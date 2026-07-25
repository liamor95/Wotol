#include "AbilityComponent.h"
#include "UnitBase.h"

UAbilityComponent::UAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	for (TSubclassOf<UAbilityBase> AbilityClass : AbilityClasses)
	{
		if (AbilityClass)
		{
			UAbilityBase* Instance = NewObject<UAbilityBase>(this, AbilityClass);
			Abilities.Add(Instance);
		}
	}
}

bool UAbilityComponent::ActivateAbilityByIndex(
	int32 Index, FVector TargetLocation, AUnitBase* TargetUnit)
{
	UAbilityBase* Ability = GetAbilityByIndex(Index);
	if (!Ability) return false;

	AUnitBase* Owner = Cast<AUnitBase>(GetOwner());
	if (!Owner) return false;

	const bool bSuccess = Ability->Activate(Owner, TargetLocation, TargetUnit);
	if (bSuccess) OnAbilityActivated.Broadcast(Ability);
	return bSuccess;
}

bool UAbilityComponent::ActivateAbilityByClass(
	TSubclassOf<UAbilityBase> AbilityClass, FVector TargetLocation, AUnitBase* TargetUnit)
{
	for (int32 i = 0; i < Abilities.Num(); ++i)
	{
		if (Abilities[i] && Abilities[i]->IsA(AbilityClass))
		{
			return ActivateAbilityByIndex(i, TargetLocation, TargetUnit);
		}
	}
	return false;
}

UAbilityBase* UAbilityComponent::GetAbilityByIndex(int32 Index) const
{
	if (!Abilities.IsValidIndex(Index)) return nullptr;
	return Abilities[Index];
}
