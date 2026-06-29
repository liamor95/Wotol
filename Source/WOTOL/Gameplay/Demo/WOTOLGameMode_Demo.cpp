#include "WOTOLGameMode_Demo.h"
#include "WOTOLDemoDirector.h"
#include "Gameplay/Battle/WOTOLBattleCamera.h"
#include "GameFramework/PlayerController.h"

AWOTOLGameMode_Demo::AWOTOLGameMode_Demo()
{
	// Pas de pawn par défaut : on possède manuellement la caméra de bataille
	DefaultPawnClass      = nullptr;
	PlayerControllerClass = APlayerController::StaticClass();
}

void AWOTOLGameMode_Demo::BeginPlay()
{
	Super::BeginPlay();

	UWorld* W = GetWorld();
	if (!W) return;

	// 1) Director : monte les armées greybox et lance la bataille RTS
	FActorSpawnParameters DirParams;
	DirParams.Owner = this;
	Director = W->SpawnActor<AWOTOLDemoDirector>(
		AWOTOLDemoDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, DirParams);

	// 2) Caméra de bataille libre, possédée par le joueur pour voir la scène
	FActorSpawnParameters CamParams;
	CamParams.Owner = this;
	Camera = W->SpawnActor<AWOTOLBattleCamera>(
		AWOTOLBattleCamera::StaticClass(), CameraSpawnLocation, FRotator::ZeroRotator, CamParams);

	if (APlayerController* PC = W->GetFirstPlayerController())
	{
		if (Camera)
		{
			PC->Possess(Camera);
		}
		PC->bShowMouseCursor = true;
	}
}
