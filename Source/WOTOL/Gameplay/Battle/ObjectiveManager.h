#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "ObjectiveManager.generated.h"

USTRUCT(BlueprintType)
struct FBattleObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FObjectiveReward> Rewards;

	UPROPERTY(BlueprintReadOnly)
	bool bCompleted = false;

	UPROPERTY(BlueprintReadOnly)
	bool bFailed = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectiveCompleted, const FBattleObjective&, Objective);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectivePopupRequested, const FBattleObjective&, Objective);

// Gère les objectifs de bataille et le popup "NOUVELLE ZONE NEUTRE"
UCLASS()
class WOTOL_API UObjectiveManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void RegisterObjective(const FBattleObjective& Objective);

	// Marque un objectif comme complété et distribue les récompenses à la faction
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void CompleteObjective(int32 Index, EFactionID RewardFaction);

	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void FailObjective(int32 Index);

	UFUNCTION(BlueprintPure, Category = "Objectives")
	const TArray<FBattleObjective>& GetObjectives() const { return Objectives; }

	UFUNCTION(BlueprintPure, Category = "Objectives")
	int32 GetCompletedCount() const;

	// Déclenche le popup de nouvelle zone neutre
	UFUNCTION(BlueprintCallable, Category = "Objectives")
	void TriggerNeutralZonePopup(const FBattleObjective& ZoneObjective);

	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnObjectiveCompleted OnObjectiveCompleted;

	// Blueprint UMG s'abonne à ça pour afficher le popup
	UPROPERTY(BlueprintAssignable, Category = "Objectives")
	FOnObjectivePopupRequested OnPopupRequested;

private:
	TArray<FBattleObjective> Objectives;
};
