#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Data/WOTOLTypes.h"
#include "UnitBase.generated.h"

class UVerticalLayerComponent;
class UUnitDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitDied, AUnitBase*, Unit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);

UCLASS(Abstract)
class WOTOL_API AUnitBase : public ACharacter
{
	GENERATED_BODY()

public:
	AUnitBase();

	// ---- Données ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	TObjectPtr<UUnitDataAsset> UnitData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vertical")
	TObjectPtr<UVerticalLayerComponent> VerticalLayer;

	// ---- État ----
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	EFactionID GetFaction() const { return Faction; }

	// Inflige des dégâts, retourne les dégâts réels appliqués
	UFUNCTION(BlueprintCallable, Category = "Combat")
	float TakeDamageFromUnit(float Damage, AUnitBase* Instigator);

	// ---- Délégués ----
	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnUnitDied OnUnitDied;

	UPROPERTY(BlueprintAssignable, Category = "Combat")
	FOnHealthChanged OnHealthChanged;

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
};
