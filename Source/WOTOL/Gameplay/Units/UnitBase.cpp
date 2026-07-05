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
#include "Gameplay/Demo/WOTOLCoverStructure.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Factions/FactionSynergySubsystem.h"

// Rythme de bataille (démo) : combats plus longs + déplacements ralentis (eau).
// 0.42 dégâts -> ~2,5× plus d'échanges ; 0.55 vitesse -> approche/repli plus lents.
// Rythme global des dégâts (frottement de l'eau). PLUS BAS = échanges plus lents
// = batailles PLUS LONGUES, sans changer l'issue ni les pertes (tout le monde frappe
// ET encaisse proportionnellement moins). 0.20 vise des batailles de ~4-5 min.
float AUnitBase::GlobalDamageScale = 0.20f;
float AUnitBase::GlobalSpeedScale  = 0.55f;

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

	// MovementSpeed dans FUnitStats est un multiplicateur (1.0 = 600 UE units/s).
	// On applique le frottement de l'eau (GlobalSpeedScale) -> déplacements sous-marins
	// plus lents = batailles moins expédiées.
	GetCharacterMovement()->MaxWalkSpeed = 600.f * UnitData->Stats.MovementSpeed * GlobalSpeedScale;
}

float AUnitBase::GetHealthPercent() const
{
	if (!UnitData || UnitData->Stats.MaxHealth <= 0) return 0.f;
	return CurrentHealth / static_cast<float>(UnitData->Stats.MaxHealth);
}

float AUnitBase::TakeDamageFromUnit(float Damage, AUnitBase* InstigatorUnit)
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
					TEXT("Esquive"), FLinearColor(0.04f, 0.16f, 0.55f, 1.f))) // bleu FONCÉ
				N->SetFollow(Anchor, Jitter);
			return 0.f;
		}
		// Parade : le coup est bloqué (dégâts fortement réduits)
		if (UnitData->Stats.BlockChance > 0.f
			&& FMath::FRandRange(0.f, 100.f) < UnitData->Stats.BlockChance)
		{
			bBlocked = true;
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(GetWorld(), FxLoc,
					TEXT("Pare"), FLinearColor(0.45f, 0.22f, 0.f, 1.f))) // orange/brun FONCÉ
				N->SetFollow(Anchor, Jitter);
		}
	}

	// Appliquer la réduction de défense de CETTE unité (DEF%) + parade éventuelle
	const float DefReduction = UnitData ? (UnitData->Stats.DefensePercent / 100.f) : 0.f;
	float EffDamage          = Damage * (1.f - DefReduction);
	if (bBlocked) EffDamage *= 0.35f; // coup paré = 65% de dégâts en moins
	const float Applied      = FMath::Min(EffDamage, CurrentHealth);
	CurrentHealth           -= Applied;
	if (InstigatorUnit) InstigatorUnit->DamageDealt += Applied; // pour le résumé de bataille

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

	// AVEUGLÉ (flash Noxeflare) : précision quasi nulle -> rate le plus souvent.
	if (Now < BlindedUntil && FMath::FRand() < 0.75f)
	{
		AWOTOLDamageNumber::SpawnText(GetWorld(), GetActorLocation() + FVector(0, 0, 70.f),
			TEXT("Rate"), FLinearColor(0.6f, 0.6f, 0.65f, 1.f));
		OnAttackPerformed(Target);
		return;
	}

	// Dégâts de base : ATK/s × cooldown = dégâts par frappe. Réduits par le multiplicateur
	// global de rythme -> plus d'échanges, batailles plus longues.
	float BaseDamage = UnitData->Stats.AttackDPS * UnitData->Stats.AttackCooldown * GlobalDamageScale;

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

	// ── COUP CRITIQUE (TOUTES les unités, pas seulement le boss) ──
	// Chance de base + gros BONUS "dans le dos" : frapper l'ennemi par l'ARRIÈRE (donc
	// réussir à le contourner) augmente fortement la chance de critique. C'est la
	// récompense du flanquement / contournement des Aquilances & co.
	{
		float CritChance = 0.12f; // 12 % de base
		const FVector ToAtk  = (GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
		const FVector TgtFwd = Target->GetActorForwardVector().GetSafeNormal2D();
		const bool bBackstab = FVector::DotProduct(TgtFwd, ToAtk) < -0.2f; // attaquant DERRIÈRE la cible
		if (bBackstab) CritChance += 0.33f;                                 // +33 % dans le dos
		if (FMath::FRand() < CritChance)
		{
			BaseDamage *= bBackstab ? 2.2f : 1.7f;
			const FVector Loc = Target->GetActorLocation() + FVector(0.f, 0.f, 90.f);
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(GetWorld(), Loc,
					bBackstab ? TEXT("CRITIQUE DOS !") : TEXT("CRITIQUE !"),
					FLinearColor(1.f, 0.85f, 0.2f, 1.f)))
				N->SetFollow(Target->GetFloatingTextAnchor(), FVector(0.f, 0.f, 140.f));
		}
	}

	// COUVERTURE : pour un tir à distance, si une STRUCTURE de décor est entre le tireur et
	// la cible, le tir la frappe ELLE (et l'endommage) au lieu de la cible -> se cacher
	// derrière un pilier protège. (La mêlée n'est pas concernée : contact direct.)
	if (UnitData->Stats.AttackType == EUnitAttackType::Ranged)
	{
		const USceneComponent* FromA = GetFloatingTextAnchor();
		const USceneComponent* ToA   = Target->GetFloatingTextAnchor();
		const FVector From = (FromA ? FromA->GetComponentLocation() : GetActorLocation()) + FVector(0, 0, 40.f);
		const FVector To   = (ToA ? ToA->GetComponentLocation() : Target->GetActorLocation()) + FVector(0, 0, 40.f);
		FHitResult Hit;
		FCollisionObjectQueryParams ObjQ(ECC_WorldStatic); // uniquement le décor (ignore les unités)
		FCollisionQueryParams Q; Q.AddIgnoredActor(this);
		if (GetWorld()->LineTraceSingleByObjectType(Hit, From, To, ObjQ, Q))
		{
			if (AWOTOLCoverStructure* Cov = Cast<AWOTOLCoverStructure>(Hit.GetActor()))
			{
				Cov->TakeCoverDamage(BaseDamage, this); // le décor encaisse (peut s'effondrer)
				const FLinearColor Col = (GetFaction() == EFactionID::Aquiloris)
					? FLinearColor(0.3f, 0.95f, 1.f, 1.f) : FLinearColor(0.55f, 0.35f, 1.f, 1.f);
				AWOTOLProjectileTracer::Fire(GetWorld(), From, Hit.ImpactPoint, Col, 1.8f);
				OnAttackPerformed(Target);
				return; // cible protégée par la couverture
			}
		}
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
