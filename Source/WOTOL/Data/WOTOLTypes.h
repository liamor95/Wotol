#pragma once

#include "CoreMinimal.h"
#include "WOTOLTypes.generated.h"

// ─── Factions ────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EFactionID : uint8
{
	None             UMETA(DisplayName = "None"),
	Aquiloris        UMETA(DisplayName = "Aquiloris"),
	Noxeens          UMETA(DisplayName = "Noxéens"),
	Thalassidra      UMETA(DisplayName = "Thalassidra"),
	Mureniens        UMETA(DisplayName = "Muréniens"),
	PiratesAbyssaux  UMETA(DisplayName = "Pirates Abyssaux")
};

// ─── Ressources ───────────────────────────────────────────────────────────────
// 3 ressources universelles + 1 par faction (5 factions)

UENUM(BlueprintType)
enum class EResourceType : uint8
{
	// Universelles
	BiomasseMarine    UMETA(DisplayName = "Biomasse Marine"),
	MinerauxAbyssaux  UMETA(DisplayName = "Minéraux Abyssaux"),
	EnergieOceanique  UMETA(DisplayName = "Énergie Océanique"),  // a un max (56/66)

	// Spécifiques faction
	Cristaux          UMETA(DisplayName = "Cristaux (Aquiloris)"),
	CorailVivant      UMETA(DisplayName = "Corail Vivant (Thalassidra)"),
	MiasmesToxiques   UMETA(DisplayName = "Miasmes Toxiques (Muréniens)"),
	Biolumens         UMETA(DisplayName = "Biolumens (Noxéens)"),
	Epaves            UMETA(DisplayName = "Épaves (Pirates Abyssaux)")
};

// ─── Verticalité (4 couches océaniques) ──────────────────────────────────────

UENUM(BlueprintType)
enum class EVerticalLayer : uint8
{
	// Nommage court pour le code — correspond aux profondeurs océaniques
	Epipelagique    UMETA(DisplayName = "Épipélagique (0-200m)"),     // lumière max, vitesse +20%
	Mesopelagique   UMETA(DisplayName = "Mésopélagique (200-1000m)"), // crépuscule, embuscades
	Bathypelagique  UMETA(DisplayName = "Bathypélagique (1000-4000m)"),// obscurité, tanks dominent
	Hadal           UMETA(DisplayName = "Hadal (4000m+)")              // élite/Titans, x3 dégâts ascendants
};

// ─── Bataille ─────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
	Preparation  UMETA(DisplayName = "Préparation"),   // chargement
	Deployment   UMETA(DisplayName = "Déploiement"),   // placement unités sur grille hex
	Tactical     UMETA(DisplayName = "Tactique"),       // combat en cours
	Resolution   UMETA(DisplayName = "Résolution"),    // fin de bataille, résultats
	Ended        UMETA(DisplayName = "Terminé")
};

UENUM(BlueprintType)
enum class EBattleResult : uint8
{
	None    UMETA(DisplayName = "None"),
	Victory UMETA(DisplayName = "Victoire"),
	Defeat  UMETA(DisplayName = "Défaite"),
	Draw    UMETA(DisplayName = "Égalité")
};

// ─── Héros ────────────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EHeroHeritage : uint8
{
	Thalassi    UMETA(DisplayName = "Thalassi"),
	Givrelier   UMETA(DisplayName = "Givrelère"),
	Abysseen    UMETA(DisplayName = "Abysséen"),
	Gardien     UMETA(DisplayName = "Gardien")
};

UENUM(BlueprintType)
enum class EHeroSpecialty : uint8
{
	Thalassi    UMETA(DisplayName = "Thalassi"),
	Guerrier    UMETA(DisplayName = "Guerrier"),
	Mage        UMETA(DisplayName = "Mage"),
	Inquisiteur UMETA(DisplayName = "Inquisiteur")
};

// ─── Unités ───────────────────────────────────────────────────────────────────

