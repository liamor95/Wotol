#include "UnitDataLibrary.h"

// ─────────────────────────────────────────────────────────────────────────────
// Toutes les valeurs viennent du GDD WOTOL (CLAUDE.md §7)
// ─────────────────────────────────────────────────────────────────────────────

static void FillAquis(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Aquis"));
	A->Description   = FText::FromString(TEXT("Chef des Aquiloris — Lame Photonique (Cône)"));
	A->Faction       = EFactionID::Aquiloris;
	A->Role          = EUnitRole::Chef;
	A->MaxCountInSquad = 1;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1400;
	S.AttackDPS        = 120.f;
	S.DefensePercent   = 15.f;
	S.MovementSpeed    = 1.0f;
	S.AttackRange      = 1;
	S.AttackCooldown   = 1.1f;
	S.AbilityCooldown  = 12.f;
	S.RecruitmentCost  = 220;
	S.StartingMorale   = 90.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::Cone;
	S.PreferredLayer   = EVerticalLayer::Epipelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 3;
	S.SynergyRating    = 5;

	A->AbilityName        = FText::FromString(TEXT("Lame Photonique"));
	A->AbilityDescription = FText::FromString(TEXT("Onde de choc directionnelle en cône"));
	A->AxisOneName        = FText::FromString(TEXT("Onde DPS"));
	A->AxisOneDescription = FText::FromString(TEXT("Onde dégâts importants sur plusieurs unités"));
	A->AxisTwoName        = FText::FromString(TEXT("Onde de Repoussement"));
	A->AxisTwoDescription = FText::FromString(TEXT("Onde circulaire projette ennemis proches"));
	A->PassiveDescription = FText::FromString(TEXT("Bonus coordination + réduction recharge alliés proches. Synergie : Aquilombres"));
	A->RequiredBuilding   = FText::FromString(TEXT("Aquilore"));
}

static void FillLeviaphenix(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Léviaphénix"));
	A->Description   = FText::FromString(TEXT("Mythique Aquiloris — Résonance Technologique (Aura)"));
	A->Faction       = EFactionID::Aquiloris;
	A->Role          = EUnitRole::Mythique;
	A->MaxCountInSquad = 1;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 2000;
	S.AttackDPS        = 130.f;
	S.DefensePercent   = 15.f;
	S.MovementSpeed    = 1.2f;
	S.AttackRange      = 3;
	S.AttackCooldown   = 1.6f;
	S.AbilityCooldown  = 20.f;
	S.RecruitmentCost  = 400;
	S.StartingMorale   = 85.f;
	S.AttackType       = EUnitAttackType::Ranged;
	S.AbilityZoneType  = EAbilityZoneType::Aura;
	S.PreferredLayer   = EVerticalLayer::Epipelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 4;
	S.SynergyRating    = 5;

	A->AbilityName        = FText::FromString(TEXT("Résonance Technologique"));
	A->AbilityDescription = FText::FromString(TEXT("Amplifie les stats de toutes les unités alliées proches"));
	A->AxisOneName        = FText::FromString(TEXT("Rayonnement Stabilisateur"));
	A->AxisOneDescription = FText::FromString(TEXT("Amplification poussée, zone élargie"));
	A->AxisTwoName        = FText::FromString(TEXT("Rayonnement Vital"));
	A->AxisTwoDescription = FText::FromString(TEXT("Fait revenir quelques unités tombées"));
	A->PassiveDescription = FText::FromString(TEXT("Amplifie dégâts, réduit recharges, augmente défense boucliers. Synergie : Aquisphères"));
	A->RequiredBuilding   = FText::FromString(TEXT("Cœur-Éclat du Léviaphénix"));
}

