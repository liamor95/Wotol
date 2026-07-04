#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLCaptureObject.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

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

	// PV de BÂTIMENT (bien plus résistant qu'une unité : un objectif stratégique)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture")
	float MaxHealth = 3500.f;

	UPROPERTY(BlueprintReadOnly, Category = "Capture")
	float CurrentHealth = 3500.f;

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

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> ShapeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTextRenderComponent> NameTag;

	// Ombre noire derrière l'étiquette (contraste)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UTextRenderComponent> NameTagShadow;

	void BuildVisual();
};
