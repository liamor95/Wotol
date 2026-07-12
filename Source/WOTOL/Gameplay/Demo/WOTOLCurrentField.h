#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLCurrentField.generated.h"

class UStaticMeshComponent;
class USceneComponent;

// Visualisation du COURANT océanique : une BANDE nette et lumineuse (traînées + nuée de
// bulles) qui file dans le sens du courant, à sa vitesse -> on VOIT distinctement le couloir
// de courant traverser la carte. Purement cosmétique.
UCLASS()
class WOTOL_API AWOTOLCurrentField : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLCurrentField();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// Chaque particule est stockée en coordonnées de BANDE : X = distance le long du courant,
	// Y = décalage perpendiculaire (dans la largeur de la bande), Z = hauteur (couche haute).
	// -> la bande reste alignée sur le courant quelle que soit sa direction (tirée au hasard).
	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Streaks;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bubbles;
	TArray<FVector> StreakCoord; // (along, perp, z)
	TArray<FVector> BubbleCoord;

	float Span      = 9000.f;   // demi-longueur de la bande (le long du courant) -> traverse la carte
	float HalfWidth = 1200.f;   // demi-largeur de la bande (perpendiculaire) -> couloir net
	float ZLow  = 1400.f;
	float ZHigh = 2500.f;
};
