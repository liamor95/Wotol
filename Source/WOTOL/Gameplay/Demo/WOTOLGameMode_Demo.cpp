#include "WOTOLGameMode_Demo.h"
#include "WOTOLDemoDirector.h"
#include "WOTOLGreyboxEnvironment.h"
#include "Gameplay/Battle/WOTOLBattleCamera.h"
#include "Core/WOTOLGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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

	// Faction joueur (menu ou défaut Aquiloris)
	EFactionID PlayerFaction = EFactionID::Aquiloris;
	if (const UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance()))
	{
		if (GI->GetSelectedFaction() != EFactionID::None)
		{
			PlayerFaction = GI->GetSelectedFaction();
		}
	}

	// 0) Décor greybox contextualisé (sol, arche centrale, zones de déploiement)
	//    Spawn différé pour fixer la faction AVANT BeginPlay (couleurs correctes)
	const FTransform EnvTM(FRotator::ZeroRotator, FVector::ZeroVector);
	if (AWOTOLGreyboxEnvironment* Env = W->SpawnActorDeferred<AWOTOLGreyboxEnvironment>(
			AWOTOLGreyboxEnvironment::StaticClass(), EnvTM, this))
	{
		Env->PlayerFaction = PlayerFaction;
		UGameplayStatics::FinishSpawningActor(Env, EnvTM);
	}

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
