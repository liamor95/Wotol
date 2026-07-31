#include "UnitBase.h"
#include "UnitDataAsset.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "VerticalLayerComponent.h"
#include "UnitMoraleComponent.h"
#include "AbilityComponent.h"
#include "AbilityBase.h"
#include "AbilityBase_Generic.h"
#include "WOTOLProjectileBase.h"
#include "Gameplay/Demo/WOTOLProjectileTracer.h"
#include "Gameplay/Demo/WOTOLDamageNumber.h"
#include "Gameplay/Demo/WOTOLCoverStructure.h"
#include "Gameplay/Demo/WOTOLDemoUnit.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Gameplay/Factions/FactionSynergySubsystem.h"
#include "Gameplay/Demo/DemoFlowSubsystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

// EUnitRole (Units) et EDemoUnitCategory (Demo) ont les mêmes 6 valeurs mais pas le même
// ordre/index -> conversion explicite requise (même logique que AbilityBase.cpp, dupliquée
// volontairement : 6 cases, pas assez pour justifier un fichier partagé).
static EDemoUnitCategory UnitRoleToAbilityCategory(EUnitRole Role)
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

// Rythme de bataille (démo) : combats plus longs + déplacements ralentis (eau).
// 0.42 dégâts -> ~2,5× plus d'échanges ; 0.55 vitesse -> approche/repli plus lents.
// Rythme global des dégâts (frottement de l'eau). PLUS BAS = échanges plus lents
// = batailles PLUS LONGUES, sans changer l'issue ni les pertes (tout le monde frappe
// ET encaisse proportionnellement moins). 0.20 vise des batailles de ~4-5 min.
// 0.20 -> 0.18 : combats un peu plus longs => l'ennemi a le temps de riposter avant
// de mourir => le joueur encaisse de VRAIES pertes (fini le ~0 perte) sans que ce
// soit disproportionné (il gagne toujours). Rééquilibrage LÉGER post-amélioration IA.
float AUnitBase::GlobalDamageScale = 0.18f;
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

	// ── COMPÉTENCE ACTIVE (repli greybox) ──────────────────────────────────────
	// Le GDD (CLAUDE.md §7, Docs/DOCUMENT_MAITRE_WOTOL.md) documente une compétence
	// active nommée pour chaque unité (AbilityName/AbilityDescription/AbilityCooldown
	// dans UnitDataLibrary.cpp), mais AUCUNE classe UAbilityBase concrète n'était encore
	// assignée nulle part -> le système existait mais n'était JAMAIS instanciable.
	// Tant qu'aucune ability dédiée n'est assignée dans le DataAsset, on instancie le
	// repli générique (UAbilityBase_Generic) configuré avec les valeurs DÉJÀ présentes
	// dans le DataAsset (aucun nombre inventé ici, seul Damage est dérivé de AttackDPS
	// -> "une salve qui vaut ~2,5 s de DPS", clairement un placeholder de démo).
	if (AbilityComp && AbilityComp->AbilityClasses.Num() == 0 && !UnitData->AbilityName.IsEmpty())
	{
		AbilityComp->AbilityClasses.Add(UAbilityBase_Generic::StaticClass());
	}

	// IMPORTANT : Super::BeginPlay() (appelé avant InitFromDataAsset dans AUnitBase::BeginPlay)
	// a DÉJÀ déclenché AbilityComp->BeginPlay() -> AbilityClasses était encore vide à ce
	// moment-là. Sans ce ré-appel explicite, aucune ability n'était jamais réellement créée,
	// même quand AbilityClasses finissait par contenir des entrées valides.
	if (AbilityComp)
	{
		AbilityComp->RebuildAbilitiesFromClasses();
		if (UAbilityBase* Generic = AbilityComp->GetAbilityByIndex(0))
		{
			if (Generic->DisplayName.IsEmpty())
			{
				Generic->DisplayName  = UnitData->AbilityName;
				Generic->Description  = UnitData->AbilityDescription;
				Generic->Cooldown     = FMath::Max(1.f, UnitData->Stats.AbilityCooldown);
				Generic->Damage       = UnitData->Stats.AttackDPS * 2.5f;
				Generic->TargetType   = EAbilityTargetType::SingleUnit;
				// Aperçu holographique avant activation (Noxeflare uniquement, demande Liamor
				// 29/07/2026). Allongé (0.6s -> 1.4s, retour Liamor du 29/07/2026 : "un peu plus
				// long") pour laisser le temps de voir la zone. Reste un DÉLAI FIXE (pas encore un
				// vrai mode de visée qui suit la souris jusqu'au clic de confirmation, comme
				// demandé) — nécessite un état d'input dédié, laissé en TODO_WOTOL.md pour une
				// passe séparée (risque sur le clic RTS existant). 0 pour toutes les autres unités
				// -> comportement instantané inchangé.
				Generic->TelegraphDuration = UnitData->bAbilityHasTelegraph ? 1.4f : 0.f;
			}
		}
	}

	// MovementSpeed dans FUnitStats est un multiplicateur (1.0 = 600 UE units/s).
	// On applique le frottement de l'eau (GlobalSpeedScale) -> déplacements sous-marins
	// plus lents = batailles moins expédiées.
	GetCharacterMovement()->MaxWalkSpeed = 600.f * UnitData->Stats.MovementSpeed * GlobalSpeedScale;
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed; // mémorisé pour le ralenti (encre)
}

