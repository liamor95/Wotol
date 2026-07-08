#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLCurrentField.generated.h"

class UStaticMeshComponent;
class USceneComponent;

// Visualisation du COURANT océanique : des traînées translucides qui dérivent sur les
// COUCHES HAUTES, dans le sens (et à la vitesse) du courant. Purement cosmétique et léger.
UCLASS()
class WOTOL_API AWOTOLCurrentField : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLCurrentField();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TObjectPtr<USceneComponent> SceneRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Streaks;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bubbles; // bulles portées par le courant
	TArray<FVector> Base; // positions locales de base (pour le bouclage)

	float Span = 3600.f;   // étendue XY autour du centre
	float ZLow = 1500.f;   // début des couches hautes
	float ZHigh = 2400.f;  // sommet
};
