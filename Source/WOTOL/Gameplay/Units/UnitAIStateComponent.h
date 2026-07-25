#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/WOTOLTypes.h"
#include "UnitAIStateComponent.generated.h"

class AUnitBase;
class AAIAdaptiveController;

// Machine d'états C++ pour un RTS temps réel — inspire Total War / Bannerlord
// PAS de tours. PAS de fenêtres tactiques. L'IA tourne en continu à 250ms.
UENUM(BlueprintType)
enum class EUnitAIState : uint8
{
	Idle,          // en attente — aucun ennemi détecté
	Patrolling,    // déplacement aléatoire dans la zone (sans ennemi)
	Seeking,       // se déplace vers l'ennemi détecté
	Attacking,     // en portée — exécute des attaques
	Retreating,    // PV bas — fuit vers une position sûre
	ChangingLayer, // transition de palier vertical en cours
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

	// Activé/désactivé par URTSBattleManager (plus par TacticalPhaseManager)
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetAIActive(bool bActive);

	UFUNCTION(BlueprintPure, Category = "AI")
	EUnitAIState GetCurrentState() const { return CurrentState; }

	// Transition publique — appelée par IssueOrder_Retreat dans AIAdaptiveController
	UFUNCTION(BlueprintCallable, Category = "AI")
	void TransitionTo(EUnitAIState NewState);

	// ─── Config (surchargeable par unité dans le BP) ──────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float SightRange = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float RetreatHealthRatio = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Config")
	float TickInterval = 0.25f;  // évaluation toutes les 250ms

	// ─── État d'ordre RTS (écrit par AIAdaptiveController) ───────────────────

	// Cible imposée par le joueur (IssueOrder_AttackTarget)
	UPROPERTY()
	TWeakObjectPtr<AUnitBase> ForceTarget;

	// Destination pour AttackMove
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	FVector AttackMoveDestination;

	// Vrai si un AttackMove est en cours
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	bool bAttackMoveActive = false;

	// Vrai si l'unité tient position (HoldPosition — attaque à portée, pas de poursuite)
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	bool bHoldPosition = false;

	// Vrai pendant qu'un ordre joueur de déplacement simple est en cours
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	bool bFollowingPlayerOrder = false;

	UPROPERTY(BlueprintAssignable, Category = "AI")
	FOnAIStateChanged OnAIStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	// Réagit à la mort de l'unité possédée (lié à AUnitBase::OnUnitDied, délégué dynamique)
	UFUNCTION()
	void HandleOwnerDied(AUnitBase* Unit);

	void AITick();

	// Logique par état
	void EvaluateIdle();
	void EvaluateSeeking();
	void EvaluateAttacking();
	void EvaluateRetreating();
	void EvaluatePatrolling();

	AUnitBase*             FindNearestEnemy() const;
	AUnitBase*             FindBestTarget() const;  // tient compte de ForceTarget
	AAIAdaptiveController* GetAIController() const;
	bool                   HasLowHealth() const;
	bool                   IsInAttackRange(AUnitBase* Target) const;

	EUnitAIState CurrentState = EUnitAIState::Idle;
	bool         bAIActive    = false;

	UPROPERTY()
	TWeakObjectPtr<AUnitBase> CurrentTarget;

	FTimerHandle AITickHandle;
	FVector      PatrolDestination;
	FVector      SpawnLocation;
};