static void FillAquiloryons(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Aquiloryons"));
	A->Description   = FText::FromString(TEXT("Infanterie Aquiloris — Mur de Cristal (Mono)"));
	A->Faction       = EFactionID::Aquiloris;
	A->Role          = EUnitRole::Infanterie;
	A->MaxCountInSquad = 3;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1700;
	S.AttackDPS        = 80.f;
	S.DefensePercent   = 25.f;
	S.MovementSpeed    = 0.8f;
	S.AttackRange      = 1;
	S.AttackCooldown   = 1.3f;
	S.AbilityCooldown  = 10.f;
	S.RecruitmentCost  = 130;
	S.StartingMorale   = 80.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::Mono;
	S.PreferredLayer   = EVerticalLayer::Epipelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 2;
	S.SynergyRating    = 4;

	A->AbilityName        = FText::FromString(TEXT("Mur de Cristal"));
	A->AbilityDescription = FText::FromString(TEXT("Formation rempart, absorption dégâts frontaux"));
	A->AxisOneName        = FText::FromString(TEXT("Mur Amplifié"));
	A->AxisOneDescription = FText::FromString(TEXT("Protection collective accrue"));
	A->AxisTwoName        = FText::FromString(TEXT("Double Lames"));
	A->AxisTwoDescription = FText::FromString(TEXT("Sacrifie défense pour dégâts"));
	A->PassiveDescription = FText::FromString(TEXT("Bonus coordination, renforce unités adjacentes. Synergie : Aquilances"));
	A->RequiredBuilding   = FText::FromString(TEXT("Académie Aquiloryon"));
}

static void FillAquilances(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Aquilances"));
	A->Description   = FText::FromString(TEXT("Montée Aquiloris — Percée Ondulatoire (Charge ligne)"));
	A->Faction       = EFactionID::Aquiloris;
	A->Role          = EUnitRole::Montee;
	A->MaxCountInSquad = 3;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1200;
	S.AttackDPS        = 140.f;
	S.DefensePercent   = 10.f;
	S.MovementSpeed    = 1.3f;
	S.AttackRange      = 3;
	S.AttackCooldown   = 0.9f;
	S.AbilityCooldown  = 14.f;
	S.RecruitmentCost  = 180;
	S.StartingMorale   = 75.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::ChargeLigne;
	S.PreferredLayer   = EVerticalLayer::Epipelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 3;
	S.SynergyRating    = 3;

	A->AbilityName        = FText::FromString(TEXT("Percée Ondulatoire"));
	A->AbilityDescription = FText::FromString(TEXT("Charge frontale concentrant l'énergie de la lance"));
	A->AxisOneName        = FText::FromString(TEXT("Percée Amplifiée"));
	A->AxisOneDescription = FText::FromString(TEXT("Dégâts augmentés, renverse unités légères"));
	A->AxisTwoName        = FText::FromString(TEXT("Rempart Synthétique"));
	A->AxisTwoDescription = FText::FromString(TEXT("Formation hauteur, empêche attaques descendantes"));
	A->PassiveDescription = FText::FromString(TEXT("Résistance frontale, saignement au contact. Synergie : Aquiloryons"));
	A->RequiredBuilding   = FText::FromString(TEXT("Dôme des Aquilances"));
}

static void FillAquipheres(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Aquisphères"));
	A->Description   = FText::FromString(TEXT("Distance Aquiloris — Hydrolaser (Petite zone)"));
	A->Faction       = EFactionID::Aquiloris;
	A->Role          = EUnitRole::Distance;
	A->MaxCountInSquad = 3;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1000;
	S.AttackDPS        = 130.f;
	S.DefensePercent   = 5.f;
	S.MovementSpeed    = 0.9f;
	S.AttackRange      = 5;
	S.AttackCooldown   = 1.2f;
	S.AbilityCooldown  = 8.f;
	S.RecruitmentCost  = 140;
	S.StartingMorale   = 70.f;
	S.AttackType       = EUnitAttackType::Ranged;
	S.AbilityZoneType  = EAbilityZoneType::PetiteZone;
	S.PreferredLayer   = EVerticalLayer::Epipelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 2;
	S.SynergyRating    = 4;

	A->AbilityName        = FText::FromString(TEXT("Hydrolaser"));
	A->AbilityDescription = FText::FromString(TEXT("Tir sphère laser précise"));
	A->AxisOneName        = FText::FromString(TEXT("Hydrosniper"));
	A->AxisOneDescription = FText::FromString(TEXT("Longue portée mono-cible, dégâts élevés"));
	A->AxisTwoName        = FText::FromString(TEXT("Hydropompe"));
	A->AxisTwoDescription = FText::FromString(TEXT("Tir zone, dégâts réduits mais AoE"));
	A->PassiveDescription = FText::FromString(TEXT("Bonne précision naturelle. Synergie : Léviaphénix"));
	A->RequiredBuilding   = FText::FromString(TEXT("Champ de Tir des Aquisphères"));
}

