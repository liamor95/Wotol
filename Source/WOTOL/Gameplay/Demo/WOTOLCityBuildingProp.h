#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "DemoFlowSubsystem.h" // EDemoUnitCategory
#include "WOTOLCityBuildingProp.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UDemoFlowSubsystem;

// ─────────────────────────────────────────────────────────────────────────────
// BÂTIMENT DE PRODUCTION DE LA CITÉ (greybox, kitbash) — un par catégorie productible
// (même ordre que AWOTOLDemoHUD::CityCardCategory), placé en anneau par
// AWOTOLCityEnvironment. Cliquable (ECC_WorldStatic) depuis la caméra isométrique -> le
// PlayerController appelle UDemoFlowSubsystem::SetSelectedCityCategory() pour ouvrir la
// fiche technique dans le HUD. L'aspect (niveau/verrouillage/sélection) suit l'état de la
// démo via Refresh(), rappelée chaque frame par l'environnement pendant l'écran Cité.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLCityBuildingProp : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLCityBuildingProp();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City")
	EDemoUnitCategory Category = EDemoUnitCategory::Infanterie;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "City")
	EFactionID OwnerFaction = EFactionID::Aquiloris;

	// Recale l'aspect (hauteur = niveau, teinte = verrouillé/actif, anneau = sélectionné)
	// sur l'état courant de la démo. Bon marché (pas de recréation de composants au repos).
	void Refresh(const UDemoFlowSubsystem* Demo);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	// Socle : seul élément avec collision (cible du clic caméra isométrique).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	// Corps du bâtiment : la hauteur reflète le niveau (1/2/3).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> TierMesh;

	// Anneau lumineux au sol, visible uniquement quand ce bâtiment est sélectionné.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> SelectionRing;

private:
	void BuildVisual();

	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> TierMID;

	// Cache pour éviter de retoucher les composants quand rien n'a changé.
	int32 LastLevel = -1;
	uint8 LastState = 255; // 0=verrouillé 1=à construire(Distance) 2=actif
	bool  bLastSelected = false;
};
