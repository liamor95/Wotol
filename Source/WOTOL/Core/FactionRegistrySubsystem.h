#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "FactionRegistrySubsystem.generated.h"

class AUnitBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitRegistered,   AUnitBase*, Unit, EFactionID, Faction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUnitUnregistered, AUnitBase*, Unit, EFactionID, Faction);

// Remplace tout GetAllActorsOfClass — liste vivante des unités actives par faction
UCLASS()
class WOTOL_API UFactionRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Appelé depuis AUnitBase::BeginPlay
	UFUNCTION(BlueprintCallable, Category = "Registry")
	void RegisterUnit(AUnitBase* Unit, EFactionID Faction);

	// Appelé depuis AUnitBase::EndPlay
	UFUNCTION(BlueprintCallable, Category = "Registry")
	void UnregisterUnit(AUnitBase* Unit, EFactionID Faction);

	UFUNCTION(BlueprintCallable, Category = "Registry")
	TArray<AUnitBase*> GetUnitsForFaction(EFactionID Faction) const;

	UFUNCTION(BlueprintCallable, Category = "Registry")
	int32 GetUnitCountForFaction(EFactionID Faction) const;

	UFUNCTION(BlueprintCallable, Category = "Registry")
	bool IsFactionEliminated(EFactionID Faction) const;

	UPROPERTY(BlueprintAssignable, Category = "Registry")
	FOnUnitRegistered OnUnitRegistered;

	UPROPERTY(BlueprintAssignable, Category = "Registry")
	FOnUnitUnregistered OnUnitUnregistered;

private:
	TMap<EFactionID, TArray<TWeakObjectPtr<AUnitBase>>> Registry;
};