static void FillAquilombres(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Aquilombres"));
	A->Description   = FText::FromString(TEXT("Spéciale Aquiloris — Ombres Glissées (Mono)"));
	A->Faction       = EFactionID::Aquiloris;
	A->Role          = EUnitRole::Speciale;
	A->MaxCountInSquad = 2;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 900;
	S.AttackDPS        = 170.f;
	S.DefensePercent   = 5.f;
	S.MovementSpeed    = 1.1f;
	S.AttackRange      = 1;
	S.AttackCooldown   = 0.8f;
	S.AbilityCooldown  = 12.f;
	S.RecruitmentCost  = 160;
	S.StartingMorale   = 75.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::Mono;
	S.PreferredLayer   = EVerticalLayer::Mesopelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 4;
	S.SynergyRating    = 3;

	A->AbilityName        = FText::FromString(TEXT("Ombres Glissées"));
	A->AbilityDescription = FText::FromString(TEXT("Mode furtif — coup critique dans le dos de la cible"));
	A->AxisOneName        = FText::FromString(TEXT("Critique Amplifié"));
	A->AxisOneDescription = FText::FromString(TEXT("Dégâts critiques augmentés + retour furtif auto"));
	A->AxisTwoName        = FText::FromString(TEXT("Ombres Projetées"));
	A->AxisTwoDescription = FText::FromString(TEXT("Ombre massive devant lignes ennemies, réduit visibilité/précision"));
	A->PassiveDescription = FText::FromString(TEXT("Invisibles si immobiles. Synergie : Aquis (Chef)"));
	A->RequiredBuilding   = FText::FromString(TEXT("Nexus des Ombres"));
}

// ─── NOXÉENS ─────────────────────────────────────────────────────────────────

static void FillNoxar(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Noxar"));
	A->Description   = FText::FromString(TEXT("Chef des Noxéens — Fracture Abyssale (tirs laser discontinus)"));
	A->Faction       = EFactionID::Noxeens;
	A->Role          = EUnitRole::Chef;
	A->MaxCountInSquad = 1;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1300;
	S.AttackDPS        = 150.f;
	S.DefensePercent   = 10.f;
	S.MovementSpeed    = 1.0f;
	S.AttackRange      = 1;
	S.AttackCooldown   = 1.0f;
	S.AbilityCooldown  = 12.f;
	S.RecruitmentCost  = 230;
	S.StartingMorale   = 90.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::Cone;
	S.PreferredLayer   = EVerticalLayer::Mesopelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 4;
	S.SynergyRating    = 4;

	A->AbilityName        = FText::FromString(TEXT("Fracture Abyssale"));
	A->AbilityDescription = FText::FromString(TEXT("Tirs laser courts discontinus — harcèlement + ignore partiellement l'armure"));
	A->AxisOneName        = FText::FromString(TEXT("Domination Laser"));
	A->AxisOneDescription = FText::FromString(TEXT("Rayons traversants, dégâts exponentiels sur cible isolée, explosion finale"));
	A->AxisTwoName        = FText::FromString(TEXT("Surcharge Bioluminescente"));
	A->AxisTwoDescription = FText::FromString(TEXT("Halo amplif vitesse attaque + dégâts énergétiques + résistance peur/contrôle alliés"));
	A->PassiveDescription = FText::FromString(TEXT("Chaque élimination proche = charge de Surcharge. Catalyse recharge Noxedrake. Synergie : Noxedrake"));
	A->RequiredBuilding   = FText::FromString(TEXT("Quartier Général Noxéen"));
}