float AUnitBase::GetHealthPercent() const
{
	if (!UnitData || UnitData->Stats.MaxHealth <= 0) return 0.f;
	return CurrentHealth / static_cast<float>(UnitData->Stats.MaxHealth);
}

float AUnitBase::GetEffectiveAttackRange() const
{
	const float Base = UnitData ? static_cast<float>(UnitData->Stats.AttackRange) : 1.f;
	if (!UnitData || !UnitData->bAxisAffectsAttackRange) return Base;

	UGameInstance* GI = GetGameInstance();
	UDemoFlowSubsystem* Demo = GI ? GI->GetSubsystem<UDemoFlowSubsystem>() : nullptr;
	if (!Demo) return Base;

	const EDemoUnitCategory Cat = UnitRoleToAbilityCategory(UnitData->Role);
	if (Demo->GetUnitGrade(Cat) < 1) return Base;

	const int32 Axis = Demo->GetUnitAxis(Cat);
	// Axe 1 (longue portée, Hydrosniper) : +2 dès le choix de l'axe, +1 supplémentaire par
	// PALIER investi ensuite (jusqu'à MaxAxisTier) — demande Liamor 29/07/2026 : "plus ils
	// montent dans cet axe, plus la portée augmente", mais BORNÉE (jamais toute la carte).
	// Axe 2 (zone rapprochée, Hydropompe) : même logique en négatif. Grade 0 ou axe non choisi :
	// AUCUN changement (bonus = 0, cf. retour anticipé ci-dessus). PROVISOIRE, cf. TODO_WOTOL.md.
	const float Bonus = 2.f + static_cast<float>(Demo->GetAxisTier(Cat));
	if (Axis == 1) return FMath::Clamp(Base + Bonus, 1.f, 10.f);
	if (Axis == 2) return FMath::Clamp(Base - Bonus, 1.f, 10.f);
	return Base;
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

	// ── ÉQUILIBRAGE (difficulté + camp) : mise à l'échelle par le multiplicateur de
	// l'ATTAQUANT. Placé ICI (et non dans PerformAttack) pour couvrir AUSSI les dégâts de
	// COMPÉTENCES (rayons, souffles, rafales…) qui appellent TakeDamageFromUnit directement.
	if (InstigatorUnit)
	{
		Damage *= InstigatorUnit->BalanceDamageMult;
		Damage *= InstigatorUnit->AdaptiveOutgoingDamageMult;
	}

	// ── ESQUIVE / PARADE (font durer les combats, tous les coups ne portent pas) ──
	bool bBlocked = false;
	if (UnitData)
	{
		// Texte accroché à l'unité (VisualRoot) -> suit l'unité et sa hauteur de couche.
		USceneComponent* Anchor = GetDamageTextAnchor();
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
	EffDamage *= IncomingDamageMult;  // avantage de ZONE (défenseur qui possède le terrain)
	EffDamage *= AdaptiveIncomingDamageMult; // adaptation de rencontre (sans toucher aux stats)
	EffDamage *= AuraDefenseMult;     // aura de protection (Léviaphénix : boucliers renforcés)
	EffDamage *= FormationDefenseMult; // bonus/malus de formation tactique (rangs tenus)
	const float DamageBudget = FMath::Max(0.f,
		CurrentHealth - FMath::Max(0.f, MinimumHealthFloor));
	const float Applied      = FMath::Min(EffDamage, DamageBudget);
	CurrentHealth           -= Applied;
	if (InstigatorUnit) InstigatorUnit->DamageDealt += Applied; // pour le résumé de bataille

	const float MaxHP = UnitData ? static_cast<float>(UnitData->Stats.MaxHealth) : CurrentHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHP);

	// MORAL DÉSACTIVÉ (décision Liamor, validée) : pas de système de moral/déroute en bataille.
	// Le composant reste présent mais n'a AUCUN effet sur le combat -> les unités ne perdent
	// jamais le moral et ne fuient jamais.
	// (ancien : choc moral proportionnel sur perte de PV — retiré)

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return Applied;
}

