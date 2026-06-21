#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Data/WOTOLTypes.h"
#include "WOTOLGameInstance.generated.h"

UCLASS()
class WOTOL_API UWOTOLGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// Faction choisie par le joueur avant la bataille
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	EFactionID SelectedFaction = EFactionID::None;

	// Loadout héros transmis de l'exploration à la bataille
	UPROPERTY(BlueprintReadWrite, Category = "Session")
	FHeroLoadout CurrentHeroLoadout;

	virtual void Init() override;
	virtual void Shutdown() override;
};