static void FillNoxedrake(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Noxedrake"));
	A->Description   = FText::FromString(TEXT("Mythique Noxéens — Souffle d'Extinction (rayon continu)"));
	A->Faction       = EFactionID::Noxeens;
	A->Role          = EUnitRole::Mythique;
	A->MaxCountInSquad = 1;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 2000;
	S.AttackDPS        = 180.f;
	S.DefensePercent   = 12.f;
	S.MovementSpeed    = 1.0f;
	S.AttackRange      = 2;
	S.AttackCooldown   = 1.2f;
	S.AbilityCooldown  = 18.f;
	S.RecruitmentCost  = 420;
	S.StartingMorale   = 80.f;
	S.AttackType       = EUnitAttackType::Ranged;
	S.AbilityZoneType  = EAbilityZoneType::Souffle;
	S.PreferredLayer   = EVerticalLayer::Bathypelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 5;
	S.SynergyRating    = 3;

	A->AbilityName        = FText::FromString(TEXT("Souffle d'Extinction"));
	A->AbilityDescription = FText::FromString(TEXT("Laser continu géant depuis la gueule — traverse les unités — dégâts massifs/s — Combustion Luminale (DoT)"));
	A->AxisOneName        = FText::FromString(TEXT("Dévastation Totale"));
	A->AxisOneDescription = FText::FromString(TEXT("Souffle plus large, explosion terminale, recharge réduite sur élimination"));
	A->AxisTwoName        = FText::FromString(TEXT("Dominion Radieux"));
	A->AxisTwoDescription = FText::FromString(TEXT("Marquage cumulatif, légère auto-régénération sur dégâts infligés"));
	A->PassiveDescription = FText::FromString(TEXT("En infligeant dégâts continus : vitesse augmente, résistance contrôle s'améliore. Synergie : Noxar. ⚠️ Vulnérable pendant canalisation"));
	A->RequiredBuilding   = FText::FromString(TEXT("Repaire du Noxedrake"));
}

static void FillNoxeflare(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Noxeflare"));
	A->Description   = FText::FromString(TEXT("Infanterie Noxéens — Éblouissement Abyssal (Mono)"));
	A->Faction       = EFactionID::Noxeens;
	A->Role          = EUnitRole::Infanterie;
	A->MaxCountInSquad = 3;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1000;
	S.AttackDPS        = 170.f;
	S.DefensePercent   = 8.f;
	S.MovementSpeed    = 1.1f;
	S.AttackRange      = 1;
	S.AttackCooldown   = 0.8f;
	S.AbilityCooldown  = 10.f;
	S.RecruitmentCost  = 125;
	S.StartingMorale   = 75.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::Mono;
	S.PreferredLayer   = EVerticalLayer::Mesopelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 2;
	S.SynergyRating    = 2;

	A->AbilityName        = FText::FromString(TEXT("Éblouissement Abyssal"));
	A->AbilityDescription = FText::FromString(TEXT("Flash violet bioluminescent frontal — réduction précision ennemie — désorientation courte"));
	A->AxisOneName        = FText::FromString(TEXT("Voie du Voile Profond"));
	A->AxisOneDescription = FText::FromString(TEXT("Zone élargie, durée augmentée, ralentissement, chance d'interruption"));
	A->AxisTwoName        = FText::FromString(TEXT("Voie de la Frappe Aveugle"));
	A->AxisTwoDescription = FText::FromString(TEXT("Bonus dégâts massifs sur aveuglés, recharge réduite sur élimination"));
	A->PassiveDescription = FText::FromString(TEXT("Ennemis proches subissent légère baisse précision passive permanente. Synergie clé : Noxeblast"));
	A->RequiredBuilding   = FText::FromString(TEXT("Caserne Noxeflare"));
}

static void FillNoxeblast(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Noxeblast"));
	A->Description   = FText::FromString(TEXT("Distance Noxéens — Décharge Abyssale (Mono)"));
	A->Faction       = EFactionID::Noxeens;
	A->Role          = EUnitRole::Distance;
	A->MaxCountInSquad = 3;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 900;
	S.AttackDPS        = 140.f;
	S.DefensePercent   = 5.f;
	S.MovementSpeed    = 1.0f;
	S.AttackRange      = 5;
	S.AttackCooldown   = 1.1f;
	S.AbilityCooldown  = 8.f;
	S.RecruitmentCost  = 140;
	S.StartingMorale   = 70.f;
	S.AttackType       = EUnitAttackType::Ranged;
	S.AbilityZoneType  = EAbilityZoneType::Mono;
	S.PreferredLayer   = EVerticalLayer::Epipelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 3;
	S.SynergyRating    = 3;

	A->AbilityName        = FText::FromString(TEXT("Décharge Abyssale"));
	A->AbilityDescription = FText::FromString(TEXT("Tir énergie concentrée, mono-cible, longue portée, dégâts purs. Projectile depuis les paumes"));
	A->AxisOneName        = FText::FromString(TEXT("Rayon Perforant"));
	A->AxisOneDescription = FText::FromString(TEXT("Tirs traversants qui percent plusieurs unités alignées"));
	A->AxisTwoName        = FText::FromString(TEXT("Explosion Bioluminescente"));
	A->AxisTwoDescription = FText::FromString(TEXT("Explose à l'impact — aveugle unités proches — désorganise formations"));
	A->PassiveDescription = FText::FromString(TEXT("Bonus dégâts significatif sur cible affectée par désorientation/aveuglement. Synergie clé : Noxeflare"));
	A->RequiredBuilding   = FText::FromString(TEXT("Tour de Décharge Noxéenne"));
}

