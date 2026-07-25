#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WOTOLGameMode_Exploration.generated.h"

// GameMode de la phase d'exploration troisième personne
// Responsable : spawn du héros, gestion du trigger d'encounter
UCLASS()
class WOTOL_API AWOTOLGameMode_Exploration : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWOTOLGameMode_Exploration();

	virtual void BeginPlay() override;

protected:
	// Classe de héros par défaut ; surchargée en Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Exploration")
	TSoftClassPtr<class AWOTOLHeroCharacter> HeroClass;
};
