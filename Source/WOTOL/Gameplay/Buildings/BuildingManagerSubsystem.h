#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/WOTOLTypes.h"
#include "BuildingDataAsset.h"
#include "BuildingManagerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBuildingConstructed,
	FName, BuildingID, EFactionID, Faction, UBuildingDataAsset*, Data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildingDestroyed,
	FName, BuildingID, EFactionID, Faction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBuildingResourceTick,
	FName, BuildingID, EResourceType, Resource, int32, Amount);

// État d'un bâtiment en jeu
USTRUCT(BlueprintType)
struct FBuildingInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FName BuildingID;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBuildingDataAsset> Data = nullptr;

	UPROPERTY(BlueprintReadOnly)
	EFactionID Owner = EFactionID::None;

	UPROPERTY(BlueprintReadOnly)
	float CurrentHealth = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bIsConstructed = false;

	UPROPERTY(BlueprintReadOnly)
	float ConstructionProgress = 0.f;   // 0–100

	// Timer interne pour production
	float ProductionAccumulator = 0.f;
};

// Gère tous les bâtiments de la bataille (construction, production, destruction)
// Scope démo : Aquiloris (10 bâtiments) uniquement
UCLASS()
class WOTOL_API UBuildingManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// ─── Construction ──────────────────────────────────────────────────────────

	// Vérifie si les prérequis sont remplis (ressources, bâtiments requis, grade territoire)
	UFUNCTION(BlueprintPure, Category = "Buildings")
	bool CanBuild(EFactionID Faction, UBuildingDataAsset* Data, FName ZoneID) const;

	// Démarre la construction ; déduit les ressources
	UFUNCTION(BlueprintCallable, Category = "Buildings")
	FName StartConstruction(EFactionID Faction, UBuildingDataAsset* Data, FName ZoneID);

	// Avance la construction de tous les bâtiments en cours
	UFUNCTION(BlueprintCallable, Category = "Buildings")
	void TickConstruction(float DeltaSeconds);

	// ─── Production ────────────────────────────────────────────────────────────

	// Tick de production des bâtiments construits ; distribue ressources via UResourceManager
	UFUNCTION(BlueprintCallable, Category = "Buildings")
	void TickProduction(float DeltaSeconds);

	// ─── Combat / dégâts ───────────────────────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "Buildings")
	void ApplyDamageToBuilding(FName BuildingID, float Damage);

	// ─── Lecture ───────────────────────────────────────────────────────────────

	// C++ interne uniquement — un pointeur de struct ne peut pas être exposé au Blueprint.
	// Pour le Blueprint, utiliser GetBuildingInfo() ci-dessous qui renvoie une copie.
	const FBuildingInstance* GetBuilding(FName BuildingID) const;

	// Version Blueprint : renvoie une copie + un booléen de validité
	UFUNCTION(BlueprintPure, Category = "Buildings")
	bool GetBuildingInfo(FName BuildingID, FBuildingInstance& OutBuilding) const;

	UFUNCTION(BlueprintPure, Category = "Buildings")
	TArray<FName> GetBuildingsForFaction(EFactionID Faction) const;

	UFUNCTION(BlueprintPure, Category = "Buildings")
	bool HasBuildingOfType(EFactionID Faction, UBuildingDataAsset* Data) const;

	UFUNCTION(BlueprintPure, Category = "Buildings")
	float GetConstructionProgress(FName BuildingID) const;

	// ─── Délégués ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Buildings")
	FOnBuildingConstructed OnBuildingConstructed;

	UPROPERTY(BlueprintAssignable, Category = "Buildings")
	FOnBuildingDestroyed OnBuildingDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Buildings")
	FOnBuildingResourceTick OnBuildingResourceTick;

private:
	TMap<FName, FBuildingInstance> Buildings;
	int32 NextBuildingIndex = 0;

	FName GenerateBuildingID(UBuildingDataAsset* Data);
	void CompleteConstruction(FName BuildingID);
	void DestroyBuilding(FName BuildingID);
};
