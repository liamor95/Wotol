#include "WOTOLGameMode_Exploration.h"
#include "WOTOLHeroCharacter.h"
#include "WOTOLPlayerController_Exploration.h"

AWOTOLGameMode_Exploration::AWOTOLGameMode_Exploration()
{
	DefaultPawnClass      = AWOTOLHeroCharacter::StaticClass();
	PlayerControllerClass = AWOTOLPlayerController_Exploration::StaticClass();
}

void AWOTOLGameMode_Exploration::BeginPlay()
{
	Super::BeginPlay();
}
