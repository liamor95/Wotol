#pragma once

#include "CoreMinimal.h"
#include "Gameplay/Units/UnitBase.h"
#include "WOTOLDemoUnit.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class USceneComponent;

// ─────────────────────────────────────────────────────────────────────────────
// UNITÉ GREYBOX CONTEXTUALISÉE (sections 3-5 du cahier des charges).
// Forme symbolique selon la CATÉGORIE (rôle), TAILLE réelle du GDD, COULEUR de
// faction (bleu Aquiloris / vert Noxéen). Nom lisible. 100 % additif : hérite de
// AUnitBase, ne modifie rien. Remplaçable par un vrai mesh Meshy plus tard.
//
// Formes : Chef = cylindre haut · Infanterie = cylindre · Montée = bloc massif ·
//          Distance = cône orienté · Spéciale = cylindre fin · Mythique = grand bloc.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDemoUnit : public AUnitBase
{
	GENERATED_BODY()

public:
	AWOTOLDemoUnit();

	// Hauteur réelle approximative en mètres (valeurs du GDD/doc), pour l'échelle
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	static float GetUnitHeightMeters(FName UnitID);

	// Si vrai : comportement de créature/boss autonome (cherche, avance, attaque)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	bool bCreatureBrain = false;

	// Multiplicateur de PV (1 = stats normales ; >1 pour un boss coriace)
	// À fixer AVANT BeginPlay (spawn différé).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Greybox")
	float HealthScale = 1.f;

	// PV max effectifs (en tenant compte de HealthScale)
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	int32 GetEffectiveMaxHealth() const;

	// % de vie calculé sur les PV EFFECTIFS (boss inclus) — pour la barre du HUD.
	// (GetHealthPercent() de base sature à 100% tant que PV > MaxHealth de base.)
	UFUNCTION(BlueprintPure, Category = "Demo|Greybox")
	float GetEffectiveHealthPercent() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UStaticMeshComponent> ShapeMesh;

	// Étiquette flottante : nom de l'unité + PV% (remplace une barre de vie UMG)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo|Greybox")
	TObjectPtr<UTextRenderComponent> NameTag;

	// Construit la forme greybox (mesh + échelle + couleur) selon rôle/faction/taille
	void BuildGreyboxShape();

private:
	// Matériau dynamique de la forme (pour changer la couleur à la sélection)
	UPROPERTY()
	TObjectPtr<class UMaterialInstanceDynamic> ShapeMID;

	// ─── Kitbash : pièces additionnelles formant la silhouette de l'unité ─────
	// Ajoute une primitive enfant (mesh + transform + couleur) à l'unité.
	UStaticMeshComponent* AddPart(const TCHAR* MeshPath, const FVector& RelLoc,
		const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color);
	// Configure le corps principal (ShapeMesh) comme une pièce kitbash.
	void SetupMainPart(const TCHAR* MeshPath, const FVector& RelLoc,
		const FVector& RelScale, const FRotator& RelRot, const FLinearColor& Color);
	// Assemble la silhouette selon l'unité (corps + tête + accessoires).
	void AssembleSilhouette(FName UnitID, EUnitRole UnitRole, float HeightU,
		const FLinearColor& Base, const FLinearColor& Accent);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> PartMIDs;
	TArray<FLinearColor> PartBaseColors;

	// Disque d'équipe sous les pieds (bleu/vert) — toujours visible, lisibilité RTS.
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TeamMarker;
	void AddTeamMarker(float Radius, float ZFeet, const FLinearColor& Color);

	// ─── Membres articulés + animation procédurale (preuve : Aquiloryons) ─────
	// Crée une articulation (pivot) enfant ; on la fait tourner pour animer.
	class USceneComponent* MakeJoint(USceneComponent* Parent, const FVector& RelLoc);
	// Crée un "os" (mesh) suspendu à une articulation (offset = pend depuis le pivot).
	UStaticMeshComponent* MakeBone(USceneComponent* Joint, const TCHAR* MeshPath,
		const FVector& Offset, const FVector& Scale, const FRotator& Rot, const FLinearColor& Color);
	// Assemble un Aquiloryons articulé (torse + 2 bras + 2 jambes + épée + bouclier).
	void BuildArticulatedAquiloryons(float HeightU, const FLinearColor& Armor, const FLinearColor& Energy);
	// Anime les articulations selon l'état (idle / marche / attaque / bouclier).
	void AnimateArticulated(float DeltaSeconds);

	UPROPERTY() TObjectPtr<USceneComponent> JRShoulder;
	UPROPERTY() TObjectPtr<USceneComponent> JRElbow;
	UPROPERTY() TObjectPtr<USceneComponent> JLShoulder;
	UPROPERTY() TObjectPtr<USceneComponent> JLElbow;
	UPROPERTY() TObjectPtr<USceneComponent> JRHip;
	UPROPERTY() TObjectPtr<USceneComponent> JLHip;

	bool  bArticulated = false;
	float AnimPhase    = 0.f;
	float SwingProgress = 0.f; // 0..1 avancement d'un coup d'épée

	// Réagit à la sélection joueur : surligne l'unité
	UFUNCTION()
	void HandleSelected(bool bSel);

	// Affiche un chiffre de dégâts flottant quand l'unité perd des PV
	UFUNCTION()
	void HandleHealthChanged(float NewHealth, float MaxHealth);

	// Cerveau autonome de créature/boss (cherche l'ennemi, avance, attaque)
	void CreatureBrainTick(float DeltaSeconds);

	float LastKnownHealth = -1.f;
	bool  bCreatureStyled = false;
};
