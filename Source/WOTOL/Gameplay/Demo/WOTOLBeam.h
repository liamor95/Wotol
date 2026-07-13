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
	// Pitch (degrés) : inclinaison VERTICALE du rayon -> il peut viser une cible sur une
	// couche de verticalité DIFFÉRENTE (Kraken en lévitation au-dessus, etc.).
	// Thickness : multiplicateur d'épaisseur du rayon (1 = normal ; le Noxedrake tire BEAUCOUP
	// plus gros que le Noxar).
	static AWOTOLBeam* Fire(UWorld* World, const FVector& Origin, float YawStart, float YawEnd,
		float Length, const FLinearColor& Color, AUnitBase* Caster, float SweepDamage,
		float Pitch = 0.f, float Thickness = 1.f, bool bBubbleTrail = false, float LifeTime = 0.9f);

	// SUIVI : le rayon reste ANCRÉ sur la tête du lanceur (position VISUELLE : suit le modèle
	// 3D même quand il change de couche/verticalité pendant le tir) et se ré-oriente vers la
	// cible en continu (rayon mono-cible « continu » façon Noxedrake).
	void SetFollow(AUnitBase* Anchor, AUnitBase* Target, float MuzzleFwdOffset, float MuzzleUpOffset);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TObjectPtr<USceneComponent> Pivot;   // tourne (balayage)
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Beam; // le trait
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BeamMID;
	UPROPERTY() TObjectPtr<class UPointLightComponent> Glow;  // lumière fluo (quart proche)
	UPROPERTY() TObjectPtr<class UPointLightComponent> Glow2; // lumière fluo (quart lointain)

	FVector OriginLoc = FVector::ZeroVector;
	FLinearColor BeamColor = FLinearColor::White;   // couleur pour les bulles de frémissement
	bool bBubbles = false;                           // émet des bulles le long du rayon
	float BubbleAccum = 0.f;                          // cadence d'émission des bulles
	float Yaw0 = 0.f, Yaw1 = 0.f, PitchAngle = 0.f, Len = 1000.f, Life = 0.f, Duration = 0.9f, Damage = 0.f;
	TWeakObjectPtr<AUnitBase> CasterUnit;
	TSet<TWeakObjectPtr<AUnitBase>> AlreadyHit;

	// Suivi de la tête du lanceur (verticalité).
	bool bFollow = false;
	TWeakObjectPtr<AUnitBase> AnchorUnit;  // lanceur à suivre (tête = position visuelle)
	TWeakObjectPtr<AUnitBase> TargetUnit;  // cible à re-viser en continu
	float MuzzleFwd = 0.f, MuzzleUp = 0.f; // décalage bouche depuis le centre visuel
	void UpdateFollow();
};
