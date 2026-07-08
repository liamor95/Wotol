#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLBeam.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UMaterialInstanceDynamic;
class AUnitBase;

// RAYON LASER CONTINU (greybox) : un trait fin et long qui dure ~1 s depuis l'origine.
// Peut être FIXE (vers une cible précise) ou BALAYER l'horizon (yaw start -> end) pour
// toucher plusieurs ennemis alignés devant le lanceur. Purement visuel + dégâts de balayage.
UCLASS()
class WOTOL_API AWOTOLBeam : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLBeam();

	// Origine, orientation de départ/fin (degrés yaw monde), longueur, couleur.
	static AWOTOLBeam* Fire(UWorld* World, const FVector& Origin, float YawStart, float YawEnd,
		float Length, const FLinearColor& Color, AUnitBase* Caster, float SweepDamage);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TObjectPtr<USceneComponent> Pivot;   // tourne (balayage)
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Beam; // le trait
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BeamMID;
	UPROPERTY() TObjectPtr<class UPointLightComponent> Glow;  // lumière fluo (quart proche)
	UPROPERTY() TObjectPtr<class UPointLightComponent> Glow2; // lumière fluo (quart lointain)

	FVector OriginLoc = FVector::ZeroVector;
	float Yaw0 = 0.f, Yaw1 = 0.f, Len = 1000.f, Life = 0.f, Duration = 0.9f, Damage = 0.f;
	TWeakObjectPtr<AUnitBase> CasterUnit;
	TSet<TWeakObjectPtr<AUnitBase>> AlreadyHit;
};
