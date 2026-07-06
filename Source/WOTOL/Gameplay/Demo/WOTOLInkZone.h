#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLInkZone.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class AUnitBase;

// NUAGE D'ENCRE du Kraken : un vrai VOLUME de bulles sombres (fumée/encre sous-marine) qui
// FLOTTE et ondule ~1,5 s à la hauteur du crachat, puis S'ÉCOULE GOUTTE À GOUTTE vers le sol
// (progressif, pas en bloc) pour former une FLAQUE qui persiste. Les unités prises dedans
// sont ralenties, aveuglées (précision ~0) et subissent un léger poison.
UCLASS()
class WOTOL_API AWOTOLInkZone : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLInkZone();

	// Configuré par le Kraken au spawn.
	float Radius      = 400.f;   // rayon d'EFFET (ralenti/aveuglement) au sol
	float Lifetime    = 9.5f;    // durée totale (nuage + écoulement + flaque)
	float FloatHeight = 300.f;   // hauteur MONDE où l'encre est crachée (couche du Kraken)
	TWeakObjectPtr<AUnitBase> Caster; // le Kraken (pour attribuer le poison)

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Une bulle du nuage (petite sphère) + ses paramètres d'animation.
	struct FBubble
	{
		TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
		FVector Home = FVector::ZeroVector; // position locale "haute" (dans le nuage)
		FVector Ground = FVector::ZeroVector; // position locale une fois écoulée au sol
		float Size = 30.f;
		float Phase = 0.f;    // déphasage d'ondulation
		float Drip = 0.f;     // 0..1 : retard d'écoulement (gouttes échelonnées)
	};

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	TArray<FBubble> Bubbles;

	float Elapsed  = 0.f;
	float DotAccum = 0.f; // accumulateur pour le poison (appliqué ~1x/s)
	float FogTopLocal = 300.f; // hauteur locale du nuage (FloatHeight - sol)

	static constexpr float FogTime     = 1.6f; // brouillard flottant (lévitation)
	static constexpr float DescendTime = 1.6f; // écoulement progressif vers le sol
};
