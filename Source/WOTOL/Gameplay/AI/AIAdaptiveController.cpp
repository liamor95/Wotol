#include "AIAdaptiveController.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/Units/VerticalLayerComponent.h"
#include "Gameplay/Units/AbilityComponent.h"

AAIAdaptiveController::AAIAdaptiveController()
{
	bWantsPlayerState = false;
}

void AAIAdaptiveController::BeginPlay()
{
	Super::BeginPlay();
}

void AAIAdaptiveController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AUnitBase* Unit = Cast<AUnitBase>(InPawn))
	{
		ControlledFaction = Unit->GetFaction();

		// L'IA démarre inactive — activée par URTSBattleManager::StartBattlePhase()
		// Les unités du JOUEUR sont contrôlées par AWOTOLPlayerController_Battle
		// Les unités ENNEMIES reçoivent ActivateRTSBehavior() au début du combat
	}
}

void AAIAdaptiveController::EndPlay(const EEndPlayReason::Type Reason)
{
	DeactivateRTSBehavior();
	Super::EndPlay(Reason);
}

// ─── Activation RTS ───────────────────────────────────────────────────────────

void AAIAdaptiveController::ActivateRTSBehavior()
{
	bAIActive    = true;
	CurrentOrder = ERTSOrder::AttackMove; // comportement par défaut : chercher et combattre
	SetAIStateActive(true);
}

void AAIAdaptiveController::DeactivateRTSBehavior()
{
	bAIActive    = false;
	CurrentOrder = ERTSOrder::None;
	SetAIStateActive(false);
	StopMovement();
}

// ─── Ordres RTS ───────────────────────────────────────────────────────────────

void AAIAdaptiveController::IssueOrder_Move(FVector TargetLocation)
{
	CurrentOrder = ERTSOrder::Move;

	// Passer la state machine en Idle pendant le déplacement ordonné
	// (elle reprendra le comportement auto à l'arrivée)
	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->bFollowingPlayerOrder = true;
		State->bHoldPosition         = false;   // un ordre de déplacement annule le "tenir position"
		State->ForceTarget           = nullptr;
	}

	MoveToLocation(TargetLocation, 50.f, true, /*bUsePathfinding=*/false);
}

void AAIAdaptiveController::IssueOrder_AttackMove(FVector TargetLocation)
{
	CurrentOrder = ERTSOrder::AttackMove;

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->bFollowingPlayerOrder = false; // IA reprend en cours de route si ennemi visible
		State->AttackMoveDestination = TargetLocation;
		State->bAttackMoveActive     = true;
	}

	MoveToLocation(TargetLocation, 50.f, true, /*bUsePathfinding=*/false);
}

void AAIAdaptiveController::IssueOrder_AttackTarget(AUnitBase* Target)
{
	if (!Target || !Target->IsAlive()) return;

	CurrentOrder = ERTSOrder::AttackTarget;

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->bFollowingPlayerOrder = false;
		State->bHoldPosition         = false;   // un ordre d'attaque annule le "tenir position"
		State->ForceTarget           = Target;
		State->bAttackMoveActive     = false;
	}

	MoveToActor(Target, 50.f, true, /*bUsePathfinding=*/false);
}

void AAIAdaptiveController::IssueOrder_HoldPosition()
{
	CurrentOrder = ERTSOrder::HoldPosition;
	StopMovement();

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->bHoldPosition         = true;
		State->bFollowingPlayerOrder = false;
		State->bAttackMoveActive     = false;
	}
}

void AAIAdaptiveController::IssueOrder_UseAbility(
	int32 AbilityIndex, FVector TargetLocation, AUnitBase* TargetUnit)
{
	CurrentOrder = ERTSOrder::UseAbility;

	AUnitBase* Unit = Cast<AUnitBase>(GetPawn());
	if (!Unit) return;

	if (UAbilityComponent* AbilComp = Unit->AbilityComp)
	{
		AbilComp->ActivateAbilityByIndex(AbilityIndex, TargetLocation, TargetUnit);
	}
}

void AAIAdaptiveController::IssueOrder_ChangeLayer(EVerticalLayer NewLayer)
{
	CurrentOrder = ERTSOrder::ChangeLayer;

	AUnitBase* Unit = Cast<AUnitBase>(GetPawn());
	if (!Unit || !Unit->VerticalLayer) return;

	if (!Unit->GetUnitData() || !Unit->GetUnitData()->Stats.bCanChangeLayer) return;

	Unit->VerticalLayer->SetLayer(NewLayer);

	// Déplacer physiquement l'unité vers la Z cible
	const float TargetZ = UVerticalLayerComponent::GetLayerTargetZ(NewLayer);
	FVector NewLoc = Unit->GetActorLocation();
	NewLoc.Z = TargetZ;
	Unit->SetActorLocation(NewLoc, true);

	// Rétablir l'ordre précédent après changement de couche
	CurrentOrder = ERTSOrder::AttackMove;
	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->bFollowingPlayerOrder = false;
	}
}

void AAIAdaptiveController::IssueOrder_Retreat()
{
	CurrentOrder = ERTSOrder::Retreat;

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->bHoldPosition         = false;
		State->bFollowingPlayerOrder = true;
		State->ForceTarget           = nullptr;
		State->bAttackMoveActive     = false;
	}

	// La state machine gère la retraite vers SpawnLocation dans EvaluateRetreating()
	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->TransitionTo(EUnitAIState::Retreating);
	}
}

// ─── Adaptation comportementale ───────────────────────────────────────────────

void AAIAdaptiveController::SetControlledFaction(EFactionID InFaction)
{
	ControlledFaction = InFaction;
}

void AAIAdaptiveController::UpdatePlayerProfile(const FPlayerBehaviorProfile& Profile)
{
	AdaptToPlayerProfile(Profile);
}

void AAIAdaptiveController::AdaptToPlayerProfile(const FPlayerBehaviorProfile& Profile)
{
	ComputedAggressionLevel = FMath::Clamp(1.f - Profile.AggressionScore * 0.6f, 0.2f, 1.f);
	ComputedCautionLevel    = FMath::Clamp(Profile.AggressionScore * 0.7f, 0.2f, 1.f);

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		// Joueur agressif → IA recule plus tard (seuil retraite réduit)
		State->RetreatHealthRatio = FMath::Lerp(0.35f, 0.15f, ComputedCautionLevel);

		// Joueur prudent → IA plus agressive (SightRange augmentée)
		State->SightRange = FMath::Lerp(1200.f, 2000.f, 1.f - ComputedCautionLevel);
	}
}

void AAIAdaptiveController::SetAIStateActive(bool bActive)
{
	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->SetAIActive(bActive);
	}
}

UUnitAIStateComponent* AAIAdaptiveController::GetStateComponent() const
{
	if (APawn* P = GetPawn())
	{
		return P->FindComponentByClass<UUnitAIStateComponent>();
	}
	return nullptr;
}
