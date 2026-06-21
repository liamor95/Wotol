#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/WOTOLTypes.h"
#include "UnitAIStateComponent.generated.h"

class AUnitBase;
class UAIAdaptiveController;

// Machine d'états C++ complète — remplace les Behavior Trees
// Liamor n'a pas besoin de créer un seul asset BT
UENUM(BlueprintType)
enum class EUnitAIState : uint8
{
	Idle,         // en attente (hors fenêtre tactique)
	Patrolling,   // déplacement aléatoire dans la zone
	Seeking,      // se déplace vers l'ennemi détecté
	Attacking,    // en portée — exécute une attaque
	Retreating,   // PV bas — fuit vers une position sûre
	ChangingLayer,// transition de palier vertical
	Dead
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnAIStateChanged, EUnitAIState, OldState, EUnitAIState, NewState);

UCLASS(ClassGroup = "WOTOL", meta = (BlueprintSpawnableComponent))
class WOTOL_API UUnitAIStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUnitAIStateComponent();

	// Activé/désactivé par le TacticalPhaseManager via l'AIAdaptiveController
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAIActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "AI")
	EUnitAIState GetCurrentState() const { return CurrentState; }

	// Config — peut être surchargée par unité dans le BP
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float SightRange          = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float RetreatHealthRatio  = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float TickInterval        = 0.25f; // évaluation toutes les 250ms

	UPROPERTY(BlueprintAssignable, Category = "AI")
	FOnAIStateChanged OnAIStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	void AITick();
	void TransitionTo(EUnitAIState NewState);

	// Logique par état
	void EvaluateIdle();
	void EvaluateSeeking();
	void EvaluateAttacking();
	void EvaluateRetreating();
	void EvaluatePatrolling();

	AUnitBase*             FindNearestEnemy() const;
	UAIAdaptiveController* GetAIController() const;
	bool                   HasLowHealth() const;
	bool                   IsInAttackRange(AUnitBase* Target) const;

	EUnitAIState CurrentState  = EUnitAIState::Idle;
	bool         bAIActive     = false;

	UPROPERTY()
	TWeakObjectPtr<AUnitBase> CurrentTarget;

	FTimerHandle AITickHandle;
	FVector      PatrolDestination;
	FVector      SpawnLocation;
};
