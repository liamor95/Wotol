#include "AbilityBase.h"
#include "UnitBase.h"
#include "UnitDataAsset.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Demo/DemoFlowSubsystem.h"
#include "Gameplay/Demo/WOTOLZoneTelegraph.h"

// EUnitRole (Units) et EDemoUnitCategory (Demo) ont les mêmes 6 valeurs mais pas le même
// ordre/index -> conversion explicite requise, PAS de static_cast (ferait correspondre les
// mauvaises catégories, ex. Mythique <-> Speciale).
static EDemoUnitCategory RoleToAbilityCategory(EUnitRole Role)
{
	switch (Role)
	{
		case EUnitRole::Chef:       return EDemoUnitCategory::Chef;
		case EUnitRole::Infanterie: return EDemoUnitCategory::Infanterie;
		case EUnitRole::Montee:     return EDemoUnitCategory::Montee;
		case EUnitRole::Distance:   return EDemoUnitCategory::Distance;
		case EUnitRole::Speciale:   return EDemoUnitCategory::Speciale;
		case EUnitRole::Mythique:   return EDemoUnitCategory::Mythique;
	}
	return EDemoUnitCategory::Infanterie;
}

// Rassemble les cibles ennemies touchées par la FORME réelle de la compétence (Grade 1+
// uniquement, cf. ExecuteAbility). Mono/Aura gardent le comportement historique (cible unique
// déjà résolue par l'appelant) — l'Aura (buff allié continu, ex. Léviaphénix) est gérée par un
// système de Tick dédié ailleurs, pas par ce chemin de dégâts, donc volontairement pas touchée
// ici pour ne pas entrer en conflit avec lui.
static TArray<AUnitBase*> GatherAbilityTargets(AUnitBase* Caster, const FVector& TargetLocation,
	AUnitBase* PrimaryTarget, EAbilityZoneType ZoneType)
{
	TArray<AUnitBase*> Out;
	if (!Caster) return Out;

	if (ZoneType == EAbilityZoneType::Mono || ZoneType == EAbilityZoneType::Aura)
	{
		if (PrimaryTarget) Out.Add(PrimaryTarget);
		return Out;
	}

	UWorld* World = Caster->GetWorld();
	UFactionRegistrySubsystem* Reg = World ? World->GetSubsystem<UFactionRegistrySubsystem>() : nullptr;
	if (!Reg)
	{
		if (PrimaryTarget) Out.Add(PrimaryTarget);
		return Out;
	}

	const EFactionID EnemyFac = (Caster->GetFaction() == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;
	const FVector CasterLoc = Caster->GetActorLocation();
	const FVector Fwd = Caster->GetActorForwardVector();
	// Zones circulaires centrées sur la cible visée (comme un tir/impact précis) ; formes
	// directionnelles (Cône/ChargeLigne/Souffle) centrées sur le lanceur, orientées devant lui.
	const bool bDirectional = (ZoneType == EAbilityZoneType::Cone
		|| ZoneType == EAbilityZoneType::ChargeLigne || ZoneType == EAbilityZoneType::Souffle);
	const FVector Origin = bDirectional ? CasterLoc
		: (PrimaryTarget ? PrimaryTarget->GetActorLocation() : TargetLocation);

	// Rayons/angles PROVISOIRS et éditables (pas de valeur GDD chiffrée pour ces formes) —
	// choisis pour rester cohérents avec les portées d'unité existantes (1 à 5, ~120 à 600 UE).
	float Radius = 350.f;
	float ConeHalfAngleDeg = 0.f; // 0 = zone circulaire pure, pas de test d'angle
	switch (ZoneType)
	{
		case EAbilityZoneType::PetiteZone:  Radius = 350.f; break;
		case EAbilityZoneType::Zone:        Radius = 600.f; break;
		case EAbilityZoneType::GrandeZone:  Radius = 900.f; break;
		case EAbilityZoneType::Cone:        Radius = 700.f; ConeHalfAngleDeg = 45.f; break;
		case EAbilityZoneType::ChargeLigne: Radius = 900.f; ConeHalfAngleDeg = 15.f; break;
		case EAbilityZoneType::Souffle:     Radius = 800.f; ConeHalfAngleDeg = 20.f; break;
		default: break;
	}

	for (AUnitBase* Enemy : Reg->GetUnitsForFaction(EnemyFac))
	{
		if (!Enemy || !Enemy->IsAlive()) continue;
		const FVector ToEnemy = Enemy->GetActorLocation() - Origin;
		if (ToEnemy.SizeSquared() > FMath::Square(Radius)) continue;
		if (bDirectional && ConeHalfAngleDeg > 0.f)
		{
			const FVector ToEnemyFromCaster = Enemy->GetActorLocation() - CasterLoc;
			const FVector Dir = ToEnemyFromCaster.GetSafeNormal();
			const float CosHalf = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngleDeg));
			if (FVector::DotProduct(Fwd, Dir) < CosHalf) continue;
		}
		Out.Add(Enemy);
	}

	// La cible déjà résolue par l'appelant reste toujours touchée, même si un arrondi de
	// portée/angle l'exclurait de la zone stricte (évite qu'un clic direct sur la cible "rate").
	if (PrimaryTarget && !Out.Contains(PrimaryTarget)) Out.Add(PrimaryTarget);
	return Out;
}

