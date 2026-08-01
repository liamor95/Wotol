#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGreyboxEnvironment.generated.h"

class AStaticMeshActor;

// ─────────────────────────────────────────────────────────────────────────────
// DÉCOR GREYBOX CONTEXTUALISÉ (section 3) — construit par code une arène lisible :
//   - un sol (fond marin) ;
//   - une arche/ruine centrale servant de repère ;
//   - deux zones de déploiement colorées (joueur vs rival).
// Formes primitives Unreal uniquement, aucun asset externe. Présentable en réunion.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLGreyboxEnvironment : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLGreyboxEnvironment();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	EFactionID PlayerFaction = EFactionID::Aquiloris;

	// Demi-écart entre les deux zones de déploiement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	float ArmySeparation = 2500.f;

	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void BuildArena();

	// VARIANT de terrain : 1 = arène de base (phases 1/2) ; 3 = zone ABYSSALE (phase 3),
	// palette + brume + disposition (seeds) DIFFÉRENTES pour donner « un autre lieu ».
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Greybox")
	int32 Variant = 1;

	// La lumière directionnelle du niveau n'est atténuée qu'UNE fois (sinon un rebuild la
	// multiplierait à nouveau et assombrirait la scène à chaque phase).
	bool bLightTuned = false;

	// Détruit tout le décor déjà généré (tous les acteurs appartenant à cet environnement).
	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void ClearArena();

	// Reconstruit l'arène pour une PHASE donnée (nettoie puis régénère avec le bon variant).
	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void RebuildForPhase(int32 Phase);

	// Active/désactive l'ambiance de bataille (brouillard + post-process + lumière du ciel).
	// BUG CORRIGE (retour terrain 31/07/2026, recherche dédiée) : ce décor n'est créé QU'UNE
	// FOIS pour toute la session (WOTOLGameMode_Demo::BeginPlay) et jamais détruit -> son
	// PostProcessVolume/brouillard/SkyLight (tous bUnbound=true) restaient actifs sur TOUS les
	// écrans, y compris la Cité, la poussant vers un bleu bien plus saturé que sa couleur
	// codée. Appelé depuis HandleScreenChanged pour ne l'activer QUE pendant les écrans de
	// bataille/exploration où il a été conçu.
	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void SetAtmosphereActive(bool bActive);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<class APostProcessVolume> ArenaPPV;
	UPROPERTY()
	TObjectPtr<class AExponentialHeightFog> ArenaFog;
	UPROPERTY()
	TObjectPtr<class ASkyLight> ArenaSkyLight;

	AStaticMeshActor* SpawnBlock(const TCHAR* MeshPath, const FVector& Loc,
		const FVector& Scale, const FLinearColor& Color,
		const FRotator& Rot = FRotator::ZeroRotator, bool bBlocking = true);

	// Mesh ÉMISSIF (l'objet RAYONNE lui-même) : pour le décor bioluminescent (coraux,
	// algues) -> il brille et n'a plus l'air éteint. EmissiveColor en HDR (>1 = glow).
	AStaticMeshActor* SpawnGlowBlock(const TCHAR* MeshPath, const FVector& Loc,
		const FVector& Scale, const FLinearColor& EmissiveColor,
		const FRotator& Rot = FRotator::ZeroRotator);

	// Petite lampe bioluminescente « naturelle » (halo doux qui éclaire le fond autour
	// de l'organisme). Rayon modéré : ça éclaire le décor sans tout inonder.
	void SpawnBioLight(const FVector& Loc, const FLinearColor& Color, float Intensity, float Radius);

	// ─── Kitbash : assemblage de primitives pour un rendu crédible ────────────
	// Rocher = amas de cubes/sphères de tailles/rotations variées (graine = variété).
	void SpawnRock(const FVector& Center, float Size, const FLinearColor& Color, int32 Seed);
	// Chaîne de montagnes sous-marines = cônes chevauchants le long d'une ligne.
	void SpawnRidge(const FVector& Start, const FVector& End, float Height,
		float Width, const FLinearColor& Color, int32 Seed);
	// Ziggourat à gradins (pyramide à étages) avec un escalier frontal.
	void SpawnZiggurat(const FVector& Base, float BaseHalf, int32 Tiers,
		float TierHeight, float YawDeg, const FLinearColor& Color);
	// Escalier (volées de marches) orienté.
	void SpawnStairs(const FVector& Base, float YawDeg, int32 Steps,
		float Width, const FLinearColor& Color);
	// Colonnade : rangée de colonnes (cylindres) éventuellement brisées.
	void SpawnColonnade(const FVector& Start, const FVector& Step, int32 Count,
		float Height, const FLinearColor& Color, int32 Seed);
	// Arche de pierre (anneau de blocs) — repère central de l'arène.
	void SpawnArch(const FVector& Base, float Radius, float YawDeg, const FLinearColor& Color);
	// Dallage : plateau de pierre carrelé légèrement surélevé.
	void SpawnPlaza(const FVector& Center, float HalfX, float HalfY, const FLinearColor& Color);
};
