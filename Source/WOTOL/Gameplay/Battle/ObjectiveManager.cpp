#include "ObjectiveManager.h"
#include "Gameplay/Resources/ResourceManager.h"

void UObjectiveManager::RegisterObjective(const FBattleObjective& Objective)
{
	Objectives.Add(Objective);
}

void UObjectiveManager::CompleteObjective(int32 Index, EFactionID RewardFaction)
{
	if (!Objectives.IsValidIndex(Index)) return;
	if (Objectives[Index].bCompleted || Objectives[Index].bFailed) return;

	Objectives[Index].bCompleted = true;

	// Distribuer les récompenses
	if (UResourceManager* RM = GetWorld()->GetSubsystem<UResourceManager>())
	{
		for (const FObjectiveReward& Reward : Objectives[Index].Rewards)
		{
			RM->AddResource(RewardFaction, Reward.ResourceType, Reward.Amount);
		}
	}

	OnObjectiveCompleted.Broadcast(Objectives[Index]);
}

void UObjectiveManager::FailObjective(int32 Index)
{
	if (!Objectives.IsValidIndex(Index)) return;
	Objectives[Index].bFailed = true;
}

int32 UObjectiveManager::GetCompletedCount() const
{
	int32 Count = 0;
	for (const FBattleObjective& Obj : Objectives)
	{
		if (Obj.bCompleted) ++Count;
	}
	return Count;
}

void UObjectiveManager::TriggerNeutralZonePopup(const FBattleObjective& ZoneObjective)
{
	OnPopupRequested.Broadcast(ZoneObjective);
}
