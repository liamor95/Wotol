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

// Une zone ne se conquiert pas toujours de la même manière. L'objectif est porté par la
// zone elle-même afin que la carte stratégique puisse alterner gardien, armée rivale,
// défense, faille/donjon, survie, escorte ou contrôle de position sans dupliquer le flux.
UENUM(BlueprintType)
enum class EZoneConquestObjective : uint8
{
	None             UMETA(DisplayName = "Aucun objectif"),
	GuardianCreature UMETA(DisplayName = "Créature gardienne"),
	RivalArmy        UMETA(DisplayName = "Armée rivale"),
	TerritoryDefense UMETA(DisplayName = "Défense de territoire"),
	RiftDungeon      UMETA(DisplayName = "Faille / donjon"),
	Survival         UMETA(DisplayName = "Survie"),
	Escort           UMETA(DisplayName = "Escorte"),
	HoldArea         UMETA(DisplayName = "Tenir une zone")
};

UENUM(BlueprintType)
enum class EZoneStrategicStatus : uint8
{
	Stable     UMETA(DisplayName = "Stable"),
	Threatened UMETA(DisplayName = "Menacée"),
	Contested  UMETA(DisplayName = "Contestée"),
	Lost       UMETA(DisplayName = "Perdue")
};

// État complet d'une zone capturable
USTRUCT(BlueprintType)
struct FZoneState
{
	GENERATED_BODY()

	// Grade actuel 0=neutre, 1-5 selon progression
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

	// Graphe territorial. La conquête est autorisée seulement si la cible touche au moins
	// une zone déjà possédée par la faction. Les liens sont rendus symétriques par ConnectZones.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FName> AdjacentZoneIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EZoneConquestObjective ConquestObjective = EZoneConquestObjective::GuardianCreature;

	UPROPERTY(BlueprintReadWrite)
	bool bConquestObjectiveCompleted = false;

	// État vivant de la zone : une faction IA peut annoncer son intention, laisser une
	// fenêtre de réaction puis rendre la zone neutre si le joueur ne revient pas à temps.
	UPROPERTY(BlueprintReadWrite)
	EZoneStrategicStatus StrategicStatus = EZoneStrategicStatus::Stable;

	UPROPERTY(BlueprintReadWrite)
	EFactionID ThreateningFaction = EFactionID::None;

	UPROPERTY(BlueprintReadWrite)
	float DefenseWindowRemainingSeconds = 0.f;

	// Fortification persistante : grade 1 = 1 emplacement, jusqu'à 5. Le niveau
	// technologique améliore les statistiques de chaque défense sans ajouter d'emplacement.
	UPROPERTY(BlueprintReadWrite)
	int32 InstalledDefenseCount = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 DefenseTechnologyLevel = 1;

	UPROPERTY(BlueprintReadWrite)
	int32 GarrisonUnits = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 GarrisonCapacity = 0;
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

	// ─── Graphe / objectifs / menaces ─────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Territory|Map")
	void ConnectZones(FName ZoneA, FName ZoneB);

	UFUNCTION(BlueprintPure, Category = "Territory|Map")
	bool AreZonesAdjacent(FName ZoneA, FName ZoneB) const;

	// Règle canon : la cible est accessible si elle est déjà possédée, ou si elle touche
	// au moins une zone possédée. Deux territoires isolés ne peuvent jamais être conquis.
	UFUNCTION(BlueprintPure, Category = "Territory|Map")
	bool CanFactionContestZone(FName TargetZoneID, EFactionID Faction) const;

	UFUNCTION(BlueprintCallable, Category = "Territory|Map")
	bool CompleteConquestObjective(FName ZoneID, EFactionID Faction);

	UFUNCTION(BlueprintCallable, Category = "Territory|Threat")
	void SetZoneThreat(FName ZoneID, EFactionID Attacker, float ReactionWindowSeconds);

	// Renvoie vrai si le délai vient d'expirer et que la zone a été neutralisée.
	UFUNCTION(BlueprintCallable, Category = "Territory|Threat")
	bool TickZoneThreat(FName ZoneID, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Territory|Threat")
	void NeutralizeZone(FName ZoneID);

	UFUNCTION(BlueprintCallable, Category = "Territory|Defense")
	void SetZoneFortification(FName ZoneID, int32 DefenseCount,
		int32 DefenseTechnologyLevel, int32 GarrisonUnits, int32 GarrisonCapacity);

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
