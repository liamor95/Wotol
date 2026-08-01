#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLInkZone.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class AUnitBase;

// Une bulle du nuage d'encre (petite sphère) + ses paramètres d'animation. BUG CORRIGE
// (01/08/2026, latent depuis sa création — repéré par un audit de suivi le 31/07/2026 mais
// laissé de côté à l'époque comme "pas encore exploitable, changement structurel trop risqué
// sans compilateur") : cette struct était nichée DANS AWOTOLInkZone, en simple `struct` (pas
// USTRUCT — UHT gère mal les USTRUCT nichées dans une UCLASS), avec un `TObjectPtr<...> Mesh`
// SANS UPROPERTY(). Résultat : le Garbage Collector ne voyait PAS ce pointeur comme une
// référence vivante vers le UStaticMeshComponent -> risque de pointeur pendant si jamais rien
// d'autre ne retient l'objet en vie (aujourd'hui protégé uniquement par le fait que ces
// composants restent enregistrés sur l'acteur tant qu'il existe — fragile, pas garanti pour un
// futur changement). Sortie en USTRUCT au niveau fichier + UPROPERTY() sur Mesh : le GC track
// maintenant correctement la référence, comme n'importe quel autre pointeur UObject du projet.
USTRUCT()
struct FWOTOLInkBubble
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

	FVector Home = FVector::ZeroVector; // position locale "haute" (dans le nuage)
	FVector Ground = FVector::ZeroVector; // position locale une fois écoulée au sol
	float Size = 30.f;
	float Phase = 0.f;    // déphasage d'ondulation
	float Drip = 0.f;     // 0..1 : retard d'écoulement (gouttes échelonnées)
};

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

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;

	// UPROPERTY() nécessaire ici (pas juste sur FWOTOLInkBubble::Mesh) : c'est ce qui fait que
	// le GC parcourt effectivement les éléments du tableau pour y trouver l'UPROPERTY nichée.
	UPROPERTY()
	TArray<FWOTOLInkBubble> Bubbles;

	float Elapsed  = 0.f;
	float DotAccum = 0.f; // accumulateur pour le poison (appliqué ~1x/s)
	float FogTopLocal = 300.f; // hauteur locale du nuage (FloatHeight - sol)

	static constexpr float FogTime     = 1.6f; // brouillard flottant (lévitation)
	static constexpr float DescendTime = 1.6f; // écoulement progressif vers le sol
};
