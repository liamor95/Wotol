#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLProjectileBase.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class AUnitBase;

UCLASS(Blueprintable)
class WOTOL_API AWOTOLProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLProjectileBase();

	// Appelé juste après le spawn pour initialiser le projectile
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitProjectile(AUnitBase* InInstigator, AUnitBase* InTarget, float InDamage);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float InitialSpeed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxSpeed = 1200.f;

	// Effet visuel à jouer à l'impact — assigné dans le BP héritant
	UFUNCTION(BlueprintImplementableEvent, Category = "Projectile")
	void OnImpact(AUnitBase* HitUnit);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

private:
	UPROPERTY()
	TWeakObjectPtr<AUnitBase> InstigatorUnit;

	UPROPERTY()
	TWeakObjectPtr<AUnitBase> TargetUnit;

	float Damage = 0.f;

	void TrackTarget();
};
