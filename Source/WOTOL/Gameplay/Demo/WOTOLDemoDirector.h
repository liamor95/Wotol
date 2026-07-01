#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLDemoDirector.generated.h"

class AWOTOLDemoUnit;
class AWOTOLCaptureObject;
class UUnitDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDemoMessage, const FString&, Message);

// ─────────────────────────────────────────────────────────────────────────────
// DIRECTOR DE DÉMO — orchestre une boucle greybox AUTO-JOUABLE en un seul niveau.
//
//   1) Bataille CRÉATURE  : armée joueur (chef + infanterie + montée) vs créature
//   2) Victoire           : débloque la distance, découvre le mythique,
//                           pose l'objet de capture (Cristalliseur/Abyssalyseur),
//                           "Zone capturée — Grade 1"
//   3) Bataille RIVALE     : armée joueur (AVEC distance) vs escouade rivale
//   4) Victoire           : objet de capture endommagé puis réparé
//   5) Fin de démo
//
// Tout s'enchaîne tout seul (idéal projection). Réutilise RTSBattleManager,
// FactionRegistrySubsystem, UnitDataRegistrySubsystem, WOTOLDemoUnit. Additif.
//
// UTILISATION : poser ce Director (ou le GameMode WOTOLGameMode_Demo) dans un
// niveau avec sol + NavMeshBoundsVolume, puis Play.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLDemoDirector();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	EFactionID DefaultPlayerFaction = EFactionID::Aquiloris;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 InfantryCount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 MountedCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo|Roster")
	int32 RangedCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float UnitSpacing = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float ArmySeparation = 4500.f;

	// Multiplicateur de PV de la créature/boss (boss coriace)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float CreatureHealthScale = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float BattleStartDelay = 1.5f;

	// Délai entre deux phases (lecture des messages à l'écran)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	float PhaseTransitionDelay = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TSubclassOf<AWOTOLDemoUnit> DemoUnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Demo")
	TSubclassOf<AWOTOLCaptureObject> CaptureObjectClass;

	// Messages narratifs (le HUD/BP peut s'y abonner pour les afficher à l'écran)
	UPROPERTY(BlueprintAssignable, Category = "Demo")
	FOnDemoMessage OnDemoMessage;

	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartCurrentBattle();

	// Flux d'écrans : monte les armées en PRÉPARATION (placement, sans combat).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void BeginPreparation();

	// Lance réellement la bataille depuis la préparation (active l'IA + le boss).
	UFUNCTION(BlueprintCallable, Category = "Demo")
	void StartBattleNow();

protected:
	virtual void BeginPlay() override;

private:
	EFactionID ResolvePlayerFaction() const;
	EFactionID RivalOf(EFactionID Faction) const;
	int32 CountAlive(EFactionID Faction) const;

	void SpawnPlayerArmy(EFactionID Faction, const FVector& Origin, const FRotator& Facing);
	void SpawnEnemyForCreature(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing);
	void SpawnRivalSquad(EFactionID RivalFaction, const FVector& Origin, const FRotator& Facing);
	AWOTOLDemoUnit* SpawnUnit(FName UnitID, const FVector& Loc, const FRotator& Facing,
		float ScaleBoost, float HealthScale = 2.2f); // armées plus résistantes = bataille plus longue

	void LaunchBattle();
	void CheckBattleEnd();
	void OnPlayerVictory();
	void OnPlayerDefeat();
	void CleanupUnits();
	void SpawnCaptureObject(EFactionID Faction);
	void StartRivalDefense();
	void EndDemo(bool bPlayerWon);
	void Say(const FString& Message);
	void FocusCameraOnPlayer(); // recadre la caméra derrière l'armée, vers l'ennemi

	EFactionID CachedPlayerFaction = EFactionID::None;
	EFactionID CachedRivalFaction  = EFactionID::None;

	UPROPERTY()
	TArray<TObjectPtr<AWOTOLDemoUnit>> SpawnedUnits;

	UPROPERTY()
	TObjectPtr<AWOTOLCaptureObject> CaptureObject;

	FTimerHandle BattleStartHandle;
	FTimerHandle BattleCheckHandle;
	FTimerHandle PhaseHandle;
	bool bBattleConcluded = false;
};
