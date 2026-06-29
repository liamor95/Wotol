#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WOTOLDamageNumber.generated.h"

class UTextRenderComponent;

// Petit chiffre de dégâts flottant (ex: "-33") qui monte et disparaît.
// Feedback de combat standard d'un jeu de stratégie.
UCLASS()
class WOTOL_API AWOTOLDamageNumber : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLDamageNumber();

	void Init(float Amount, const FLinearColor& Color);

	// Crée un chiffre de dégâts à une position du monde
	static AWOTOLDamageNumber* Spawn(UWorld* World, const FVector& Loc,
		float Amount, const FLinearColor& Color);

protected:
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Text;

	float Age  = 0.f;
	float Life = 1.1f;
};
