#pragma once

#include "CoreMinimal.h"
#include "WOTOLTypes.generated.h"

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

// 3 paliers pour la démo ; extensible à 4 pour le jeu complet
UENUM(BlueprintType)
enum class EVerticalLayer : uint8
{
	Ground  UMETA(DisplayName = "Ground (0m)"),
	Mid     UMETA(DisplayName = "Mid (~5m)"),
	High    UMETA(DisplayName = "High (~15m)")
};

UENUM(BlueprintType)
enum class EBattlePhase : uint8
{
	Preparation  UMETA(DisplayName = "Preparation"),
	Tactical     UMETA(DisplayName = "Tactical"),
	Resolution   UMETA(DisplayName = "Resolution"),
	Ended        UMETA(DisplayName = "Ended")
};

UENUM(BlueprintType)
enum class EBattleResult : uint8
{
	None    UMETA(DisplayName = "None"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat  UMETA(DisplayName = "Defeat"),
	Draw    UMETA(DisplayName = "Draw")
};

USTRUCT(BlueprintType)
struct FTerritoryGrade
{
	GENERATED_BODY()

	// 1–4 ; démo = grade 1 uniquement
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
	FName HeroName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFactionID Faction = EFactionID::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> EquippedAbilities;
};
