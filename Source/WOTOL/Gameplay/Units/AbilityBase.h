#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AbilityBase.generated.h"

class AUnitBase;

UENUM(BlueprintType)
enum class EAbilityTargetType : uint8
{
	Self,
	SingleUnit,
	Location,
	AreaOfEffect
};

UCLASS(Abstract, Blueprintable, BlueprintType)
class WOTOL_API UAbilityBase : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	EAbilityTargetType TargetType = EAbilityTargetType::SingleUnit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float Cooldown = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float Range = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	float HealAmount = 0.f;

	// ---

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool CanActivate(AUnitBase* Caster) const;

	UFUNCTION(BlueprintCallable, Category = "Ability")
	bool Activate(AUnitBase* Caster, FVector TargetLocation, AUnitBase* TargetUnit);

	UFUNCTION(BlueprintPure, Category = "Ability")
	float GetCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Ability")
	bool IsOnCooldown() const { return GetCooldownRemaining() > 0.f; }

	// Override en Blueprint pour les effets visuels/son
	UFUNCTION(BlueprintImplementableEvent, Category = "Ability")
	void OnAbilityActivated(AUnitBase* Caster, FVector TargetLocation, AUnitBase* TargetUnit);

protected:
	// Logique C++ pure de l'ability — override dans les sous-classes
	virtual void ExecuteAbility(AUnitBase* Caster, FVector TargetLocation, AUnitBase* TargetUnit);

private:
	float LastActivationTime = -9999.f;

	UWorld* GetWorld() const override;
};
