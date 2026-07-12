#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLAmbientFish.generated.h"

class UStaticMeshComponent;

// Espèces de faune ambiante (décor). Chacune a une SILHOUETTE reconnaissable, bâtie à partir
// de primitives (BasicShapes), et NAGE en ondulant (corps + queue qui fouettent l'eau).
UENUM()
enum class EFishSpecies : uint8
{
	SmallFish, // petit poisson de banc
	Shark,     // requin : aileron dorsal triangulaire, queue en croissant
	Dolphin,   // dauphin : rostre, aileron courbe, nageoires
	Orca,      // orque : aileron dorsal HAUT et droit, corps massif
	Whale,     // baleine : corps énorme arrondi, nageoire caudale horizontale
	Ray        // raie : corps plat en losange, longue queue fine
};

// Faune ambiante purement décorative : nage en boucle (cercle + houle) avec ONDULATION du
// corps. Sans collision, sans interaction gameplay. Léger (quelques meshes + Tick trigo).
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
		float InPhase, float InHeightAmp, float InBaseZ, const FLinearColor& Color, float SizeM,
		EFishSpecies InSpecies = EFishSpecies::SmallFish);

protected:
	// Fabrique une pièce de mesh (corps, aileron, queue…) attachée à un parent.
	UStaticMeshComponent* MakePart(USceneComponent* Parent, const TCHAR* MeshPath,
		const FVector& RelLoc, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color);

	UPROPERTY() TObjectPtr<USceneComponent> Hull;   // pivot du corps (ondule légèrement)
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
	// Segments arrière (queue) : ondulent en vague progressive -> nage crédible.
	UPROPERTY() TArray<TObjectPtr<USceneComponent>> TailJoints;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Fins; // pectorales/ailerons (léger battement)

	EFishSpecies Species = EFishSpecies::SmallFish;
	float Angle = 0.f;
	float SwimRate = 6.f; // vitesse de battement (ondulation)
	float BodyLen  = 100.f;
};
