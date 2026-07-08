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

	// Tire un projectile de From vers To (couleur = faction).
	// bBolt = true -> ovale ALLONGÉ (trait/projectile, ex. Noxeblast) orienté vers la cible.
	// bBolt = false -> petite SPHÈRE (ex. Aquisphères), taille d'un vrai projectile.
	static void Fire(UWorld* World, const FVector& From, const FVector& To,
		const FLinearColor& Color, float Size = 1.f, bool bBolt = false);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Ball; // cœur lumineux
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Halo; // halo plus large (meilleure visibilité)
	UPROPERTY() TObjectPtr<class UPointLightComponent> Glow; // source de lumière rattachée
	FVector Target = FVector::ZeroVector;
	float Speed = 2600.f; // plus lent = mieux suivi à l'œil
	float Life = 0.f;
};
