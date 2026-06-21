#include "WOTOLGameMode_Exploration.h"
#include "WOTOLHeroCharacter.h"

AWOTOLGameMode_Exploration::AWOTOLGameMode_Exploration()
{
	DefaultPawnClass = AWOTOLHeroCharacter::StaticClass();
}

void AWOTOLGameMode_Exploration::BeginPlay()
{
	Super::BeginPlay();
}
