#include "AIAdaptiveController.h"
#include "Gameplay/Battle/TacticalPhaseManager.h"
#include "Core/FactionRegistrySubsystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UAIAdaptiveController::UAIAdaptiveController()
{
	bWantsPlayerState = false;
}

void UAIAdaptiveController::BeginPlay()
{
	Super::BeginPlay();
}

void UAIAdaptiveController::EndPlay(const EEndPlayReason::Type Reason)
{
	Super::EndPlay(Reason);
}

void UAIAdaptiveController::SetControlledFaction(EFactionID InFaction)
{
	ControlledFaction = InFaction;

	// S'abonner au TacticalPhaseManager via le GameMode
	// Le GameMode expose son PhaseManager en BlueprintReadOnly ; ici on accède via GameState
	// Le branchement concret est fait dans le Blueprint héritant de cette classe
}

void UAIAdaptiveController::UpdatePlayerProfile(const FPlayerBehaviorProfile& Profile)
{
	AdaptToPlayerProfile(Profile);
}

void UAIAdaptiveController::AdaptToPlayerProfile(const FPlayerBehaviorProfile& Profile)
{
	// Si le joueur est agressif → l'IA devient plus défensive pour compenser
	// Si le joueur est prudent → l'IA attaque davantage pour le forcer à bouger
	ComputedAggressionLevel = FMath::Clamp(1.f - Profile.AggressionScore * 0.6f, 0.2f, 1.f);
	ComputedCautionLevel    = FMath::Clamp(Profile.AggressionScore * 0.7f, 0.2f, 1.f);

	// Exposer au Blackboard pour que les Behavior Trees le lisent
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(TEXT("AggressionLevel"), ComputedAggressionLevel);
		BB->SetValueAsFloat(TEXT("CautionLevel"),    ComputedCautionLevel);
		BB->SetValueAsFloat(TEXT("PlayerVerticalUsage"), Profile.VerticalUsageRatio);
	}
}

void UAIAdaptiveController::OnTacticalWindowOpened(EFactionID Faction)
{
	if (Faction == ControlledFaction)
	{
		ActivateAI();
	}
}

void UAIAdaptiveController::OnTacticalWindowClosed(EFactionID Faction)
{
	if (Faction == ControlledFaction)
	{
		DeactivateAI();
	}
}

void UAIAdaptiveController::ActivateAI()
{
	bIsActive = true;
	// Le Behavior Tree reprend son exécution
	// Le Blueprint peut override BrainComponent->RestartLogic() ici
}

void UAIAdaptiveController::DeactivateAI()
{
	bIsActive = false;
	// On stoppe le Behavior Tree jusqu'à la prochaine fenêtre
}
