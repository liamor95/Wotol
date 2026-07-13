#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLCoverStructure.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UTextRenderComponent;

// ─────────────────────────────────────────────────────────────────────────────
// STRUCTURE DE DÉCOR (ruine technologique Éthérienne : pilier, pan de mur…).
//   - COLLISION : bloque les unités (elles la contournent) et les TIRS à distance
//     (couverture : se cacher derrière protège des projectiles/rayons).
//   - PV : certaines sont INDESTRUCTIBLES (bIndestructible), d'autres tombent sous
//     le feu. À la destruction -> DÉBRIS : dégâts de zone aux unités proches
//     (on peut détruire un pilier au-dessus d'ennemis pour les blesser).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLCoverStructure : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLCoverStructure();

	// Forme : 0 = grand pilier, 1 = pan de mur en ruine, 2 = arche brisée.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	int32 Variant = 0;

	// Indestructible = couverture permanente (aucun PV consommé).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	bool bIndestructible = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cover")
	float MaxHealth = 1200.f;

	UPROPERTY(BlueprintReadOnly, Category = "Cover")
	float CurrentHealth = 1200.f;

	// Encaisse des dégâts (ignoré si indestructible). À 0 -> effondrement + débris.
	UFUNCTION(BlueprintCallable, Category = "Cover")
	void TakeCoverDamage(float Amount, class AUnitBase* InstigatorUnit);

	UFUNCTION(BlueprintPure, Category = "Cover")
	bool IsDestroyed() const { return bDestroyed; }

	bool IsIndestructible() const { return bIndestructible; }
	float GetPillarLen() const { return PillarLen; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void BuildVisual();
	void Collapse();               // déclenche la CHUTE (bascule) vers les unités proches
	void TickFall(float DeltaSeconds); // anime la bascule + écrase les unités sur le passage

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TObjectPtr<UTextRenderComponent> HealthTag; // PV (visible si endommagé)

	// FEEDBACK D'INTERACTION : une VAGUE DE LUMIÈRE qui monte de bas en haut À TRAVERS le
	// MODÈLE lui-même (les pièces s'illuminent en émissif + bloom quand l'onde les traverse,
	// puis reviennent en pierre) -> exactement comme la lumière qui jaillit du cristal. Pas de
	// carré plat.
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> PartRestMID;  // matériau normal (pierre)
	UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> PartPulseMID; // matériau émissif (onde)
	TArray<float>  PartZ;        // hauteur (Z relatif) de chaque pièce
	TArray<uint8>  PartGlowing;  // pièce actuellement illuminée ?
	float ScanTimer = 0.f;
	float ScanTop   = 700.f;     // hauteur balayée

	bool  bDestroyed = false;
	float DebrisRadius = 380.f;
	float DebrisDamage = 190.f;

	// ── Effondrement en FRAGMENTS (chaque morceau vole dans SA direction et retombe à SON
	// endroit -> plusieurs points de chute, pas un seul bloc). ──
	bool    bFalling = false;
	float   FallElapsed = 0.f;
	float   FallDuration = 4.5f;              // durée max avant de figer les gravats (chute lente)
	FVector FallDir = FVector(1.f, 0.f, 0.f); // biais d'éjection (sens du tir)
	float   PillarLen = 700.f;                // hauteur (position du texte / burst haut)
	// État physique par morceau (parallèle à Parts).
	TArray<FVector> PartVel;
	TArray<FVector> PartAngAxis;
	TArray<float>   PartAngSpeed;
	TArray<uint8>   PartLanded;

	FVector LastFireDir = FVector::ZeroVector; // direction du dernier tir reçu (sens d'éjection)
	bool    bHasFireDir = false;
};
