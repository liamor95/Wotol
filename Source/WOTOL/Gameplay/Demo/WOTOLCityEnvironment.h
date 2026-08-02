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

	// Rebâtit intégralement le décor (bâtiments/labels/fond/couleurs) pour la faction
	// RÉELLEMENT choisie par le joueur. NÉCESSAIRE (bug terrain du 02/08/2026, "toujours les
	// visuels des Aquiloris en jouant Noxéens") : cet acteur est créé dans
	// AWOTOLGameMode_Demo::BeginPlay, qui s'exécute AVANT que le joueur ait choisi sa faction
	// sur l'écran de sélection (Demo->GetPlayerFaction() renvoie encore None à cet instant ->
	// repli sur la valeur par défaut Aquiloris de PlayerFaction ci-dessus). Sans ce rappel,
	// la cité restait visuellement Aquiloris même en jouant Noxéens, quelle que soit la vraie
	// faction. Appelé une fois par AWOTOLDemoDirector::StartDemoAfterSelection(), au moment où
	// la faction devient définitivement connue.
	void RebuildForFaction(EFactionID NewFaction);

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

	// Ruines/architecture (arches, colonnades, escaliers, dallage) — MÊME vocabulaire
	// architectural que le terrain de bataille/exploration (AWOTOLGreyboxEnvironment::
	// SpawnArch/SpawnColonnade/SpawnStairs/SpawnPlaza), demande explicite de Liamor du
	// 02/08/2026 : "refais pareil que pour la phase 2 -> phase 3, mais pour la cité" (même
	// technique de reconstruction de décor, PAS un partage du même acteur/de la même carte —
	// clarifié explicitement, aucune fusion des deux systèmes). Portée ici en composants
	// attachés à SceneRoot (plutôt que les AStaticMeshActor world-space de GreyboxEnvironment)
	// pour rester compatible avec RebuildForFaction() ci-dessus, qui détruit/reconstruit tous
	// les enfants de SceneRoot.
	void BuildRuinsDecor();
	void SpawnCityArch(const FVector& Base, float Radius, float YawDeg, const FLinearColor& Color);
	void SpawnCityColonnade(const FVector& Start, const FVector& Step, int32 Count,
		float Height, const FLinearColor& Color, int32 Seed);
	void SpawnCityStairs(const FVector& Base, float YawDeg, int32 Steps,
		float Width, const FLinearColor& Color);
	void SpawnCityPlaza(const FVector& Center, float HalfX, float HalfY, const FLinearColor& Color);

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
