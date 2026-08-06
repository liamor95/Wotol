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
// AWOTOLGreyboxEnvironment::BuildCityLayout. Cliquable (ECC_WorldStatic) depuis la caméra
// isométrique -> le PlayerController appelle UDemoFlowSubsystem::SetSelectedCityCategory()
// pour ouvrir la fiche technique dans le HUD. L'aspect (niveau/verrouillage/sélection) suit
// l'état de la démo via Refresh(), rappelée chaque frame par l'environnement pendant l'écran
// Cité.
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

	// Corps du bâtiment : silhouette DISTINCTE par faction (02/08/2026, "chaque faction a sa
	// cité") — tour-cristal à étages pour Aquiloris, amas organique de pointes + pods
	// bioluminescents pour Noxéens. Remplace l'ancien plan texturé avec illustration officielle
	// collée (trompe-l'œil rejeté explicitement par Liamor : "tu dois faire des formes
	// toi-même, pas coller une image"). La TAILLE (échelle uniforme de TierCluster) reflète
	// le niveau (1/2/3).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> TierCluster;

	// Anneau lumineux au sol, visible uniquement quand ce bâtiment est sélectionné.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> SelectionRing;

private:
	void BuildVisual();

	// Matériaux dynamiques PARTAGÉS par toutes les pièces du cluster : Refresh() n'a besoin que
	// de deux SetVectorParameterValue pour changer tout le bâtiment d'un coup. TierMID = pièces
	// émissives (flèche Aquiloris, pods Noxéens) ; TierMatteMID = pièces mates (épines/cocon
	// Noxéens uniquement — reste nullptr côté Aquiloris, RAS pour Refresh()).
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> TierMID;
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> TierMatteMID;

	// Cache pour éviter de retoucher les composants quand rien n'a changé.
	int32 LastLevel = -1;
	uint8 LastState = 255; // 0=verrouillé 1=à construire(Distance) 2=actif
	bool  bLastSelected = false;
};
