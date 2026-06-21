#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/WOTOLTypes.h"
#include "UnitDataAsset.generated.h"

UENUM(BlueprintType)
enum class EUnitAttackType : uint8
{
	Melee,
	Ranged
};

UCLASS(BlueprintType)
class WOTOL_API UUnitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EFactionID Faction = EFactionID::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AttackDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AttackRange = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float AttackCooldown = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float MovementSpeed = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	EUnitAttackType AttackType = EUnitAttackType::Melee;

	// Classe de projectile — uniquement si AttackType == Ranged
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat",
		meta = (EditCondition = "AttackType == EUnitAttackType::Ranged"))
	TSoftClassPtr<class AWOTOLProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vertical")
	EVerticalLayer PreferredLayer = EVerticalLayer::Ground;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vertical")
	bool bCanChangeLayer = false;

	// Abilities par défaut de cette unité
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<class UAbilityBase>> DefaultAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn")
	TSoftClassPtr<class AUnitBase> UnitClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId("UnitData", GetFName());
	}
};
