#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UnitMoraleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoraleChanged, float, NewMorale);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitRouting);      // moral à 0 → déroute
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnitMoraleRestored); // retour à l'ordre

// Jauge de moral d'une unité — DISTINCTE des PV
// À 0 → état ROUTING, unité incontrôlable jusqu'à intervention du chef
UCLASS(ClassGroup = "WOTOL", meta = (BlueprintSpawnableComponent))
class WOTOL_API UUnitMoraleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnitMoraleComponent();

	virtual void BeginPlay() override;

	// Réduit le moral (dégâts psychologiques)
	UFUNCTION(BlueprintCallable, Category = "Morale")
	void ApplyMoraleHit(float Amount);

	// Restaure du moral (présence commandant, compétences de soutien)
	UFUNCTION(BlueprintCallable, Category = "Morale")
	void RestoreMorale(float Amount);

	UFUNCTION(BlueprintPure, Category = "Morale")
	float GetMorale() const { return CurrentMorale; }

	UFUNCTION(BlueprintPure, Category = "Morale")
	float GetMoralePercent() const;

	UFUNCTION(BlueprintPure, Category = "Morale")
	bool IsRouting() const { return bIsRouting; }

	// Mort du commandant : malus massif
	UFUNCTION(BlueprintCallable, Category = "Morale")
	void OnCommanderDied();

	// Mort d'une unité alliée à portée
	UFUNCTION(BlueprintCallable, Category = "Morale")
	void OnNearbyAllyDied();

	// Aura passive d'un commandant proche
	UFUNCTION(BlueprintCallable, Category = "Morale")
	void SetCommanderAura(bool bActive, float AuraBonus = 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morale", meta = (ClampMin = "0", ClampMax = "100"))
	float StartingMorale = 80.f;

	// Malus quand une unité alliée meurt à portée
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morale")
	float AllyDeathMoralePenalty = 5.f;

	// Malus massif quand le commandant meurt
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Morale")
	float CommanderDeathPenalty = 40.f;

	UPROPERTY(BlueprintAssignable, Category = "Morale")
	FOnMoraleChanged OnMoraleChanged;

	UPROPERTY(BlueprintAssignable, Category = "Morale")
	FOnUnitRouting OnUnitRouting;

	UPROPERTY(BlueprintAssignable, Category = "Morale")
	FOnUnitMoraleRestored OnMoraleRestored;

private:
	float CurrentMorale = 80.f;
	bool bIsRouting = false;
	float ActiveCommanderAuraBonus = 0.f;

	void CheckRoutingState();
};
