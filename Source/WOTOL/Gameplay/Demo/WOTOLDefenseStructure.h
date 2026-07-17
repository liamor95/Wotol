#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLDefenseStructure.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// Type de défense (greybox). Identité propre à chaque faction :
//  - Aquiloris : TOURELLE cristal (canon hydrosphère / laser d'énergie de cristaux)
//  - Noxéens   : SENTINELLE bioluminescente (poste de tir Nox Blast)
UENUM(BlueprintType)
enum class EWOTOLDefenseType : uint8
{
	AquilorisTurret   UMETA(DisplayName = "Tourelle Aquiloris"),
	NoxeenSentinel    UMETA(DisplayName = "Sentinelle Noxéenne")
};

// ─────────────────────────────────────────────────────────────────────────────
// STRUCTURE DE DÉFENSE (greybox) — posée autour du bâtiment central pour défendre la
// zone (Docs/SYSTEME_CITE_ET_DEFENSE.md). Tire automatiquement sur l'unité ennemie la
// plus proche à portée. A des PV (attaquable plus tard). Pattern tower-defense / They
// Are Billions. Identité visuelle par faction (tourelle cristal / sentinelle verte).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDefenseStructure : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLDefenseStructure();

	// Faction propriétaire (détermine la cible = faction adverse + la couleur).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	EFactionID OwnerFaction = EFactionID::Aquiloris;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	EWOTOLDefenseType DefenseType = EWOTOLDefenseType::AquilorisTurret;

	// Portée de tir (uu). ~14 m.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float Range = 1400.f;

	// Dégâts par tir + cadence (s).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float DamagePerShot = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float FireCooldown = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Defense")
	float MaxHealth = 3000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Defense")
	float CurrentHealth = 3000.f;

	UFUNCTION(BlueprintCallable, Category = "Defense")
	void ApplyDamage(float Amount);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	// Tête pivotante (s'oriente vers la cible).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Head;

	void BuildVisual();
	class AUnitBase* FindTarget() const;

private:
	float FireTimer = 0.f;
};
