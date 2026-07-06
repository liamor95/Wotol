#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLInkZone.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class AUnitBase;

// FLAQUE D'ENCRE du Kraken : tache sombre IRRÉGULIÈRE au sol (pas un cercle net), posée
// par le jet d'encre. Tant qu'elle est active, les unités qui s'y trouvent sont RALENTIES
// et voient leur PRÉCISION chuter (aveuglement), et subissent un léger poison d'encre.
// Se dissipe après quelques secondes.
UCLASS()
class WOTOL_API AWOTOLInkZone : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLInkZone();

	// Configuré par le Kraken au spawn.
	float Radius   = 380.f;   // rayon d'effet (~7-8 m de diamètre)
	float Lifetime = 8.f;     // durée avant dissipation
	TWeakObjectPtr<AUnitBase> Caster; // le Kraken (pour attribuer le poison)

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Blobs;

	float Elapsed = 0.f;
	float DotAccum = 0.f; // accumulateur pour le poison (appliqué ~1x/s)
};