// Type d'unité — correspond aux rôles définis dans le GDD
UENUM(BlueprintType)
enum class EUnitRole : uint8
{
	Chef       UMETA(DisplayName = "Chef / Commandant"),
	Mythique   UMETA(DisplayName = "Mythique"),
	Speciale   UMETA(DisplayName = "Spéciale"),
	Montee     UMETA(DisplayName = "Montée"),
	Distance   UMETA(DisplayName = "Distance"),
	Infanterie UMETA(DisplayName = "Infanterie")
};

// État interne d'une unité (state machine)
UENUM(BlueprintType)
enum class EUnitState : uint8
{
	Idle      UMETA(DisplayName = "Repos"),
	Moving    UMETA(DisplayName = "Déplacement"),
	Attacking UMETA(DisplayName = "Attaque"),
	Ability   UMETA(DisplayName = "Compétence"),
	Routing   UMETA(DisplayName = "Déroute"),  // moral à 0 — incontrôlable
	Dead      UMETA(DisplayName = "Mort")
};

// Type d'attaque — mêlée ou distance (critique pour Noxar et Noxeflare)
UENUM(BlueprintType)
enum class EUnitAttackType : uint8
{
	Melee  UMETA(DisplayName = "Mêlée"),
	Ranged UMETA(DisplayName = "Distance")
};

// Zone d'effet des compétences
UENUM(BlueprintType)
enum class EAbilityZoneType : uint8
{
	Mono        UMETA(DisplayName = "Mono-cible"),
	PetiteZone  UMETA(DisplayName = "Petite zone"),
	Zone        UMETA(DisplayName = "Zone"),
	GrandeZone  UMETA(DisplayName = "Grande zone"),
	Cone        UMETA(DisplayName = "Cône"),
	Aura        UMETA(DisplayName = "Aura"),
	ChargeLigne UMETA(DisplayName = "Charge en ligne"),
	Souffle     UMETA(DisplayName = "Souffle (canal continu)")
};

// ─── Menu & Session ───────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EDifficultyLevel : uint8
{
	Novice   UMETA(DisplayName = "Novice"),
	Guerrier UMETA(DisplayName = "Guerrier"),
	Stratege UMETA(DisplayName = "Stratège"),
	Legende  UMETA(DisplayName = "Légende")
};

UENUM(BlueprintType)
enum class EGameMode : uint8
{
	Campagne    UMETA(DisplayName = "Campagne"),
	Escarmouche UMETA(DisplayName = "Escarmouche"),
	Multijoueur UMETA(DisplayName = "Multijoueur")
};

// Étapes du flux de menu (correspond aux screens numérotés dans le design)
UENUM(BlueprintType)
enum class EMenuStep : uint8
{
	MainMenu,          // screen 3/5
	GameModeSelection, // screen 6
	FactionSelection,  // screen 7
	ChefPresentation,  // screen 8 (présentation du chef de faction)
	DifficultyChoice,  // screen 9
	CaptainCreation,   // screen 10
	Introduction,      // screen 11 (histoire + carte + objectifs)
	Kingdom,           // screen votre royaume
	Loading
};

// ─── Structs ──────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FTerritoryGrade
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "4"))
	int32 Grade = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CaptureProgressRequired = 100.f;
};

USTRUCT(BlueprintType)
struct FPlayerBehaviorProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	float AggressionScore = 0.5f;

	UPROPERTY(BlueprintReadWrite)
	float CautionScore = 0.5f;

	UPROPERTY(BlueprintReadWrite)
	float VerticalUsageRatio = 0.f;

	UPROPERTY(BlueprintReadWrite)
	int32 BattlesPlayed = 0;
};

