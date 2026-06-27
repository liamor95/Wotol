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

	// S'abonner au gestionnaire de tours — un seul timer central pour toutes les unités
	if (UTacticalPhaseManager* PhaseManager =
			GetWorld()->GetSubsystem<UTacticalPhaseManager>())
	{
		PhaseManager->OnTacticalWindowOpened.AddDynamic(
			this, &UAIAdaptiveController::OnTacticalWindowOpened);
		PhaseManager->OnTacticalWindowClosed.AddDynamic(
			this, &UAIAdaptiveController::OnTacticalWindowClosed);
	}
}

void UAIAdaptiveController::EndPlay(const EEndPlayReason::Type Reason)
{
	// Désabonnement propre
	if (UTacticalPhaseManager* PhaseManager =
			GetWorld()->GetSubsystem<UTacticalPhaseManager>())
	{
		PhaseManager->OnTacticalWindowOpened.RemoveDynamic(
			this, &UAIAdaptiveController::OnTacticalWindowOpened);
		PhaseManager->OnTacticalWindowClosed.RemoveDynamic(
			this, &UAIAdaptiveController::OnTacticalWindowClosed);
	}

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
	MoveToLocation(TargetLocation, 50.f);
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

	if (UUnitAIStateComponent* State = GetStateComponent())
	{
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
