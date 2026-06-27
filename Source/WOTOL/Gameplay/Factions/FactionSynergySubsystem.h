#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "FactionSynergySubsystem.generated.h"

class AUnitBase;

// Résultat calculé pour une unité spécifique
USTRUCT(BlueprintType)
struct FSynergyBonus
{
	GENERATED_BODY()

	// Bonus dégâts multiplicatif (1.0 = aucun)
	UPROPERTY(BlueprintReadOnly)
	float DamageMultiplier = 1.f;

	// Bonus défense additif (%)
	UPROPERTY(BlueprintReadOnly)
	float DefenseBonus = 0.f;

	// Bonus vitesse multiplicatif
	UPROPERTY(BlueprintReadOnly)
	float SpeedMultiplier = 1.f;

	// Réduction cooldown compétences (%)
	UPROPERTY(BlueprintReadOnly)
	float CooldownReduction = 0.f;

	// Bonus moral (absorption des malus)
	UPROPERTY(BlueprintReadOnly)
	float MoraleResistance = 0.f;
};

// Calcule et met en cache les bonus de synergie par unité
// Aquiloris : bonus si unités adjacentes se soutiennent mutuellement
// Noxéens : bonus dans zones bioluminescentes (via TerrainModifier)
UCLASS()
class WOTOL_API UFactionSynergySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Calcule les bonus de synergie pour une unité donnée
	// Appelé dans PerformAttack et TakeDamageFromUnit via AUnitBase
	UFUNCTION(BlueprintPure, Category = "Synergy")
	FSynergyBonus ComputeSynergyBonus(AUnitBase* Unit) const;

	// Notifie le système qu'une zone bioluminescente est active
	// (appelé par BP_Zone ou UObjectiveManager quand Noxéons activent la zone)
	UFUNCTION(BlueprintCallable, Category = "Synergy")
	void SetBioluminescentZoneActive(FName ZoneID, bool bActive);

	UFUNCTION(BlueprintPure, Category = "Synergy")
	bool IsInBioluminescentZone(AUnitBase* Unit) const;

	// Rayon de détection des alliés proches pour Aquiloris (UE units)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synergy")
	float AquilorisSupportRadius = 500.f;

	// Rayon de détection pour la synergie (zone bioluminescente Noxéens)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synergy")
	float NoxeenBioZoneRadius = 800.f;

private:
	// Zones bioluminescentes actives (activées par les Noxéons)
	TSet<FName> ActiveBioluminescentZones;

	FSynergyBonus ComputeAquilorisBonus(AUnitBase* Unit) const;
	FSynergyBonus ComputeNoxeenBonus(AUnitBase* Unit) const;

	int32 CountNearbyAllies(AUnitBase* Unit, float Radius) const;
	bool IsUnitInBioZone(AUnitBase* Unit) const;
};
