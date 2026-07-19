#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLCityEnvironment.generated.h"

class USceneComponent;
class AWOTOLCityBuildingProp;

// ─────────────────────────────────────────────────────────────────────────────
// DÉCOR GREYBOX DE LA CITÉ (kitbash, sans asset) — sol + hub central décoratif + un
// AWOTOLCityBuildingProp par catégorie productible (même ordre que
// AWOTOLDemoHUD::CityCardCategory), disposés en anneau autour du hub. Existe UNE SEULE
// fois, posé loin de l'arène de bataille/exploration (aucun chevauchement possible : la
// démo réutilise un seul niveau pour tout le reste). Vu depuis AWOTOLCityCamera, la
// caméra isométrique fixe possédée pendant EDemoScreen::City.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLCityEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLCityEnvironment();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City")
	EFactionID PlayerFaction = EFactionID::Aquiloris;

	// Rayon de l'anneau où sont disposés les bâtiments de production autour du hub.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City")
	float RingRadius = 1500.f;

	// Position du hub (centre de la cité) — sert à cadrer la caméra isométrique dessus.
	UFUNCTION(BlueprintPure, Category = "City")
	FVector GetHubLocation() const { return GetActorLocation(); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BuildEnvironment();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TArray<TObjectPtr<AWOTOLCityBuildingProp>> Props;
};
