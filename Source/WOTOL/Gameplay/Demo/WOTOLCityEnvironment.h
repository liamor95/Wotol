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
	// RÉDUIT de 1500 à 700 (retour terrain 31/07/2026, recherche dédiée) : à 1500, avec l'angle
	// fixe de la caméra isométrique (Pitch -55°) et l'OrthoWidth par défaut (2400, demi-largeur
	// 1200), les 5 bâtiments de l'anneau tombaient TOUS hors du cadre visible (calcul : décalage
	// vertical à l'écran ≈1229 unités pour une demi-hauteur de cadre ≈675 sur un ratio 16:9) —
	// masqué côté Aquiloris par le fond peint (qui donne l'illusion d'une cité même sans les
	// bâtiments 3D visibles), mais totalement visible côté Noxéens une fois le mauvais fond
	// désactivé : plus aucun bâtiment à l'écran, juste le hub + les bulles ambiantes.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City")
	float RingRadius = 700.f;

	// Position du hub (centre de la cité) — sert à cadrer la caméra isométrique dessus.
	UFUNCTION(BlueprintPure, Category = "City")
	FVector GetHubLocation() const { return GetActorLocation(); }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BuildEnvironment();
	void BuildAmbientBubbles();
	void TickAmbientBubbles(float DeltaSeconds);
	// Chemin pavé plat entre le hub et un bâtiment + amas de décor (corail/rochers) dispersés
	// sur le sol — casse l'aspect "anneau géométrique nu" (retour de Liamor le 31/07/2026,
	// références Call of Dragons : terrain organique, chemins visibles, végétation en bordure).
	void AddCityPath(const FVector& From, const FVector& To);
	void BuildGroundDecor();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TArray<TObjectPtr<AWOTOLCityBuildingProp>> Props;

	// Grand fond de cité (illustration officielle, WOTOLBuildingArt::GetCityBackdrop) posé
	// loin derrière la scène, orienté face à la caméra isométrique fixe. Absent (nullptr) si
	// le fichier officiel n'existe pas : le décor kitbash existant reste seul visible.
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> BackdropMesh;

	// Petites bulles animées qui montent en boucle — demande de Liamor du 25/07/2026 : garder
	// la vue Cité "vivante" malgré des bâtiments désormais en illustrations 2D (plates) plutôt
	// qu'en kitbash 3D. Purement décoratif, aucune collision.
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> AmbientBubbles;
	TArray<float> BubblePhase;
	TArray<float> BubbleSpeed;
	TArray<FVector> BubbleOrigin;
};
