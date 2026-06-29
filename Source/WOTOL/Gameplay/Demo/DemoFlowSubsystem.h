#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "DemoFlowSubsystem.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// MACHINE D'ÉTATS DE LA DÉMO WOTOL — colonne vertébrale de la boucle jouable.
// Temps réel (PAS de tour par tour). Persiste dans le GameInstance entre phases.
// Pilote : cité → exploration → bataille créature → capture → mythique →
//          retour cité/déblocages → défense rivale → réparation → fin.
// ─────────────────────────────────────────────────────────────────────────────

// Phases majeures de la démo (regroupent les 41 étapes du cahier des charges)
UENUM(BlueprintType)
enum class EDemoPhase : uint8
{
	None                  UMETA(DisplayName = "Aucune"),
	CityIntro             UMETA(DisplayName = "Cité — intro + 1ère mission"),
	Exploration_Creature  UMETA(DisplayName = "Exploration — vers la créature"),
	Battle_Creature       UMETA(DisplayName = "Bataille — créature"),
	Capture_Zone          UMETA(DisplayName = "Capture de zone (Grade 1)"),
	Mythic_Discovery      UMETA(DisplayName = "Découverte du mythique juvénile"),
	City_Unlock           UMETA(DisplayName = "Cité — déblocages"),
	Exploration_Rival     UMETA(DisplayName = "Alerte — zone attaquée"),
	Battle_Rival          UMETA(DisplayName = "Bataille — défense rivale"),
	Repair_Zone           UMETA(DisplayName = "Réparation de la zone"),
	DemoEnd               UMETA(DisplayName = "Fin de démo")
};

// Type de bataille (la même arène sert pour les deux)
UENUM(BlueprintType)
enum class EBattleType : uint8
{
	CreatureEncounter UMETA(DisplayName = "Rencontre de créature"),
	RivalDefense      UMETA(DisplayName = "Défense contre la faction rivale")
};

// Catégorie d'unité (pilote forme greybox + règles de déblocage)
UENUM(BlueprintType)
enum class EDemoUnitCategory : uint8
{
	Chef       UMETA(DisplayName = "Chef / Héros"),
	Infanterie UMETA(DisplayName = "Infanterie"),
	Montee     UMETA(DisplayName = "Montée"),
	Distance   UMETA(DisplayName = "Distance"),
	Speciale   UMETA(DisplayName = "Spéciale"),
	Mythique   UMETA(DisplayName = "Mythique")
};

// Progression / déblocages de la démo
USTRUCT(BlueprintType)
struct FDemoProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bRangedUnlocked        = false; // Aquisphères / Noxeblast
	UPROPERTY(BlueprintReadOnly) bool bMythicDiscovered      = false; // juvénile découvert
	UPROPERTY(BlueprintReadOnly) bool bMythicBuildingUnlocked = false;
	UPROPERTY(BlueprintReadOnly) bool bRecruitBuildingActive = false;
	UPROPERTY(BlueprintReadOnly) bool bZoneCaptured          = false; // Grade 1 posé
	UPROPERTY(BlueprintReadOnly) bool bZoneDamaged           = false; // attaquée par la rivale
	UPROPERTY(BlueprintReadOnly) bool bZoneRepaired          = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDemoPhaseChanged,
	EDemoPhase, NewPhase, EDemoPhase, PreviousPhase);

UCLASS()
class WOTOL_API UDemoFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ─── Phase courante ────────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetPhase(EDemoPhase NewPhase);

	// Avance à la phase suivante dans l'ordre linéaire de la démo
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void AdvancePhase();

	UFUNCTION(BlueprintPure, Category = "Demo")
	EDemoPhase GetPhase() const { return CurrentPhase; }

	// Type de bataille correspondant à la phase courante (pour l'arène)
	UFUNCTION(BlueprintPure, Category = "Demo")
	EBattleType GetCurrentBattleType() const;

	// ─── Faction & roster ──────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Demo")
	EFactionID GetPlayerFaction() const;

	// ID canonique d'une unité de la faction joueur pour une catégorie donnée
	UFUNCTION(BlueprintPure, Category = "Demo")
	FName GetUnitID(EFactionID Faction, EDemoUnitCategory Category) const;

	// Vrai si l'unité est jouable/déployable à ce stade de la démo
	UFUNCTION(BlueprintPure, Category = "Demo")
	bool IsCategoryUnlocked(EDemoUnitCategory Category) const;

	// ─── Déblocages (appelés par le Director aux bons moments) ──────────────────

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void UnlockRangedUnit();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void DiscoverMythic();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void ActivateRecruitBuilding();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void MarkZoneCaptured();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void MarkZoneDamaged();

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void MarkZoneRepaired();

	UFUNCTION(BlueprintPure, Category = "Demo")
	const FDemoProgress& GetProgress() const { return Progress; }

	// ─── Statique : mapping catégorie → ID d'unité (noms du GDD/code) ───────────
	UFUNCTION(BlueprintPure, Category = "Demo")
	static EDemoUnitCategory GetCategoryForUnit(FName UnitID);

	UPROPERTY(BlueprintAssignable, Category = "Demo")
	FOnDemoPhaseChanged OnDemoPhaseChanged;

	// Message courant affiché par le HUD (objectif / narration)
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	FString CurrentMessage;

	// Issue de la démo (pour l'écran de fin : victoire ou défaite)
	UPROPERTY(BlueprintReadOnly, Category = "Demo")
	bool bDemoVictory = false;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetMessage(const FString& Msg) { CurrentMessage = Msg; }

	// Boss courant (créature) — pour la barre de vie du HUD
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void SetBoss(AActor* InBoss) { BossActor = InBoss; }

	UFUNCTION(BlueprintPure, Category = "Demo")
	AActor* GetBoss() const { return BossActor.Get(); }

private:
	TWeakObjectPtr<AActor> BossActor;

	EDemoPhase    CurrentPhase = EDemoPhase::None;
	FDemoProgress Progress;
};
