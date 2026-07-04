#include "UnitBase.h"
#include "UnitDataAsset.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VerticalLayerComponent.h"
#include "UnitMoraleComponent.h"
#include "AbilityComponent.h"
#include "AbilityBase.h"
#include "WOTOLProjectileBase.h"
#include "Gameplay/Demo/WOTOLProjectileTracer.h"
#include "Gameplay/Demo/WOTOLDamageNumber.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Factions/FactionSynergySubsystem.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = false;

	VerticalLayer = CreateDefaultSubobject<UVerticalLayerComponent>(TEXT("VerticalLayer"));
	AbilityComp   = CreateDefaultSubobject<UAbilityComponent>(TEXT("AbilityComp"));
	MoraleComp    = CreateDefaultSubobject<UUnitMoraleComponent>(TEXT("MoraleComp"));
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

	// La déroute (moral à 0) est exposée via MoraleComp->OnUnitRouting :
	// le Blueprint de l'unité peut s'y abonner pour changer l'animation.
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
	CurrentHealth = static_cast<float>(UnitData->Stats.MaxHealth);

	if (VerticalLayer)
	{
		VerticalLayer->DefaultLayer = UnitData->Stats.PreferredLayer;
	}

	if (MoraleComp)
	{
		MoraleComp->StartingMorale = UnitData->Stats.StartingMorale;
	}

	// AbilityClasses dans le DataAsset sont TSoftClassPtr<UObject> — charger et filtrer
	if (AbilityComp && UnitData->AbilityClasses.Num() > 0)
	{
		AbilityComp->AbilityClasses.Reset();
		for (const TSoftClassPtr<UObject>& SoftClass : UnitData->AbilityClasses)
		{
			if (UClass* Cls = SoftClass.LoadSynchronous())
			{
				if (Cls->IsChildOf(UAbilityBase::StaticClass()))
				{
					AbilityComp->AbilityClasses.Add(Cls);
				}
			}
		}
	}

	// MovementSpeed dans FUnitStats est un multiplicateur (1.0 = 600 UE units/s)
	GetCharacterMovement()->MaxWalkSpeed = 600.f * UnitData->Stats.MovementSpeed;
}

float AUnitBase::GetHealthPercent() const
{
	if (!UnitData || UnitData->Stats.MaxHealth <= 0) return 0.f;
	return CurrentHealth / static_cast<float>(UnitData->Stats.MaxHealth);
}

float AUnitBase::TakeDamageFromUnit(float Damage, AUnitBase* /*Instigator*/)
{
	if (!IsAlive()) return 0.f;

	if (Damage < 0.f)
	{
		// Soin
		const float MaxHP  = UnitData ? static_cast<float>(UnitData->Stats.MaxHealth) : CurrentHealth;
		const float Healed = FMath::Min(-Damage, MaxHP - CurrentHealth);
		CurrentHealth     += Healed;
		OnHealthChanged.Broadcast(CurrentHealth, MaxHP);
		return -Healed;
	}

	if (Damage <= 0.f) return 0.f;

	// ── ESQUIVE / PARADE (font durer les combats, tous les coups ne portent pas) ──
	bool bBlocked = false;
	if (UnitData)
	{
		// Texte accroché à l'unité (VisualRoot) -> suit l'unité et sa hauteur de couche.
		USceneComponent* Anchor = GetFloatingTextAnchor();
		const FVector FxLoc = Anchor ? Anchor->GetComponentLocation() : GetActorLocation();
		const FVector Jitter(FMath::FRandRange(-30.f, 30.f), FMath::FRandRange(-30.f, 30.f), 110.f);
		// Esquive : le coup rate complètement
		if (UnitData->Stats.DodgeChance > 0.f
			&& FMath::FRandRange(0.f, 100.f) < UnitData->Stats.DodgeChance)
		{
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(GetWorld(), FxLoc,
					TEXT("Esquive"), FLinearColor(0.5f, 0.9f, 1.f, 1.f)))
				N->SetFollow(Anchor, Jitter);
			return 0.f;
		}
		// Parade : le coup est bloqué (dégâts fortement réduits)
		if (UnitData->Stats.BlockChance > 0.f
			&& FMath::FRandRange(0.f, 100.f) < UnitData->Stats.BlockChance)
		{
			bBlocked = true;
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(GetWorld(), FxLoc,
					TEXT("Pare"), FLinearColor(1.f, 0.85f, 0.3f, 1.f)))
				N->SetFollow(Anchor, Jitter);
		}
	}

	// Appliquer la réduction de défense de CETTE unité (DEF%) + parade éventuelle
	const float DefReduction = UnitData ? (UnitData->Stats.DefensePercent / 100.f) : 0.f;
	float EffDamage          = Damage * (1.f - DefReduction);
	if (bBlocked) EffDamage *= 0.35f; // coup paré = 65% de dégâts en moins
	const float Applied      = FMath::Min(EffDamage, CurrentHealth);
	CurrentHealth           -= Applied;

	const float MaxHP = UnitData ? static_cast<float>(UnitData->Stats.MaxHealth) : CurrentHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHP);

	// Choc moral proportionnel (perte > 20% PV max = malus moral)
	if (MoraleComp && UnitData)
	{
		const float DamageRatio = EffDamage / static_cast<float>(UnitData->Stats.MaxHealth);
		if (DamageRatio > 0.2f)
		{
			MoraleComp->ApplyMoraleHit(DamageRatio * 20.f);
		}
	}

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return Applied;
}