// Fait apparaître le disque holographique (AWOTOLZoneTelegraph) à l'endroit visé. bBlind=true
// pour un voile de CONTRÔLE réel (Aquilombres Axe 2) ; false pour un simple aperçu (Noxeflare).
static void SpawnZoneTelegraph(AUnitBase* Caster, const FVector& Center, float Radius,
	float Duration, bool bBlind)
{
	UWorld* World = Caster ? Caster->GetWorld() : nullptr;
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Instigator = Caster;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWOTOLZoneTelegraph* Telegraph = World->SpawnActor<AWOTOLZoneTelegraph>(
		AWOTOLZoneTelegraph::StaticClass(), Center, FRotator::ZeroRotator, Params);
	if (!Telegraph) return;

	Telegraph->Radius = Radius;
	Telegraph->Duration = Duration;
	Telegraph->Color = bBlind ? FLinearColor(0.08f, 0.02f, 0.12f, 1.f) // voile sombre (encre)
		: FFactionColors::Get(Caster->GetFaction()); // aperçu = couleur de faction
	Telegraph->bAppliesBlindToEnemies = bBlind;
	Telegraph->CasterFaction = Caster->GetFaction();
}

bool UAbilityBase::CanActivate(AUnitBase* Caster) const
{
	if (!Caster || !Caster->IsAlive()) return false;
	return !IsOnCooldown();
}

bool UAbilityBase::Activate(AUnitBase* Caster, FVector TargetLocation, AUnitBase* TargetUnit)
{
	if (!CanActivate(Caster)) return false;

	UWorld* World = GetWorld();
	if (World)
	{
		LastActivationTime = World->GetTimeSeconds();
	}

	// TelegraphDuration > 0 (Noxeflare uniquement, cf. UUnitDataAsset::bAbilityHasTelegraph) :
	// aperçu holographique de zone AVANT le coup réel, puis exécution différée. Comportement
	// historique (instantané) préservé pour TOUTE unité qui ne configure pas ce champ.
	if (TelegraphDuration > 0.f && World && Caster)
	{
		const FVector Center = TargetUnit ? TargetUnit->GetActorLocation() : TargetLocation;
		SpawnZoneTelegraph(Caster, Center, FMath::Max(200.f, Range), TelegraphDuration, false);

		TWeakObjectPtr<UAbilityBase> WeakThis(this);
		TWeakObjectPtr<AUnitBase> WeakCaster(Caster);
		TWeakObjectPtr<AUnitBase> WeakTarget(TargetUnit);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda(
			[WeakThis, WeakCaster, TargetLocation, WeakTarget]()
			{
				if (!WeakThis.IsValid() || !WeakCaster.IsValid() || !WeakCaster->IsAlive()) return;
				WeakThis->ExecuteAbility(WeakCaster.Get(), TargetLocation, WeakTarget.Get());
				WeakThis->OnAbilityActivated(WeakCaster.Get(), TargetLocation, WeakTarget.Get());
			}), TelegraphDuration, false);
		return true;
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
	// Forme réelle (cône/zone/aura... touchant potentiellement plusieurs cibles) UNIQUEMENT si
	// le Grade de compétence de la catégorie du lanceur est >= 1 ET qu'un axe est choisi
	// (système Grade/Axe, cf. DemoFlowSubsystem::GetUnitGrade/GetUnitAxis). Au Grade 0 (état
	// par défaut, phases 1 & 2), ce bloc ne s'active JAMAIS -> comportement mono-cible + soin
	// inchangé à l'octet près, l'équilibrage déjà validé n'est pas affecté.
	bool bUseZoneEffect = false;
	EAbilityZoneType ZoneType = EAbilityZoneType::Mono;
	int32 ChosenAxis = 0;
	if (Caster && Caster->GetUnitData())
	{
		if (UGameInstance* GI = Caster->GetGameInstance())
		{
			if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
			{
				const EDemoUnitCategory Cat = RoleToAbilityCategory(Caster->GetUnitData()->Role);
				ChosenAxis = Demo->GetUnitAxis(Cat);
				if (Demo->GetUnitGrade(Cat) >= 1 && ChosenAxis != 0)
				{
					bUseZoneEffect = true;
					ZoneType = Caster->GetUnitData()->Stats.AbilityZoneType;
				}
			}
		}
	}

	if (bUseZoneEffect)
	{
		if (Damage > 0.f)
		{
			for (AUnitBase* T : GatherAbilityTargets(Caster, TargetLocation, TargetUnit, ZoneType))
			{
				if (T) T->TakeDamageFromUnit(Damage, Caster);
			}
		}

		// Voile sombre de CONTRÔLE (Aquilombres "Ombres Projetées", Axe 2 uniquement) — ~3x la
		// taille de l'unité, réduit réellement la précision/visibilité ennemie pendant sa durée.
		if (ChosenAxis == 2 && Caster->GetUnitData()->bAxisTwoSpawnsShadowVeil)
		{
			const FVector Center = TargetUnit ? TargetUnit->GetActorLocation() : TargetLocation;
			SpawnZoneTelegraph(Caster, Center, 700.f, 4.f, /*bBlind=*/true);
		}
	}
	else if (TargetUnit && Damage > 0.f)
	{
		// Comportement de base (Grade 0) : dégâts directs mono-cible.
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
