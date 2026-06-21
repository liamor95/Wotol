#include "WOTOLPlayerController_Exploration.h"

AWOTOLPlayerController_Exploration::AWOTOLPlayerController_Exploration()
{
	bShowMouseCursor = false;
}

void AWOTOLPlayerController_Exploration::BeginPlay()
{
	Super::BeginPlay();

	// Mode souris caché, rotation libre
	SetInputMode(FInputModeGameOnly());
}

void AWOTOLPlayerController_Exploration::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindAction("Interact", IE_Pressed, this,
		&AWOTOLPlayerController_Exploration::Interact);
}

void AWOTOLPlayerController_Exploration::Interact()
{
	// Placeholder — BP peut override via BlueprintImplementableEvent sur le héros
}
