#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WOTOLPlayerController_Exploration.generated.h"

// PlayerController de l'exploration — gère les inputs du héros TPS
// Câble l'input directement en C++, aucun BP d'input nécessaire
UCLASS()
class WOTOL_API AWOTOLPlayerController_Exploration : public APlayerController
{
	GENERATED_BODY()

public:
	AWOTOLPlayerController_Exploration();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	void Interact();
};
