#include "TerritoryStateManager.h"

void UTerritoryStateManager::Initialize(UWorld* InWorld)
{
	WorldRef = InWorld;
}

void UTerritoryStateManager::TickCapture(EFactionID ControllingFaction, float DeltaSeconds)
{
	if (bCaptured || ControllingFaction == EFactionID::None) return;

	if (ControllingFaction != CurrentCapturingFaction)
	{
		// Nouvelle faction — on repart de zéro
		CaptureProgress         = 0.f;
		CurrentCapturingFaction = ControllingFaction;
	}

	CaptureProgress += CaptureRatePerSecond * DeltaSeconds;
	CaptureProgress  = FMath::Clamp(CaptureProgress, 0.f, TargetGrade.CaptureProgressRequired);

	OnCaptureProgressChanged.Broadcast(CurrentCapturingFaction, CaptureProgress);

	if (CaptureProgress >= TargetGrade.CaptureProgressRequired)
	{
		bCaptured = true;
		OnTerritoryCaptured.Broadcast(CurrentCapturingFaction, TargetGrade);
	}
}

void UTerritoryStateManager::ResetCapture()
{
	CaptureProgress         = 0.f;
	CurrentCapturingFaction = EFactionID::None;
	bCaptured               = false;
}

float UTerritoryStateManager::GetCaptureProgress(EFactionID Faction) const
{
	return (Faction == CurrentCapturingFaction) ? CaptureProgress : 0.f;
}
