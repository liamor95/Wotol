#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HeroExperienceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeroLevelUp, int32, NewLevel, int32, OldLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeroXPChanged, int32, CurrentXP, int32, RequiredXP);

// Composant XP/niveau du héros — affiché en bas gauche de l'écran de bataille (ex: "20")
UCLASS(ClassGroup = "WOTOL", meta = (BlueprintSpawnableComponent))
class WOTOL_API UHeroExperienceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHeroExperienceComponent();

	UFUNCTION(BlueprintCallable, Category = "HeroXP")
	void AddXP(int32 Amount);

	UFUNCTION(BlueprintPure, Category = "HeroXP")
	int32 GetCurrentLevel() const { return CurrentLevel; }

	UFUNCTION(BlueprintPure, Category = "HeroXP")
	int32 GetCurrentXP() const { return CurrentXP; }

	UFUNCTION(BlueprintPure, Category = "HeroXP")
	int32 GetXPForNextLevel() const;

	// 0.0 à 1.0 pour la barre de progression XP
	UFUNCTION(BlueprintPure, Category = "HeroXP")
	float GetXPProgress() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeroXP", meta = (ClampMin = "1"))
	int32 StartingLevel = 1;

	// XP requis pour level 1→2. Chaque niveau = XPBase * Level^XPExponent
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeroXP")
	int32 XPBase = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HeroXP")
	float XPExponent = 1.5f;

	UPROPERTY(BlueprintAssignable, Category = "HeroXP")
	FOnHeroLevelUp OnLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "HeroXP")
	FOnHeroXPChanged OnXPChanged;

	virtual void BeginPlay() override;

private:
	int32 CurrentLevel = 1;
	int32 CurrentXP = 0;

	void CheckLevelUp();
};
