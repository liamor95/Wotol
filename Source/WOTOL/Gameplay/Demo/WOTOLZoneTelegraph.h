#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLZoneTelegraph.generated.h"

class UStaticMeshComponent;

// Disque holographique translucide posé au sol pour matérialiser une ZONE (aperçu avant
// activation d'une competence, ou zone de controle active). Réutilisable par plusieurs
// unités/usages (demande Liamor 29/07/2026) :
//  - Noxeflare : "il faut qu'on voit la zone qui va etre impactee avant l'activation" -> aperçu
//    seul (bAppliesBlindToEnemies = false), détruit après Duration sans effet de jeu.
//  - Aquilombres Axe 2 "Ombres Projetées" : voile sombre façon jet d'encre du Kraken, ~3x la
//    taille de l'unité, qui réduit réellement la précision/visibilité ennemie pendant sa durée
//    (bAppliesBlindToEnemies = true -> rafraîchit AUnitBase::BlindedUntil, MÊME mécanique que
//    AWOTOLInkZone, pas une nouvelle stat inventée).
UCLASS()
class WOTOL_API AWOTOLZoneTelegraph : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLZoneTelegraph();

	// Configuré par l'appelant juste après SpawnActor, avant le premier Tick.
	float Radius = 400.f;
	float Duration = 1.f;
	FLinearColor Color = FLinearColor(0.6f, 0.9f, 1.f, 1.f);

	// Si vrai, aveugle en continu les unités de la faction ADVERSE à CasterFaction tant
	// qu'elles restent dans le rayon (sinon, pur aperçu visuel sans effet de jeu).
	bool bAppliesBlindToEnemies = false;
	EFactionID CasterFaction = EFactionID::None;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY() TObjectPtr<UStaticMeshComponent> Disc;
	float Elapsed = 0.f;
};
