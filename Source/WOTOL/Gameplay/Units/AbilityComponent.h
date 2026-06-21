#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityBase.h"
#include "AbilityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityActivated, UAbilityBase*, Ability);

UCLASS(ClassGroup = "WOTOL", meta = (BlueprintSpawnableComponent))
class WOTOL_API UAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityComponent();

	// Classes d'abilities à instancier au BeginPlay
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UAbilityBase>> AbilityClasses;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool ActivateAbilityByIndex(int32 Index, FVector TargetLocation, AUnitBase* TargetUnit);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool ActivateAbilityByClass(TSubclassOf<UAbilityBase> AbilityClass,
		FVector TargetLocation, AUnitBase* TargetUnit);

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UAbilityBase* GetAbilityByIndex(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Abilities")
	const TArray<UAbilityBase*>& GetAllAbilities() const { return Abilities; }

	UPROPERTY(BlueprintAssignable)
	FOnAbilityActivated OnAbilityActivated;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<TObjectPtr<UAbilityBase>> Abilities;
};