USTRUCT(BlueprintType)
struct FHeroLoadout
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HeroName = TEXT("Aquilian");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFactionID Faction = EFactionID::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeroHeritage Heritage = EHeroHeritage::Thalassi;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EHeroSpecialty Specialty = EHeroSpecialty::Guerrier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "4"))
	int32 PortraitIndex = 0;

	// Aquiloris uniquement (choix demandé par Liamor le 25/07/2026) : le joueur incarne soit
	// Aquis (chef historique) soit Aquira (reine, stats/capacites strictement identiques —
	// Role Chef). Celui des deux qui N'EST PAS choisi devient le chef de faction en
	// narration/PNJ. Sans effet pour les Noxeens (Noxar reste le seul chef jouable).
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPlayAsAquira = false;
};

USTRUCT(BlueprintType)
struct FUnitRosterEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<class UUnitDataAsset> UnitData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUnitRole Role = EUnitRole::Infanterie;

	// Quota max dans l'escouade (ex: 3 pour Aquisphères, 1 pour Léviaphénix)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

// Stats numériques d'une unité — source de vérité unique (S_UnitData)
// JAMAIS hardcoder ces valeurs ailleurs
USTRUCT(BlueprintType)
struct FUnitStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MaxHealth = 1000;

	// Dégâts par seconde moyens (ATK/s dans le GDD)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackDPS = 100.f;

	// Réduction dégâts reçus en % (DEF%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
	float DefensePercent = 5.f;

	// Chance de PARADE en % : le coup est bloqué (dégâts fortement réduits).
	// Élevée pour les unités à bouclier / lourdes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "90"))
	float BlockChance = 0.f;

	// Chance d'ESQUIVE en % : le coup rate complètement (0 dégât).
	// Élevée pour les unités rapides / agiles.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "90"))
	float DodgeChance = 0.f;

	// Multiplicateur vitesse (1.0 = normal, 1.2 = +20%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeed = 1.f;

	// Portée d'attaque (1 = mêlée, 5 = longue distance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "5"))
	int32 AttackRange = 1;

	// Temps entre deux attaques en secondes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackCooldown = 1.0f;

	// Cooldown compétence principale en secondes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AbilityCooldown = 10.f;

	// Coût de recrutement en ressources primaires
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RecruitmentCost = 100;

	// Temps de recrutement en secondes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RecruitmentTime = 20.f;

	// Moral de départ (0-100), distinct des PV
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "100"))
	float StartingMorale = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUnitAttackType AttackType = EUnitAttackType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAbilityZoneType AbilityZoneType = EAbilityZoneType::Mono;

	// Couche verticale préférée (placement déploiement)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EVerticalLayer PreferredLayer = EVerticalLayer::Epipelagique;

	// Peut-elle changer de couche verticale en combat ?
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanChangeLayer = true;

	// Difficulté de maîtrise (1-5)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "5"))
	int32 MasteryDifficulty = 2;

	// Dépendance au groupe (1-5 : 1=autonome, 5=dépend du groupe)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "5"))
	int32 SynergyRating = 3;
};

// Coordonnée hexagonale axiale (système standard q/r)
USTRUCT(BlueprintType)
struct FHexCoord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Q = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 R = 0;

	FHexCoord() = default;
	FHexCoord(int32 InQ, int32 InR) : Q(InQ), R(InR) {}

	bool operator==(const FHexCoord& O) const { return Q == O.Q && R == O.R; }
};

FORCEINLINE uint32 GetTypeHash(const FHexCoord& H)
{
	return HashCombine(GetTypeHash(H.Q), GetTypeHash(H.R));
}

// Récompense d'objectif
USTRUCT(BlueprintType)
struct FObjectiveReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EResourceType ResourceType = EResourceType::BiomasseMarine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Amount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 XPReward = 0;
};

// Apparence du capitaine (indices 0-based, ex: 6 visages = indices 0-5)
USTRUCT(BlueprintType)
struct FHeroAppearance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 FaceIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 HairIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 ArmorIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 WeaponIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 BannerIndex = 0;

	// Index dans la palette de couleurs secondaires (6 couleurs)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "5"))
	int32 SecondaryColorIndex = 0;
};

