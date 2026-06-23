#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/WOTOLTypes.h"
#include "VerticalLayerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnLayerChanged, EVerticalLayer, OldLayer, EVerticalLayer, NewLayer);

// Composant sur chaque unité — palier vertical actuel
// Consommé par TerritoryStateManager, AIAdaptiveController, HUD
UCLASS(ClassGroup = "WOTOL", meta = (BlueprintSpawnableComponent))
class WOTOL_API UVerticalLayerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UVerticalLayerComponent();

	UFUNCTION(BlueprintCallable, Category = "Vertical")
	EVerticalLayer GetCurrentLayer() const { return CurrentLayer; }

	UFUNCTION(BlueprintCallable, Category = "Vertical")
	void SetLayer(EVerticalLayer NewLayer);

	// Hauteur cible en Z selon le palier (valeur négative = profondeur)
	UFUNCTION(BlueprintPure, Category = "Vertical")
	static float GetLayerTargetZ(EVerticalLayer Layer);

	// Multiplicateur de dégâts selon la direction d'attaque verticale
	// Attaque ascendante depuis Hadal = ×3, depuis Bathypélagique = ×1.75, etc.
	UFUNCTION(BlueprintPure, Category = "Vertical")
	static float GetAttackDamageMultiplier(EVerticalLayer AttackerLayer, EVerticalLayer TargetLayer);

	// Vitesse +20% en Épipélagique
	UFUNCTION(BlueprintPure, Category = "Vertical")
	static float GetSpeedMultiplierForLayer(EVerticalLayer Layer);

	UPROPERTY(BlueprintAssignable, Category = "Vertical")
	FOnLayerChanged OnLayerChanged;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vertical")
	EVerticalLayer DefaultLayer = EVerticalLayer::Epipelagique;

protected:
	virtual void BeginPlay() override;

private:
	EVerticalLayer CurrentLayer;
};
