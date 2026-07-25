#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerVolume.h"
#include "EncounterTriggerVolume.generated.h"

// Déclencheur de l'encounter boss (style Pokémon)
// Le Blueprint héritant gère la cinématique de transition et le changement de niveau
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEncounterTriggered, class AWOTOLHeroCharacter*, Hero);

UCLASS()
class WOTOL_API AEncounterTriggerVolume : public ATriggerVolume
{
	GENERATED_BODY()

public:
	AEncounterTriggerVolume();

	UPROPERTY(BlueprintAssignable, Category = "Encounter")
	FOnEncounterTriggered OnEncounterTriggered;

	// Niveau de bataille à charger après le trigger
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TSoftObjectPtr<UWorld> BattleLevel;

	// Une seule activation par session
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	bool bOneShot = true;

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
	bool bTriggered = false;

	void TransitionToBattle();
};
