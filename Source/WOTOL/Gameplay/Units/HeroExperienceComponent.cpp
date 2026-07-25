#include "HeroExperienceComponent.h"

UHeroExperienceComponent::UHeroExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHeroExperienceComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentLevel = FMath::Max(1, StartingLevel);
	CurrentXP = 0;
}

void UHeroExperienceComponent::AddXP(int32 Amount)
{
	if (Amount <= 0) return;

	CurrentXP += Amount;
	OnXPChanged.Broadcast(CurrentXP, GetXPForNextLevel());
	CheckLevelUp();
}

int32 UHeroExperienceComponent::GetXPForNextLevel() const
{
	return FMath::RoundToInt(XPBase * FMath::Pow(static_cast<float>(CurrentLevel), XPExponent));
}

float UHeroExperienceComponent::GetXPProgress() const
{
	const int32 Required = GetXPForNextLevel();
	if (Required <= 0) return 1.f;
	return FMath::Clamp(static_cast<float>(CurrentXP) / static_cast<float>(Required), 0.f, 1.f);
}

void UHeroExperienceComponent::CheckLevelUp()
{
	int32 Required = GetXPForNextLevel();
	while (CurrentXP >= Required)
	{
		CurrentXP -= Required;
		const int32 OldLevel = CurrentLevel;
		++CurrentLevel;
		OnLevelUp.Broadcast(CurrentLevel, OldLevel);
		Required = GetXPForNextLevel();
	}
}
