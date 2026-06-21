#include "PlayerProfileSubsystem.h"

void UPlayerProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPlayerProfileSubsystem::RecordAttack(bool bWasAggressive)
{
	const float Target = bWasAggressive ? 1.f : 0.f;
	Profile.AggressionScore = FMath::Lerp(Profile.AggressionScore, Target, SmoothingFactor);
	Profile.CautionScore    = 1.f - Profile.AggressionScore;
}

void UPlayerProfileSubsystem::RecordLayerChange(EVerticalLayer NewLayer)
{
	if (NewLayer != EVerticalLayer::Ground)
	{
		Profile.VerticalUsageRatio = FMath::Clamp(
			Profile.VerticalUsageRatio + 0.05f, 0.f, 1.f);
	}
}

void UPlayerProfileSubsystem::RecordBattleEnd(EBattleResult Result)
{
	Profile.BattlesPlayed++;
}

void UPlayerProfileSubsystem::ResetProfile()
{
	Profile = FPlayerBehaviorProfile{};
}
