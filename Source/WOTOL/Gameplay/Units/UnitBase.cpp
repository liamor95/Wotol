#include "UnitBase.h"
#include "UnitDataAsset.h"
#include "VerticalLayerComponent.h"
#include "AbilityComponent.h"
#include "WOTOLProjectileBase.h"
#include "Core/FactionRegistrySubsystem.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = false;

	VerticalLayer = CreateDefaultSubobject<UVerticalLayerComponent>(TEXT("VerticalLayer"));
	AbilityComp   = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComp"));
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

	if (AbilityComp)
	{
		AbilityComp->AbilityClasses = UnitData->DefaultAbilities;
	}

	GetCharacterMovement()->MaxWalkSpeed = UnitData->MovementSpeed;
}

float AUnitBase::GetHealthPercent() const
{
	if (!UnitData || UnitData->MaxHealth <= 0.f) return 0.f;
	return CurrentHealth / UnitData->MaxHealth;
}

float AUnitBase::TakeDamageFromUnit(float Damage, AUnitBase* /*Instigator*/)
{
	if (!IsAlive()) return 0.f;

	if (Damage < 0.f)
	{
		// Soin
		const float MaxHP   = UnitData ? UnitData->MaxHealth : CurrentHealth;
		const float Healed  = FMath::Min(-Damage, MaxHP - CurrentHealth);
		CurrentHealth      += Healed;
		OnHealthChanged.Broadcast(CurrentHealth, MaxHP);
		return -Healed;
	}

	if (Damage <= 0.f) return 0.f;

	const float Applied = FMath::Min(Damage, CurrentHealth);
	CurrentHealth -= Applied;

	OnHealthChanged.Broadcast(CurrentHealth, UnitData ? UnitData->MaxHealth : CurrentHealth);

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return Applied;
}

void AUnitBase::PerformAttack(AUnitBase* Target)
{
	if (!Target || !Target->IsAlive() || !UnitData) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < UnitData->AttackCooldown) return;

	LastAttackTime = Now;

	if (UnitData->AttackType == EUnitAttackType::Ranged)
	{
		SpawnProjectileToward(Target);
	}
	else
	{
		Target->TakeDamageFromUnit(UnitData->AttackDamage, this);
	}

	OnAttackPerformed(Target);
}

void AUnitBase::SpawnProjectileToward(AUnitBase* Target)
{
	if (!UnitData || UnitData->ProjectileClass.IsNull()) return;

	TSubclassOf<AWOTOLProjectileBase> ProjClass = UnitData->ProjectileClass.LoadSynchronous();
	if (!ProjClass) return;

	const FVector SpawnLoc = GetActorLocation() + GetActorForwardVector() * 80.f;
	FActorSpawnParameters Params;
	Params.Instigator = this;

	AWOTOLProjectileBase* Proj = GetWorld()->SpawnActor<AWOTOLProjectileBase>(
		ProjClass, SpawnLoc, FRotator::ZeroRotator, Params);

	if (Proj)
	{
		Proj->InitProjectile(this, Target, UnitData->AttackDamage);
	}
}

void AUnitBase::SetSelected(bool bNewSelected)
{
	if (bSelected == bNewSelected) return;
	bSelected = bNewSelected;
	OnUnitSelected.Broadcast(bSelected);
	OnSelectionChanged(bSelected);
}

void AUnitBase::Die()
{
	CurrentHealth = 0.f;
	OnUnitDied.Broadcast(this);
	SetSelected(false);
	// Destruction effective déléguée au Blueprint (animation de mort + timer)
}
