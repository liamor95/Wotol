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
	Thalassidras     UMETA(DisplayName = "Thalassidras"),
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

// ─── Verticalité (3 paliers démo, extensible à 4) ────────────────────────────

UENUM(BlueprintType)
enum class EVerticalLayer : uint8
{
	Ground  UMETA(DisplayName = "Sol"),
	Mid     UMETA(DisplayName = "Mid (~5m)"),
	High    UMETA(DisplayName = "Surface (~15m)")
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

UENUM(BlueprintType)
enum class EUnitRole : uint8
{
	Distance   UMETA(DisplayName = "Distance"),
	Infanterie UMETA(DisplayName = "Infanterie"),
	Magie      UMETA(DisplayName = "Magie"),
	Montee     UMETA(DisplayName = "Montée"),
	Mythique   UMETA(DisplayName = "Mythique")
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
};

USTRUCT(BlueprintType)
struct FUnitRosterEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<class UUnitDataAsset> UnitData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUnitRole Role = EUnitRole::Infanterie;

	// Quota max dans l'escouade (ex: 3 pour Aquistance, 1 pour Léviaphénix)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 MaxCount = 1;
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