void AUnitBase::PerformAttack(AUnitBase* Target)
{
	if (!Target || !Target->IsAlive() || !UnitData) return;

	// MORAL DÉSACTIVÉ (décision Liamor) : la déroute ne bloque PLUS l'attaque -> une unité
	// combat toujours, quel que soit son « moral ». (ancien : if (routing) return;)

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < UnitData->Stats.AttackCooldown) return;

	LastAttackTime = Now;
	OnAttackAnimTrigger(); // déclenche l'ANIM d'attaque UNIQUEMENT au moment d'un vrai coup

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
	BaseDamage *= OutgoingDamageMult; // avantage OFFENSIF de zone (défenseur galvanisé)
	// NB : l'équilibrage (BalanceDamageMult) est appliqué dans TakeDamageFromUnit (couvre
	// aussi les compétences), pas ici -> pas de double application.
	BaseDamage *= SynergyDamageMult;  // synergie dynamique (ex. bouclier soutenu par une lance)
	BaseDamage *= AuraDamageMult;     // aura d'amplification (Léviaphénix)
	BaseDamage *= NextHitCritMult;    // coup critique ponctuel (ex. backstab Aquilombres)
	NextHitCritMult = 1.f;            // consommé : ne vaut que pour CE coup

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

	// ÉQUILIBRAGE PHASE 1 : les DEUX factions manquaient de punch contre le Kraken (combat trop
	// long -> le Kraken use l'armée et gagne au chrono). On augmente les dégâts UNIQUEMENT
	// contre le boss (n'affecte donc PAS la phase 2/3) -> le Kraken tombe dans un délai correct.
	if (const AWOTOLDemoUnit* TDU = Cast<AWOTOLDemoUnit>(Target))
	{
		if (TDU->bCreatureBrain) BaseDamage *= 1.6f;
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
			// GARDE-FOU D'ENCOUNTER (MinimumHealthFloor, ex. le Kraken avant le quota de pertes) :
			// si la cible est DÉJÀ au plancher, le coup sera intégralement absorbé par
			// TakeDamageFromUnit (DamageBudget = 0) -> la vie ne bougera PAS. Afficher "CRITIQUE !"
			// dans ce cas donne l'impression d'un bug (le joueur voit "critique" mais aucune perte
			// de vie). On affiche donc un texte différent qui explique visuellement l'absorption.
			const bool bAbsorbedByFloor =
				(Target->CurrentHealth - FMath::Max(0.f, Target->MinimumHealthFloor)) <= 1.f;
			USceneComponent* CritAnchor = Target->GetDamageTextAnchor();
			const FVector Loc = (CritAnchor ? CritAnchor->GetComponentLocation() : Target->GetActorLocation()) + FVector(0.f, 0.f, 50.f);
			const FString CritText = bAbsorbedByFloor
				? TEXT("CARAPACE !")
				: (bBackstab ? TEXT("CRITIQUE DOS !") : TEXT("CRITIQUE !"));
			const FLinearColor CritColor = bAbsorbedByFloor
				? FLinearColor(0.55f, 0.6f, 0.65f, 1.f)
				: FLinearColor(1.f, 0.85f, 0.2f, 1.f);
			if (AWOTOLDamageNumber* N = AWOTOLDamageNumber::SpawnText(GetWorld(), Loc, CritText, CritColor))
				N->SetFollow(CritAnchor, FVector(0.f, 0.f, 140.f));
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
			// Noxéen (Noxeblast) = ovale allongé violet ; Aquiloris (Aquisphères) = petite sphère.
			const bool bBolt = (GetFaction() == EFactionID::Noxeens);
			// Distinction visuelle Hydrosniper/Hydropompe (Aquisphères, Axe 1/2 — demande Liamor
			// 29/07/2026 : "il faut qu'on voit la telle ou telle attaque"). Grade 0 ou toute autre
			// unité -> Size=1.0/pas de traînée, IDENTIQUE au comportement historique.
			float Size = 1.0f;
			bool bBubbleTrail = false;
			if (UnitData->bAxisAffectsAttackRange)
			{
				if (UGameInstance* GI = GetGameInstance())
				{
					if (UDemoFlowSubsystem* Demo = GI->GetSubsystem<UDemoFlowSubsystem>())
					{
						const EDemoUnitCategory Cat = UnitRoleToAbilityCategory(UnitData->Role);
						if (Demo->GetUnitGrade(Cat) >= 1)
						{
							const int32 Axis = Demo->GetUnitAxis(Cat);
							if (Axis == 1) Size = 0.65f;                          // Hydrosniper : tir fin, precis
							else if (Axis == 2) { Size = 1.9f; bBubbleTrail = true; } // Hydropompe : jet large
						}
					}
				}
			}
			AWOTOLProjectileTracer::Fire(GetWorld(), FromLoc, ToLoc, Col, Size, bBolt, bBubbleTrail);
		}
	}

	// ─── EFFETS SPÉCIAUX assignés sur la fiche de l'unité (Niagara / Fab) ───
	// Joués automatiquement : flash au niveau de l'unité + impact sur la cible. No-op si vides.
	{
		const USceneComponent* SelfA = GetFloatingTextAnchor();
		const FVector SelfLoc = (SelfA ? SelfA->GetComponentLocation() : GetActorLocation());
		PlayVFX(UnitData->MuzzleVFX, SelfLoc + GetActorForwardVector() * 40.f, GetActorRotation());
		const USceneComponent* TgtA = Target->GetDamageTextAnchor();
		const FVector TgtLoc = (TgtA ? TgtA->GetComponentLocation() : Target->GetActorLocation());
		PlayVFX(UnitData->AttackImpactVFX, TgtLoc);
	}

	OnAttackPerformed(Target);
}

void AUnitBase::PlayVFX(const TSoftObjectPtr<UNiagaraSystem>& VFX, const FVector& Loc, const FRotator& Rot)
{
	if (VFX.IsNull() || !GetWorld()) return;
	if (UNiagaraSystem* Sys = VFX.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Sys, Loc, Rot);
	}
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
