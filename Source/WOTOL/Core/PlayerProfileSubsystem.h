#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "PlayerProfileSubsystem.generated.h"

UCLASS()
class WOTOL_API UPlayerProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Profile")
	const FPlayerBehaviorProfile& GetProfile() const { return Profile; }

	// Appelé à chaque action significative en bataille
	UFUNCTION(BlueprintCallable, Category = "Profile")
	void RecordAttack(bool bWasAggressive);

	UFUNCTION(BlueprintCallable, Category = "Profile")
	void RecordLayerChange(EVerticalLayer NewLayer);

	UFUNCTION(BlueprintCallable, Category = "Profile")
	void RecordBattleEnd(EBattleResult Result);

	// Réinitialise le profil (début de nouvelle session)
	UFUNCTION(BlueprintCallable, Category = "Profile")
	void ResetProfile();

private:
	FPlayerBehaviorProfile Profile;

	// Lissage exponentiel pour éviter les pics transitoires
	static constexpr float SmoothingFactor = 0.1f;
};