void AUnitBase::PerformAttack(AUnitBase* Target)
{
	if (!Target || !Target->IsAlive() || !UnitData) return;

	// Unité en déroute = ne peut pas attaquer
	if (MoraleComp && MoraleComp->IsRouting()) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < UnitData->Stats.AttackCooldown) return;

	LastAttackTime = Now;

	// Dégâts de base : ATK/s × cooldown = dégâts par frappe
	float BaseDamage = UnitData->Stats.AttackDPS * UnitData->Stats.AttackCooldown;

	// Appliquer le multiplicateur vertical (attaque ascendante depuis Hadal = ×3)
	if (VerticalLayer && Target->VerticalLayer)
	{
		BaseDamage *= UVerticalLayerComponent::GetAttackDamageMultiplier(
			VerticalLayer->GetCurrentLayer(),
			Target->VerticalLayer->GetCurrentLayer());
	}

	// Appliquer les synergies de faction (Aquiloris coordination, Noxéens bio-zones)
	if (UFactionSynergySubsystem* Synergy =
			GetWorld()->GetSubsystem<UFactionSynergySubsystem>())
	{
		const FSynergyBonus Bonus = Synergy->ComputeSynergyBonus(this);
		BaseDamage *= Bonus.DamageMultiplier;
	}

	// À distance AVEC projectile défini -> tir ; sinon (mêlée OU distance sans
	// projectile assigné) -> dégâts directs instantanés. Évite le "zéro dégât"
	// quand aucune classe de projectile n'est configurée.
	if (UnitData->Stats.AttackType == EUnitAttackType::Ranged && !UnitData->ProjectileClass.IsNull())
	{
		SpawnProjectileToward(Target, BaseDamage);
	}
	else
	{
		Target->TakeDamageFromUnit(BaseDamage, this);

		// À distance sans classe de projectile : on tire une boule VISUELLE (greybox)
		// pour voir le tir (Aquisphères, Noxeblast). Cosmétique uniquement.
		if (UnitData->Stats.AttackType == EUnitAttackType::Ranged)
		{
			const FLinearColor Col = (GetFaction() == EFactionID::Aquiloris)
				? FLinearColor(0.3f, 0.95f, 1.f, 1.f)    // cyan vif Aquiloris
				: FLinearColor(0.55f, 0.35f, 1.f, 1.f);  // violet vif Noxéen (Noxeblast)
			// Part et arrive à la position VISUELLE (couche verticale comprise).
			const USceneComponent* FromA = GetFloatingTextAnchor();
			const USceneComponent* ToA   = Target->GetFloatingTextAnchor();
			const FVector FromLoc = (FromA ? FromA->GetComponentLocation() : GetActorLocation()) + FVector(0, 0, 40.f);
			const FVector ToLoc   = (ToA ? ToA->GetComponentLocation() : Target->GetActorLocation()) + FVector(0, 0, 40.f);
			AWOTOLProjectileTracer::Fire(GetWorld(), FromLoc, ToLoc, Col, 1.8f);
		}
	}

	OnAttackPerformed(Target);
}

void AUnitBase::SpawnProjectileToward(AUnitBase* Target, float OverrideDamage)
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
		Proj->InitProjectile(this, Target, OverrideDamage);
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
