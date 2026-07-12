#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLAmbientFish.generated.h"

class UStaticMeshComponent;
class USceneComponent;

// Espèces de faune ambiante (décor). Chacune a une SILHOUETTE reconnaissable : un CORPS
// FUSELÉ unique (pas d'amas de boules) + des nageoires PLATES caractéristiques.
UENUM()
enum class EFishSpecies : uint8
{
	SmallFish, // petit poisson de banc
	Shark,     // requin : aileron dorsal triangulaire haut, museau, queue en croissant
	Dolphin,   // dauphin : rostre, aileron dorsal courbe
	Orca,      // orque : aileron dorsal TRÈS haut et droit, ventre clair
	Whale,     // baleine : corps massif, petite dorsale, caudale horizontale
	Ray        // raie : corps plat en losange, ailes qui battent, longue queue
};

// Faune ambiante purement décorative : nage en boucle (cercle + houle). La QUEUE bat et le
// corps ondule doucement -> nage crédible. Sans collision, léger.
UCLASS()
class WOTOL_API AWOTOLAmbientFish : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLAmbientFish();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() FVector CenterPoint = FVector::ZeroVector;
	UPROPERTY() float Radius   = 1500.f;
	UPROPERTY() float Speed    = 0.4f;
	UPROPERTY() float Phase    = 0.f;
	UPROPERTY() float HeightAmp = 120.f;
	UPROPERTY() float BaseZ    = 300.f;

	void Configure(const FVector& InCenter, float InRadius, float InSpeed,
		float InPhase, float InHeightAmp, float InBaseZ, const FLinearColor& Color, float SizeM,
		EFishSpecies InSpecies = EFishSpecies::SmallFish);

protected:
	// Ajoute une pièce (corps, nageoire…) attachée à un parent. Mesh : "sphere"/"cone"/"cube".
	UStaticMeshComponent* AddMesh(USceneComponent* Parent, const TCHAR* MeshPath,
		const FVector& RelLoc, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color);

	UPROPERTY() TObjectPtr<USceneComponent> Hull;       // corps entier (ondule légèrement en lacet)
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;  // corps fuselé unique
	UPROPERTY() TObjectPtr<USceneComponent> TailPivot;  // articulation de queue (bat)
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wings; // ailes de raie (battement roulis)

	EFishSpecies Species = EFishSpecies::SmallFish;
	float Angle = 0.f;
	float SwimRate = 5.f;
	float BodyLen  = 200.f;
	float TrailTimer = 0.f;
};
