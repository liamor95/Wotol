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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void BuildVisual();
	void Collapse(); // effondrement : débris (dégâts de zone) + retire la collision

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY() TObjectPtr<UTextRenderComponent> HealthTag; // PV (visible si endommagé)

	bool  bDestroyed = false;
	float DebrisRadius = 550.f;
	float DebrisDamage = 220.f;
};
