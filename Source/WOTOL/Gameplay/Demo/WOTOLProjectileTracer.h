#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLProjectileTracer.generated.h"

class UStaticMeshComponent;

// Petit projectile VISUEL (greybox) : une boule lumineuse qui file du tireur vers la
// cible puis disparaît. Purement cosmétique (les dégâts sont appliqués à part).
UCLASS()
class WOTOL_API AWOTOLProjectileTracer : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLProjectileTracer();

	// Tire une boule de From vers To (couleur = faction).
	static void Fire(UWorld* World, const FVector& From, const FVector& To,
		const FLinearColor& Color, float Size = 1.f);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ball;
	FVector Target = FVector::ZeroVector;
	float Speed = 4000.f;
	float Life = 0.f;
};