static void FillNoxeons(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Noxéons"));
	A->Description   = FText::FromString(TEXT("Spéciale Noxéens — Émergence Luminale (Zone) — organismes bioluminescents verts"));
	A->Faction       = EFactionID::Noxeens;
	A->Role          = EUnitRole::Speciale;
	A->MaxCountInSquad = 2;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1100;
	S.AttackDPS        = 90.f;
	S.DefensePercent   = 8.f;
	S.MovementSpeed    = 0.9f;
	S.AttackRange      = 3;
	S.AttackCooldown   = 1.3f;
	S.AbilityCooldown  = 10.f;
	S.RecruitmentCost  = 160;
	S.StartingMorale   = 70.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::Zone;
	S.PreferredLayer   = EVerticalLayer::Mesopelagique;
	S.bCanChangeLayer  = false;
	S.MasteryDifficulty = 4;
	S.SynergyRating    = 5;

	A->AbilityName        = FText::FromString(TEXT("Émergence Luminale"));
	A->AbilityDescription = FText::FromString(TEXT("Zone bioluminescente au sol — bonus dégâts + vitesse aux Noxéens dans la zone"));
	A->AxisOneName        = FText::FromString(TEXT("Réacteur de Guerre"));
	A->AxisOneDescription = FText::FromString(TEXT("Bonus zone fortement augmentés, cooldowns réduits, charge Noxedrake accélérée"));
	A->AxisTwoName        = FText::FromString(TEXT("Ancrage Abyssal"));
	A->AxisTwoDescription = FText::FromString(TEXT("Zone plus large, résistance accrue alliés, régénération continue, réduction contrôles"));
	A->PassiveDescription = FText::FromString(TEXT("Chaque Noxéon actif augmente légèrement la production énergétique globale. Cumulatif. Synergie : Noxar et Noxedrake. ⚠️ Faible mobilité"));
	A->RequiredBuilding   = FText::FromString(TEXT("Bassin Luminal"));
}

static void FillNoxebeast(UUnitDataAsset* A)
{
	A->DisplayName   = FText::FromString(TEXT("Noxebeast"));
	A->Description   = FText::FromString(TEXT("Montée Noxéens — Fracasse-Fosse (Petite zone)"));
	A->Faction       = EFactionID::Noxeens;
	A->Role          = EUnitRole::Montee;
	A->MaxCountInSquad = 3;

	FUnitStats& S      = A->Stats;
	S.MaxHealth        = 1700;
	S.AttackDPS        = 120.f;
	S.DefensePercent   = 20.f;
	S.MovementSpeed    = 1.2f;
	S.AttackRange      = 1;
	S.AttackCooldown   = 1.1f;
	S.AbilityCooldown  = 14.f;
	S.RecruitmentCost  = 190;
	S.StartingMorale   = 80.f;
	S.AttackType       = EUnitAttackType::Melee;
	S.AbilityZoneType  = EAbilityZoneType::PetiteZone;
	S.PreferredLayer   = EVerticalLayer::Bathypelagique;
	S.bCanChangeLayer  = true;
	S.MasteryDifficulty = 3;
	S.SynergyRating    = 3;

	A->AbilityName        = FText::FromString(TEXT("Fracasse-Fosse"));
	A->AbilityDescription = FText::FromString(TEXT("Charge destructrice frontale — repousse/renverse unités légères — interrompt compétences ennemies"));
	A->AxisOneName        = FText::FromString(TEXT("Bastion Brutal"));
	A->AxisOneDescription = FText::FromString(TEXT("Réduction massive dégâts après charge — provocation courte — zone instable au sol (ralentit)"));
	A->AxisTwoName        = FText::FromString(TEXT("Défoncement"));
	A->AxisTwoDescription = FText::FromString(TEXT("Charge plus rapide, dégâts augmentés, perfore formations, renverse unités lourdes"));
	A->PassiveDescription = FText::FromString(TEXT("Plus il subit dégâts consécutifs, plus résistance augmente. Immunité brève contrôle à haut seuil. Synergie : Noxedrake. ⚠️ Vulnérable 2-3s APRÈS la charge"));
	A->RequiredBuilding   = FText::FromString(TEXT("Fosse aux Noxebeasts"));
}

