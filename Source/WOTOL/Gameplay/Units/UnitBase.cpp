#include "UnitBase.h"
#include "UnitDataAsset.h"
#include "VerticalLayerComponent.h"
#include "Core/FactionRegistrySubsystem.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = false;

	VerticalLayer = CreateDefaultSubobject<UVerticalLayerComponent>(TEXT("VerticalLayer"));
}

void AUnitBase::BeginPlay()
{
	Super::BeginPlay();
	InitFromDataAsset();

	if (UFactionRegistrySubsystem* Registry =
			GetWorld()->GetSubsystem<UFactionRegistrySubsystem>())
	{
		Registry->RegisterUnit(this, Faction);
	}
}

void AUnitBase::EndPlay(const EEndPlayReason::Type Reason)
{
	if (UFactionRegistrySubsystem* Registry =
			GetWorld()->GetSubsystem<UFactionRegistrySubsystem>())
	{
		Registry->UnregisterUnit(this, Faction);
	}

	Super::EndPlay(Reason);
}

void AUnitBase::InitFromDataAsset()
{
	if (!UnitData) return;

	Faction       = UnitData->Faction;
	CurrentHealth = UnitData->MaxHealth;

	if (VerticalLayer)
	{
		VerticalLayer->DefaultLayer = UnitData->PreferredLayer;
	}
}

float AUnitBase::GetHealthPercent() const
{
	if (!UnitData || UnitData->MaxHealth <= 0.f) return 0.f;
	return CurrentHealth / UnitData->MaxHealth;
}

float AUnitBase::TakeDamageFromUnit(float Damage, AUnitBase* /*Instigator*/)
{
	if (!IsAlive() || Damage <= 0.f) return 0.f;

	const float Applied = FMath::Min(Damage, CurrentHealth);
	CurrentHealth -= Applied;

	OnHealthChanged.Broadcast(CurrentHealth, UnitData ? UnitData->MaxHealth : CurrentHealth);

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return Applied;
}

void AUnitBase::Die()
{
	CurrentHealth = 0.f;
	OnUnitDied.Broadcast(this);
	// La destruction effective est déléguée au Blueprint (animation de mort, FX, timer)
}
