#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLRewardActor.generated.h"

class USceneComponent;
class UStaticMeshComponent;

// Type de récompense de la séquence post-créature (phases 7-8).
UENUM(BlueprintType)
enum class EWOTOLRewardType : uint8
{
	HeartShard    UMETA(DisplayName = "Cœur-Éclat"),      // apparaît après la pose du Cristalliseur
	LeviaphenixEgg UMETA(DisplayName = "Œuf de Léviaphénix") // récompense finale de la conquête
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardCollected, EWOTOLRewardType, Type);

// ─────────────────────────────────────────────────────────────────────────────
// RÉCOMPENSE GREYBOX flottante et lumineuse (Cœur-Éclat bleu cristallin / Œuf de
// Léviaphénix). Tourne + flotte pour attirer l'œil. Se récupère par PROXIMITÉ du
// héros joueur (ou sur appel de Collect() depuis le flux d'objectif). À la collecte,
// diffuse OnRewardCollected -> le Director enchaîne la phase suivante.
// ─────────────────────────────────────────────────────────────────────────────
UCLASS()
class WOTOL_API AWOTOLRewardActor : public AActor
{
	GENERATED_BODY()

public:
	AWOTOLRewardActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	EWOTOLRewardType RewardType = EWOTOLRewardType::HeartShard;

	// Rayon (uu) de récupération automatique quand le héros s'approche (~3 m).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	float CollectRadius = 300.f;

	// Si vrai, la récupération de proximité est active (sinon récupération scriptée seulement).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	bool bAutoCollectByProximity = true;

	UPROPERTY(BlueprintAssignable, Category = "Reward")
	FOnRewardCollected OnRewardCollected;

	// Récupère la récompense (diffuse le delegate puis se détruit). Idempotent.
	UFUNCTION(BlueprintCallable, Category = "Reward")
	void Collect();

	UFUNCTION(BlueprintPure, Category = "Reward")
	bool IsCollected() const { return bCollected; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USceneComponent> Spinner; // pivote/flotte : les pièces s'y attachent

	void BuildVisual();

private:
	bool  bCollected = false;
	float BobPhase   = 0.f;
	float BaseZ      = 0.f;
};
