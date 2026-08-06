#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGreyboxEnvironment.generated.h"

class AStaticMeshActor;
class AWOTOLCityBuildingProp;
class UDemoFlowSubsystem;

// ─────────────────────────────────────────────────────────────────────────────
// DÉCOR GREYBOX CONTEXTUALISÉ (section 3) — construit par code une arène lisible :
//   - un sol (fond marin) ;
//   - une arche/ruine centrale servant de repère ;
//   - deux zones de déploiement colorées (joueur vs rival).
// Formes primitives Unreal uniquement, aucun asset externe. Présentable en réunion.
//
// DEPUIS LE 02/08/2026 (demande explicite et répétée de Liamor, "utilise le MÊME plateau que
// celui pour les batailles mais remodèle-le pour que ça ressemble à la cité") : ce MÊME acteur
// sert AUSSI de décor pour l'écran Cité, à la place de l'ancien AWOTOLCityEnvironment (disque
// séparé posé à 30000 unités de l'arène — supprimé). RebuildAsCity()/RebuildFromCity() basculent
// le MÊME emplacement entre "terrain de bataille" et "cité" exactement comme RebuildForPhase()
// bascule déjà entre les variantes de phase — aucun autre acteur, aucune autre carte.
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

	// Bascule ce MÊME emplacement en mode CITÉ (nettoie puis reconstruit une disposition
	// radiale de tours-cristal au lieu du terrain de bataille) — appelé une seule fois par
	// AWOTOLDemoDirector::HandleScreenChanged en entrant sur EDemoScreen::City. Mémorise le
	// variant de bataille en cours pour RebuildFromCity(). Sans effet si déjà en mode cité.
	// NewFaction : passée explicitement par l'appelant (Director->CachedPlayerFaction, déjà
	// résolue de façon fiable) plutôt que de relire PlayerFaction ci-dessus, qui peut être
	// périmé (figé à sa valeur par défaut Aquiloris si défini avant le choix de faction du
	// joueur — même classe de bug déjà rencontrée et corrigée sur l'ancien
	// AWOTOLCityEnvironment le 02/08/2026).
	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void RebuildAsCity(EFactionID NewFaction);

	// Restaure le terrain de bataille (le variant actif avant RebuildAsCity()) en quittant la
	// cité — appelé par HandleScreenChanged sur tout écran != City. Sans effet si pas en
	// mode cité (évite un rebuild coûteux à chaque changement d'écran hors sujet).
	UFUNCTION(BlueprintCallable, Category = "Greybox")
	void RebuildFromCity();

	bool IsCityMode() const { return bCityMode; }

	// Position de référence pour cadrer AWOTOLCityCamera (même emplacement que l'arène).
	FVector GetCityHubLocation() const { return GetActorLocation(); }

	// Rafraîchit l'aspect (niveau/verrouillage/sélection) des bâtiments de la cité — appelé
	// chaque frame pendant EDemoScreen::City uniquement (voir Tick()).
	void RefreshCityProps(const UDemoFlowSubsystem* Demo);

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
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<class APostProcessVolume> ArenaPPV;
	UPROPERTY()
	TObjectPtr<class AExponentialHeightFog> ArenaFog;
	UPROPERTY()
	TObjectPtr<class ASkyLight> ArenaSkyLight;

	// ── Mode CITÉ (même emplacement que l'arène, voir RebuildAsCity/RebuildFromCity) ──
	bool bCityMode = false;
	int32 PreCityVariant = 1;
	UPROPERTY()
	TArray<TObjectPtr<AWOTOLCityBuildingProp>> CityProps;

	void BuildCityLayout();
	// Tour-cristal à ÉTAGES DÉCROISSANTS (base large -> flèche fine), PAS un simple amas de
	// pointes façon oursin — rejeté explicitement par Liamor ("formes dégueulasses") pour les
	// bâtiments de la cité. Silhouette conique à niveaux, cônes d'accent aux quatre coins de la
	// base, dans l'esprit des tours des planches de référence envoyées par Liamor.
	void SpawnCrystalTower(const FVector& Base, float BaseRadius, float Height,
		const FLinearColor& Color, int32 Seed);

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