// ─── Dispatch ─────────────────────────────────────────────────────────────────

// Parade (bouclier/lourd) & Esquive (rapide/agile) — dérivées du profil de chaque
// unité (la matrice n'a pas de colonne dédiée). Font durer les combats et évitent
// que tous les coups portent. Valeurs en % (tunables).
static void ApplyBlockDodge(const FName& UnitID, FUnitStats& S)
{
	struct FBD { const TCHAR* Id; float Block; float Dodge; };
	static const FBD Table[] = {
		// Aquiloris
		{ TEXT("Aquis"),        20.f, 10.f },
		{ TEXT("Aquiloryons"),  40.f,  5.f }, // Mur de Cristal = grosse parade
		{ TEXT("Aquilances"),   10.f, 20.f },
		{ TEXT("Aquipheres"),    5.f, 10.f },
		{ TEXT("Aquilombres"),   5.f, 35.f }, // assassin agile = grosse esquive
		{ TEXT("Leviaphenix"),  15.f, 10.f },
		// Noxéens
		{ TEXT("Noxar"),        15.f, 15.f },
		{ TEXT("Noxeflare"),     8.f, 20.f },
		{ TEXT("Noxeblast"),     5.f, 12.f },
		{ TEXT("Noxeons"),       8.f,  5.f },
		{ TEXT("Noxebeast"),    30.f,  8.f }, // carapace pressurisée = grosse parade
		{ TEXT("Noxedrake"),    10.f,  5.f }, // boss : peu d'esquive (restant battable)
	};
	for (const FBD& E : Table)
	{
		if (UnitID == E.Id) { S.BlockChance = E.Block; S.DodgeChance = E.Dodge; return; }
	}
}

UUnitDataAsset* UUnitDataLibrary::CreateUnitDataAsset(const FName& UnitID, UObject* Outer)
{
	UUnitDataAsset* Asset = NewObject<UUnitDataAsset>(Outer, UnitID);

	if      (UnitID == "Aquis")         FillAquis(Asset);
	else if (UnitID == "Leviaphenix")   FillLeviaphenix(Asset);
	else if (UnitID == "Aquiloryons")   FillAquiloryons(Asset);
	else if (UnitID == "Aquilances")    FillAquilances(Asset);
	else if (UnitID == "Aquipheres")    FillAquipheres(Asset);
	else if (UnitID == "Aquilombres")   FillAquilombres(Asset);
	else if (UnitID == "Noxar")         FillNoxar(Asset);
	else if (UnitID == "Noxedrake")     FillNoxedrake(Asset);
	else if (UnitID == "Noxeflare")     FillNoxeflare(Asset);
	else if (UnitID == "Noxeblast")     FillNoxeblast(Asset);
	else if (UnitID == "Noxeons")       FillNoxeons(Asset);
	else if (UnitID == "Noxebeast")     FillNoxebeast(Asset);

	ApplyBlockDodge(UnitID, Asset->Stats);
	return Asset;
}

TArray<FName> UUnitDataLibrary::GetAllDemoUnitIDs()
{
	return {
		TEXT("Aquis"), TEXT("Leviaphenix"), TEXT("Aquiloryons"),
		TEXT("Aquilances"), TEXT("Aquipheres"), TEXT("Aquilombres"),
		TEXT("Noxar"), TEXT("Noxedrake"), TEXT("Noxeflare"),
		TEXT("Noxeblast"), TEXT("Noxeons"), TEXT("Noxebeast")
	};
}