// Config de session complète — persiste dans GameInstance
USTRUCT(BlueprintType)
struct FSessionConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	EFactionID SelectedFaction = EFactionID::None;

	UPROPERTY(BlueprintReadWrite)
	EGameMode GameMode = EGameMode::Campagne;

	UPROPERTY(BlueprintReadWrite)
	EDifficultyLevel Difficulty = EDifficultyLevel::Guerrier;

	UPROPERTY(BlueprintReadWrite)
	bool bIronmanMode = false;

	UPROPERTY(BlueprintReadWrite)
	bool bRandomMode = false;

	UPROPERTY(BlueprintReadWrite)
	FHeroLoadout HeroLoadout;

	UPROPERTY(BlueprintReadWrite)
	FHeroAppearance HeroAppearance;

	// Escouade choisie (max 5)
	UPROPERTY(BlueprintReadWrite)
	TArray<TSoftObjectPtr<class UUnitDataAsset>> SelectedSquad;
};

// ─── Couleurs officielles des factions — SOURCE DE VÉRITÉ UNIQUE ─────────────
// Aquiloris   = Bleu
// Noxéens     = Vert (PAS violet)
// Thalassidra = Jaune-orange
// Muréniens   = Violet (PAS vert)
// Pirates Abyssaux = Rouge
// Ne jamais définir ces couleurs ailleurs dans le code ou les assets.

USTRUCT(BlueprintType)
struct FFactionColors
{
	GENERATED_BODY()

	static FLinearColor Get(EFactionID Faction)
	{
		switch (Faction)
		{
			case EFactionID::Aquiloris:       return FLinearColor(0.05f, 0.35f, 0.90f, 1.f); // Bleu
			case EFactionID::Noxeens:         return FLinearColor(0.05f, 0.75f, 0.20f, 1.f); // Vert
			case EFactionID::Thalassidra:    return FLinearColor(0.95f, 0.65f, 0.05f, 1.f); // Jaune-orange
			case EFactionID::Mureniens:       return FLinearColor(0.50f, 0.10f, 0.80f, 1.f); // Violet
			case EFactionID::PiratesAbyssaux: return FLinearColor(0.85f, 0.10f, 0.10f, 1.f); // Rouge
			default:                          return FLinearColor::White;
		}
	}

	// Teinte SECONDAIRE (plus claire/douce, même famille de teinte que Get()) — pour le texte
	// de lore, les halos, les titres, tout élément d'UI qui a besoin d'un ton moins saturé que
	// l'accent principal. Fait partie de la même source de vérité unique que Get() : à utiliser
	// PARTOUT où le HUD a besoin d'une couleur de faction, plutôt que de redéfinir une teinte
	// approximative localement (thème d'interface par faction, décision Liamor du 22/07/2026).
	static FLinearColor GetSecondary(EFactionID Faction)
	{
		switch (Faction)
		{
			case EFactionID::Aquiloris:       return FLinearColor(0.55f, 0.85f, 1.00f, 1.f);
			case EFactionID::Noxeens:         return FLinearColor(0.50f, 1.00f, 0.62f, 1.f);
			case EFactionID::Thalassidra:    return FLinearColor(1.00f, 0.85f, 0.45f, 1.f);
			case EFactionID::Mureniens:       return FLinearColor(0.75f, 0.45f, 0.95f, 1.f);
			case EFactionID::PiratesAbyssaux: return FLinearColor(1.00f, 0.45f, 0.40f, 1.f);
			default:                          return FLinearColor::White;
		}
	}

	// Vrai si la faction est jouable dans la démo (Couche 1 uniquement)
	static bool IsPlayableInDemo(EFactionID Faction)
	{
		return Faction == EFactionID::Aquiloris || Faction == EFactionID::Noxeens;
	}

	// Vrai si le nom de la faction doit être caché dans l'écran de sélection
	static bool IsNameHiddenInDemo(EFactionID Faction)
	{
		return !IsPlayableInDemo(Faction) && Faction != EFactionID::None;
	}
};
