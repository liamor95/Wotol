#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "TerritoryStateManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCaptureProgressChanged,
	FName, ZoneID, EFactionID, Faction, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnZoneGradeChanged,
	FName, ZoneID, EFactionID, Faction, int32, NewGrade);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneLost,
	FName, ZoneID, EFactionID, PreviousOwner);

// État complet d'une zone capturable
USTRUCT(BlueprintType)
struct FZoneState
{
	GENERATED_BODY()

	// Grade actuel 0=neutre, 1-4 selon progression
	UPROPERTY(BlueprintReadOnly)
	int32 Grade = 0;

	// Faction qui contrôle la zone (None = neutre)
	UPROPERTY(BlueprintReadOnly)
	EFactionID Owner = EFactionID::None;

	// Faction en train de capturer (peut différer de Owner pendant contestation)
	UPROPERTY(BlueprintReadOnly)
	EFactionID CapturingFaction = EFactionID::None;

	// Progression dans le grade courant (0–100)
	UPROPERTY(BlueprintReadOnly)
	float CaptureProgress = 0.f;

	// Modificateur de vitesse de capture (courants marins, volcans…)
	UPROPERTY(BlueprintReadOnly)
	float TerrainCaptureModifier = 1.f;

	// Production de ressources par grade (index 0 = grade 1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<int32> ResourcePerGrade = { 5, 10, 20, 40 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EResourceType ProducedResource = EResourceType::BiomasseMarine;

	// Vitesse de capture de base (unités/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseCaptureRate = 10.f;
};

// Modificateur de terrain actif sur une zone
USTRUCT(BlueprintType)
struct FTerrainModifier
{
	GENERATED_BODY()

	// Identificateur du modificateur (ex: "CurrentMarin_Nord")
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ModifierID;

	// Zones affectées
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> AffectedZones;

	// Multiplicateur de vitesse déplacement (courant marin : 1.3 dans le sens, 0.7 à contre-courant)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeedMult = 1.f;

	// Modificateur capture (zone bioluminescente : avantage Noxéen)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CaptureRateMult = 1.f;

	// Faction qui bénéficie du bonus (None = tous)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFactionID BenefitsFaction = EFactionID::None;

	// Durée restante (-1 = permanent)
	UPROPERTY(BlueprintReadWrite)
	float RemainingDuration = -1.f;
};

// Gestionnaire de toutes les zones du niveau + modificateurs terrain
// Remplace l'ancien UTerritoryStateManager (UObject, capture binaire)
UCLASS()
class WOTOL_API UTerritoryStateManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ─── Enregistrement des zones ─────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Territory")
	void RegisterZone(FName ZoneID, const FZoneState& InitialState);

	UFUNCTION(BlueprintCallable, Category = "Territory")
	void UnregisterZone(FName ZoneID);

	// ─── Tick de capture ──────────────────────────────────────────────────────

	// Appelé par BP_Zone chaque seconde ; ControllingFaction = faction majoritaire dans la zone
	UFUNCTION(BlueprintCallable, Category = "Territory")
	void TickZoneCapture(FName ZoneID, EFactionID ControllingFaction, float DeltaSeconds);

	// ─── Lecture d'état ───────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "Territory")
	const FZoneState& GetZoneState(FName ZoneID) const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	int32 GetZoneGrade(FName ZoneID) const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	EFactionID GetZoneOwner(FName ZoneID) const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	float GetCaptureProgress(FName ZoneID) const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	int32 GetZoneResourceOutput(FName ZoneID) const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	TArray<FName> GetAllZonesOwnedBy(EFactionID Faction) const;

	// ─── Modificateurs de terrain ─────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Territory|Terrain")
	void AddTerrainModifier(const FTerrainModifier& Modifier);

	UFUNCTION(BlueprintCallable, Category = "Territory|Terrain")
	void RemoveTerrainModifier(FName ModifierID);

	// Obtenir le multiplicateur vitesse effectif (courants marins) pour une zone + faction
	UFUNCTION(BlueprintPure, Category = "Territory|Terrain")
	float GetMovementModifier(FName ZoneID, EFactionID Faction) const;

	// Tick des modificateurs avec durée limitée
	UFUNCTION(BlueprintCallable, Category = "Territory|Terrain")
	void TickTerrainModifiers(float DeltaSeconds);

	// ─── Délégués ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Territory")
	FOnCaptureProgressChanged OnCaptureProgressChanged;

	UPROPERTY(BlueprintAssignable, Category = "Territory")
	FOnZoneGradeChanged OnZoneGradeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Territory")
	FOnZoneLost OnZoneLost;

private:
	TMap<FName, FZoneState> Zones;
	TArray<FTerrainModifier> TerrainModifiers;

	// Zone vide retournée si ID invalide
	static const FZoneState EmptyZone;

	void AdvanceGrade(FName ZoneID, FZoneState& Zone, EFactionID Faction);
	void HandleFactionChange(FName ZoneID, FZoneState& Zone, EFactionID NewFaction);
	float ComputeEffectiveCaptureRate(FName ZoneID, const FZoneState& Zone,
		EFactionID Faction) const;
};
