#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLAmbientFish.generated.h"

class UStaticMeshComponent;

// Faune ambiante purement décorative : un poisson qui nage en boucle (cercle + houle).
// Sans collision, sans interaction gameplay. Léger (1 mesh + Tick trigonométrique).
UCLASS()
class WOTOL_API AWOTOLAmbientFish : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLAmbientFish();
	virtual void Tick(float DeltaSeconds) override;

	// Paramètres de trajectoire (réglés au spawn par l'environnement)
	UPROPERTY() FVector CenterPoint = FVector::ZeroVector;
	UPROPERTY() float Radius   = 1500.f;
	UPROPERTY() float Speed    = 0.4f;   // rad/s
	UPROPERTY() float Phase    = 0.f;
	UPROPERTY() float HeightAmp = 120.f;
	UPROPERTY() float BaseZ    = 300.f;

	void Configure(const FVector& InCenter, float InRadius, float InSpeed,
		float InPhase, float InHeightAmp, float InBaseZ, const FLinearColor& Color, float SizeM);

protected:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Tail;
	float Angle = 0.f;
};
