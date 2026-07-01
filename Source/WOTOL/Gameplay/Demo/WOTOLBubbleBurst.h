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

private:
	UPROPERTY() TObjectPtr<USceneComponent> Root;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Bubbles;
	TArray<FVector> Vels;
	TArray<FVector> BaseScales;
	float Life = 0.f;
	float MaxLife = 1.0f;
};
