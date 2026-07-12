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

	// Crée un texte flottant libre (ex: "Esquive", "Paré")
	static AWOTOLDamageNumber* SpawnText(UWorld* World, const FVector& Loc,
		const FString& Label, const FLinearColor& Color);

	// ACCROCHE le texte à un composant (ex: le VisualRoot d'une unité) : il SUIT l'unité
	// et reste à SA hauteur de couche verticale, avec un petit décalage latéral aléatoire
	// -> chaque unité a son propre chiffre, plus d'empilement illisible au niveau du sol.
	void SetFollow(class USceneComponent* Comp, const FVector& LocalOffset);

	// Plafond global de chiffres flottants vivants (evite des centaines de TextRender en
	// phase 3 -> lag + bouillie illisible). Au-dela, on n'en cree plus.
	static int32 LiveCount;
	static int32 MaxLive;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> Text;

	TWeakObjectPtr<class USceneComponent> Follow; // composant suivi (VisualRoot de l'unité)
	FVector FollowOffset = FVector::ZeroVector;    // décalage local (autour de l'unité)
	float   Rise = 0.f;                            // montée cumulée du texte

	float Age  = 0.f;
	float Life = 1.5f;
};
