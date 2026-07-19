#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLCaptureObject.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInstanceDynamic;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCaptureObjectHealthChanged,
	float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCaptureObjectDestroyed);

// ─────────────────────────────────────────────────────────────────────────────
// OBJET DE CAPTURE TERRITORIAL (section 12).
//   Aquiloris : Cristalliseur (bleu cristallin)   — forme cône/cristal
//   Noxéens   : Abyssalyseur  (vert organique)    — forme sphère/organique
// Fonction identique : revendique la zone (Grade 1), a des PV, peut être
// endommagé puis réparé. Valeurs PV = PrototypeDefault (doc : 600 max).
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLCaptureObject : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLCaptureObject();

	// Faction propriétaire (détermine nom + couleur + forme)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	EFactionID OwnerFaction = EFactionID::Aquiloris;

	// Identifiant de zone capturée (pour TerritoryStateManager)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	FName ZoneID = TEXT("NeutralZone_01");

	// PV de BÂTIMENT (bien plus résistant qu'une unité : un objectif stratégique).
	// Dimensionné pour tenir face au siège le temps que le joueur écarte les assaillants.
	// 16000 : sous siège total (24/s) il tombe en ~660 s > chrono 600 s -> défendable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float MaxHealth = 16000.f;

	UPROPERTY(BlueprintReadOnly, Category = "Capture")
	float CurrentHealth = 16000.f;

	// Nom affiché selon la faction (Cristalliseur / Abyssalyseur)
	UFUNCTION(BlueprintPure, Category = "Capture")
	FText GetDisplayName() const;

	// Revendique la zone -> Grade 1 + notifie la progression de la démo
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void ClaimZone();

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Capture")
	void Repair(float Amount);

	UFUNCTION(BlueprintPure, Category = "Capture")
	float GetHealthPercent() const;

	UPROPERTY(BlueprintAssignable, Category = "Capture")
	FOnCaptureObjectHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Capture")
	FOnCaptureObjectDestroyed OnCaptureDestroyed;

	// ── APERÇU HOLOGRAPHIQUE (placement) ─────────────────────────────────────────
	// Mode aperçu : instance visuelle SEULE (pas de PV, pas de ClaimZone, pas de collision),
	// reprend exactement la même forme que le vrai bâtiment mais en coquille translucide
	// pulsante (WOTOLGlow::MakeHalo), même langage visuel que le feedback des éléments
	// destructibles. À appeler avant BeginPlay (juste après SpawnActorDeferred).
	UFUNCTION(BlueprintCallable, Category = "Capture|Ghost")
	void SetGhostPreviewMode(bool bEnable) { bIsGhostPreview = bEnable; }

	UFUNCTION(BlueprintPure, Category = "Capture|Ghost")
	bool IsGhostPreview() const { return bIsGhostPreview; }

	// ── ANIMATION DE CONSTRUCTION ─────────────────────────────────────────────────
	// Démarre une montée en échelle (0 -> 1) sur Duration secondes après la pose réelle
	// du bâtiment : feedback visuel clair que la construction est en cours.
	UFUNCTION(BlueprintCallable, Category = "Capture")
	void BeginConstruction(float Duration);

	UFUNCTION(BlueprintPure, Category = "Capture")
	bool IsUnderConstruction() const { return bIsUnderConstruction; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void ApplyHologramMaterial();

	bool  bIsGhostPreview     = false;
	bool  bIsUnderConstruction = false;
	float ConstructionDuration = 2.5f;
	float ConstructionElapsed  = 0.f;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> HologramMID;

	// Racine NON mise à l'échelle : le mesh (agrandi) et les étiquettes s'y attachent
	// séparément -> le texte n'hérite pas de l'échelle (3,3,4) du mesh (sinon démesuré).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> ShapeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTextRenderComponent> NameTag;

	// Ombre noire derrière l'étiquette (contraste)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTextRenderComponent> NameTagShadow;

	void BuildVisual();
};
