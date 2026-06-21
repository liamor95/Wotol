#include "EncounterTriggerVolume.h"
#include "WOTOLHeroCharacter.h"
#include "Core/WOTOLGameInstance.h"
#include "Kismet/GameplayStatics.h"

AEncounterTriggerVolume::AEncounterTriggerVolume()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEncounterTriggerVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bOneShot && bTriggered) return;

	AWOTOLHeroCharacter* Hero = Cast<AWOTOLHeroCharacter>(OtherActor);
	if (!Hero) return;

	bTriggered = true;
	OnEncounterTriggered.Broadcast(Hero);

	// La transition effective vers la bataille est déclenchée depuis le Blueprint
	// (cinématique, fade, etc.) — le C++ ne force pas le level change directement
}

void AEncounterTriggerVolume::TransitionToBattle()
{
	if (!BattleLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, BattleLevel);
	}
}
