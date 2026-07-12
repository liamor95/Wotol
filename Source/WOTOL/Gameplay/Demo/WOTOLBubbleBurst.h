#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLBubbleBurst.generated.h"

class UStaticMeshComponent;

// VFX greybox : petit éclat de bulles qui montent et rétrécissent puis disparaissent.
// 100 % C++ (pas de système de particules requis). Utilisé aux impacts de combat.
UCLASS()
class WOTOL_API AWOTOLBubbleBurst : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLBubbleBurst();

	// Crée un éclat de bulles à un endroit donné.
	static void Burst(UWorld* World, const FVector& Loc, const FLinearColor& Color, int32 Count = 6);

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// PLAFOND GLOBAL d'éclats de bulles vivants : en phase 3 (des dizaines de pouvoirs +
	// sillages) ils se comptaient par centaines -> gros lag. Au-delà du plafond, Burst() ne
	// spawne plus (les bulles existantes finissent leur vie) -> le coût reste borne.
	static int32 LiveCount;
	static int32 MaxLive;

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bubbles;
	TArray<FVector> Vels;
	TArray<FVector> BaseScales;
	float Life = 0.f;
	float MaxLife = 1.0f;
};
