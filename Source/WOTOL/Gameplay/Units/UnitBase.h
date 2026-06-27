#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/WOTOLTypes.h"
#include "UnitBase.generated.h"

class UVerticalLayerComponent;
class UAbilityComponent;
class UUnitMoraleComponent;
class UUnitDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied,      AUnitBase*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitSelected,  bool, bSelected);

UCLASS(Abstract)
class WOTOL_API AUnitBase : public ACharacter
{
	GENERATED_BODY()

public:
	AUnitBase();

	// ---- Data ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<UUnitDataAsset> UnitData;

	UFUNCTION(BlueprintPure, Category = "Unit")
	UUnitDataAsset* GetUnitData() const { return UnitData; }

	// ---- Composants ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vertical")
	TObjectPtr<UVerticalLayerComponent> VerticalLayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilityComponent> AbilityComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Morale")
	TObjectPtr<UUnitMoraleComponent> MoraleComp;

	// ---- État combat ----
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	UFUNCTION(BlueprintPure, Category = "Combat")
	EFactionID GetFaction() const { return Faction; }

	// Inflige des dégâts ; valeur négative = soin
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float TakeDamageFromUnit(float Damage, AUnitBase* Instigator);

	// Déclenche une attaque vers la cible (appelé par l'IA ou le joueur)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void PerformAttack(AUnitBase* Target);

	// ---- Sélection (joueur) ----
	UFUNCTION(BlueprintCallable, Category = "Selection")
	void SetSelected(bool bNewSelected);

	UFUNCTION(BlueprintPure, Category = "Selection")
	bool IsSelected() const { return bSelected; }

	// ---- Délégués ----
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnUnitDied OnUnitDied;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Selection")
	FOnUnitSelected OnUnitSelected;

	// Événement Blueprint pour l'animation d'attaque / FX
	UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
	void OnAttackPerformed(AUnitBase* Target);

	// Événement Blueprint pour l'indicateur de sélection (décal, cercle, etc.)
	UFUNCTION(BlueprintImplementableEvent, Category = "Selection")
	void OnSelectionChanged(bool bNewSelected);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	EFactionID Faction = EFactionID::None;

private:
	void InitFromDataAsset();
	void Die();
	void SpawnProjectileToward(AUnitBase* Target, float OverrideDamage);

	float LastAttackTime  = -9999.f;
	bool  bSelected       = false;
};
