#include "WOTOLBattleWidget.h"
#include "Gameplay/Battle/WOTOLGameState_Battle.h"
#include "Gameplay/Battle/UnitSelectionManager.h"
#include "Gameplay/Battle/BattleTimerManager.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/HeroExperienceComponent.h"
#include "Gameplay/Resources/ResourceManager.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Kismet/GameplayStatics.h"

void UWOTOLBattleWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SubscribeToGameState();
}

void UWOTOLBattleWidget::SubscribeToGameState()
{
	if (UUnitSelectionManager* SelectionMgr =
			GetWorld()->GetSubsystem<UUnitSelectionManager>())
	{
		SelectionMgr->OnSelectionChanged.AddDynamic(this, &UWOTOLBattleWidget::OnSelectionChanged);
	}
}

float UWOTOLBattleWidget::GetCaptureProgress() const
{
	if (AWOTOLGameState_Battle* GS = Cast<AWOTOLGameState_Battle>(
			UGameplayStatics::GetGameState(this)))
	{
		return GS->CaptureProgress;
	}
	return 0.f;
}

EFactionID UWOTOLBattleWidget::GetCapturingFaction() const
{
	if (AWOTOLGameState_Battle* GS = Cast<AWOTOLGameState_Battle>(
			UGameplayStatics::GetGameState(this)))
	{
		return GS->ActiveTurnFaction;
	}
	return EFactionID::None;
}

EFactionID UWOTOLBattleWidget::GetActiveTurnFaction() const
{
	if (AWOTOLGameState_Battle* GS = Cast<AWOTOLGameState_Battle>(
			UGameplayStatics::GetGameState(this)))
	{
		return GS->ActiveTurnFaction;
	}
	return EFactionID::None;
}

EBattlePhase UWOTOLBattleWidget::GetBattlePhase() const
{
	if (AWOTOLGameState_Battle* GS = Cast<AWOTOLGameState_Battle>(
			UGameplayStatics::GetGameState(this)))
	{
		return GS->CurrentPhase;
	}
	return EBattlePhase::Preparation;
}

int32 UWOTOLBattleWidget::GetPlayerUnitCount() const
{
	if (AWOTOLGameState_Battle* GS = Cast<AWOTOLGameState_Battle>(
			UGameplayStatics::GetGameState(this)))
	{
		if (UFactionRegistrySubsystem* Registry =
				GetWorld()->GetSubsystem<UFactionRegistrySubsystem>())
		{
			return Registry->GetUnitCountForFaction(GS->PlayerFaction);
		}
	}
	return 0;
}

int32 UWOTOLBattleWidget::GetEnemyUnitCount() const
{
	if (AWOTOLGameState_Battle* GS = Cast<AWOTOLGameState_Battle>(
			UGameplayStatics::GetGameState(this)))
	{
		if (UFactionRegistrySubsystem* Registry =
				GetWorld()->GetSubsystem<UFactionRegistrySubsystem>())
		{
			return Registry->GetUnitCountForFaction(GS->EnemyFaction);
		}
	}
	return 0;
}

TArray<AUnitBase*> UWOTOLBattleWidget::GetSelectedUnits() const
{
	if (UUnitSelectionManager* SelectionMgr =
			GetWorld()->GetSubsystem<UUnitSelectionManager>())
	{
		return SelectionMgr->GetSelectedUnits();
	}
	return {};
}

float UWOTOLBattleWidget::GetTimeRemaining() const
{
	if (UBattleTimerManager* Timer = GetWorld()->GetSubsystem<UBattleTimerManager>())
	{
		return Timer->GetRemainingSeconds();
	}
	return 0.f;
}

FText UWOTOLBattleWidget::GetFormattedTime() const
{
	if (UBattleTimerManager* Timer = GetWorld()->GetSubsystem<UBattleTimerManager>())
	{
		return Timer->GetFormattedTime();
	}
	return FText::FromString(TEXT("00:00"));
}

int32 UWOTOLBattleWidget::GetResourceAmount(EFactionID Faction, EResourceType Resource) const
{
	if (UResourceManager* RM = GetWorld()->GetSubsystem<UResourceManager>())
	{
		return RM->GetResource(Faction, Resource);
	}
	return 0;
}

int32 UWOTOLBattleWidget::GetHeroLevel() const
{
	const TArray<AUnitBase*> Selected = GetSelectedUnits();
	for (AUnitBase* Unit : Selected)
	{
		if (!Unit) continue;
		if (UHeroExperienceComponent* XP = Unit->FindComponentByClass<UHeroExperienceComponent>())
		{
			return XP->GetCurrentLevel();
		}
	}
	return 1;
}

float UWOTOLBattleWidget::GetHeroXPProgress() const
{
	const TArray<AUnitBase*> Selected = GetSelectedUnits();
	for (AUnitBase* Unit : Selected)
	{
		if (!Unit) continue;
		if (UHeroExperienceComponent* XP = Unit->FindComponentByClass<UHeroExperienceComponent>())
		{
			return XP->GetXPProgress();
		}
	}
	return 0.f;
}
