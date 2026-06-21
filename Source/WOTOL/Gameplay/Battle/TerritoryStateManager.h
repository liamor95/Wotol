#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Data/WOTOLTypes.h"
#include "TerritoryStateManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCaptureProgressChanged, EFactionID, Faction, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerritoryCaptured, EFactionID, Faction, FTerritoryGrade, Grade);

UCLASS()
class WOTOL_API UTerritoryStateManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UWorld* InWorld);

	// Appelé par le tick de bataille pour avancer la capture
	UFUNCTION(BlueprintCallable, Category = "Territory")
	void TickCapture(EFactionID ControllingFaction, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Territory")
	void ResetCapture();

	UFUNCTION(BlueprintPure, Category = "Territory")
	float GetCaptureProgress(EFactionID Faction) const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	EFactionID GetCapturingFaction() const { return CurrentCapturingFaction; }

	UFUNCTION(BlueprintPure, Category = "Territory")
	bool IsCaptured() const { return bCaptured; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
	FTerritoryGrade TargetGrade;

	UPROPERTY(BlueprintAssignable)
	FOnCaptureProgressChanged OnCaptureProgressChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTerritoryCaptured OnTerritoryCaptured;

	// Vitesse de capture par seconde (unités de progression)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Territory")
	float CaptureRatePerSecond = 10.f;

private:
	TWeakObjectPtr<UWorld> WorldRef;

	EFactionID CurrentCapturingFaction = EFactionID::None;
	float      CaptureProgress         = 0.f;
	bool       bCaptured               = false;
};
