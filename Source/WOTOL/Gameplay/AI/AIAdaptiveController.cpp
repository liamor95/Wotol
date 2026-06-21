#include "AIAdaptiveController.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitAIStateComponent.h"
#include "Gameplay/Battle/TacticalPhaseManager.h"

UAIAdaptiveController::UAIAdaptiveController()
{
	bWantsPlayerState = false;
}

void UAIAdaptiveController::BeginPlay()
{
	Super::BeginPlay();
}

void UAIAdaptiveController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AUnitBase* Unit = Cast<AUnitBase>(InPawn))
	{
		ControlledFaction = Unit->GetFaction();
	}
}

void UAIAdaptiveController::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
}

void UAIAdaptiveController::SetControlledFaction(EFactionID InFaction)
{
	ControlledFaction = InFaction;
}

void UAIAdaptiveController::UpdatePlayerProfile(const FPlayerBehaviorProfile& Profile)
{
	AdaptToPlayerProfile(Profile);
}

void UAIAdaptiveController::IssueMoveCommand(FVector TargetLocation)
{
	// Interrompt l'IA autonome momentanément — l'unité obéit au joueur
	MoveToLocation(TargetLocation, 50.f);

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		// Remettre en Idle après déplacement ; l'IA reprend dès l'arrivée
		// (évaluation automatique au prochain AITick)
	}
}

void UAIAdaptiveController::IssueAttackCommand(AUnitBase* TargetUnit)
{
	if (!TargetUnit || !TargetUnit->IsAlive()) return;
	MoveToActor(TargetUnit, 50.f);
}

void UAIAdaptiveController::AdaptToPlayerProfile(const FPlayerBehaviorProfile& Profile)
{
	ComputedAggressionLevel = FMath::Clamp(1.f - Profile.AggressionScore * 0.6f, 0.2f, 1.f);
	ComputedCautionLevel    = FMath::Clamp(Profile.AggressionScore * 0.7f, 0.2f, 1.f);

	// Propager au composant d'état pour ajuster les seuils de retraite / agressivité
	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		// Plus le joueur est agressif, plus l'IA recule tard (seuil de retraite plus bas)
		State->RetreatHealthRatio =
			FMath::Lerp(0.35f, 0.15f, ComputedCautionLevel);
	}
}

void UAIAdaptiveController::OnTacticalWindowOpened(EFactionID Faction)
{
	if (Faction == ControlledFaction) SetAIStateActive(true);
}

void UAIAdaptiveController::OnTacticalWindowClosed(EFactionID Faction)
{
	if (Faction == ControlledFaction) SetAIStateActive(false);
}

void UAIAdaptiveController::SetAIStateActive(bool bActive)
{
	bIsActive = bActive;
	if (UUnitAIStateComponent* State = GetStateComponent())
	{
		State->SetAIActive(bActive);
	}
}

UUnitAIStateComponent* UAIAdaptiveController::GetStateComponent() const
{
	if (APawn* P = GetPawn())
	{
		return P->FindComponentByClass<UUnitAIStateComponent>();
	}
	return nullptr;
}
