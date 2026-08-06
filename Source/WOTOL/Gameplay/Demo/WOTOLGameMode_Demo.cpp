#include "WOTOLGameMode_Demo.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLDemoDirector.h"
#include "WOTOLGreyboxEnvironment.h"
#include "WOTOLDemoHUD.h"
#include "WOTOLCityCamera.h"
#include "Gameplay/Battle/WOTOLBattleCamera.h"
#include "Gameplay/Battle/WOTOLPlayerController_Battle.h"
#include "Kismet/GameplayStatics.h"

AWOTOLGameMode_Demo::AWOTOLGameMode_Demo()
{
	// PlayerController de bataille : sélection + ordres (clic droit) sur tes unités
	DefaultPawnClass      = nullptr;
	PlayerControllerClass = AWOTOLPlayerController_Battle::StaticClass();
	HUDClass              = AWOTOLDemoHUD::StaticClass(); // HUD dessiné en C++ (pas d'UMG)
}

void AWOTOLGameMode_Demo::BeginPlay()
{
	Super::BeginPlay();

	UWorld* W = GetWorld();
	if (!W) return;

	// Faction joueur (menu ou défaut Aquiloris). BUG CORRIGE (01/08/2026, retour terrain :
	// "j'avais choisi les Noxeens et tu as mis le visuel de la cité des Aquiloris") : lisait
	// directement GameInstance->GetSelectedFaction() au lieu de la source FIABLE
	// UDemoFlowSubsystem::GetPlayerFaction() -- même anti-pattern déjà corrigé dans
	// AWOTOLHeroCharacter::BeginPlay() et AWOTOLPlayerController_Battle::BeginPlay() plus tôt
	// dans la session, mais présent ICI AUSSI (3e occurrence non détectée alors). Ce résultat
	// est propagé à GreyboxEnvironment (qui sert AUSSI de décor de cité, voir RebuildAsCity),
	// Director->DefaultPlayerFaction ET PC->SetPlayerFaction() (qui ÉCRASE la résolution propre
	// que le PlayerController fait lui-même dans son propre BeginPlay) -> un seul repli périmé
	// ici pouvait fausser la faction dans TOUTE la scène de bataille/cité.
	EFactionID PlayerFaction = EFactionID::Aquiloris;
	if (UDemoFlowSubsystem* Demo = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UDemoFlowSubsystem>() : nullptr)
	{
		if (Demo->GetPlayerFaction() != EFactionID::None)
		{
			PlayerFaction = Demo->GetPlayerFaction();
		}
	}

	// 0) Décor greybox contextualisé (faction fixée avant BeginPlay)
	const FTransform EnvTM(FRotator::ZeroRotator, FVector::ZeroVector);
	if (AWOTOLGreyboxEnvironment* Env = W->SpawnActorDeferred<AWOTOLGreyboxEnvironment>(
			AWOTOLGreyboxEnvironment::StaticClass(), EnvTM, this))
	{
		Env->PlayerFaction = PlayerFaction;
		UGameplayStatics::FinishSpawningActor(Env, EnvTM);
	}

	// 1) Director : monte les armées et lance la bataille (différé = faction avant BeginPlay)
	const FTransform DirTM(FRotator::ZeroRotator, FVector::ZeroVector);
	Director = W->SpawnActorDeferred<AWOTOLDemoDirector>(
		AWOTOLDemoDirector::StaticClass(), DirTM, this);
	if (Director)
	{
		Director->DefaultPlayerFaction = PlayerFaction;
		UGameplayStatics::FinishSpawningActor(Director, DirTM);
	}

	// 2) Caméra de bataille libre, cadrée d'emblée sur l'armée du joueur
	//    (l'armée joueur est montée à gauche : X = -ArmySeparation/2 ; l'ennemi à droite).
	const float Sep = Director ? Director->ArmySeparation : 4500.f;
	const FVector PlayerOrigin(-Sep * 0.5f, 0.f, 0.f);
	// Pivot un peu en avant de l'armée (vers l'ennemi) et légèrement surélevé
	const FVector CamFocus = PlayerOrigin + FVector(700.f, 0.f, 150.f);

	FActorSpawnParameters CamParams;
	CamParams.Owner = this;
	Camera = W->SpawnActor<AWOTOLBattleCamera>(
		AWOTOLBattleCamera::StaticClass(), CamFocus, FRotator::ZeroRotator, CamParams);
	if (Camera)
	{
		// Yaw 0 = regard vers +X (l'ennemi) ; pitch plongeant ; zoom proche de l'armée
		Camera->SetInitialView(CamFocus, 0.f, -45.f, 2600.f);
	}

	// 2b) Caméra de la vue CITÉ (isométrique fixe). PLUS de décor séparé ici (l'ancien
	//     AWOTOLCityEnvironment était posé à 30000 unités de l'arène — supprimé le 02/08/2026,
	//     demande explicite de Liamor). La cité réutilise désormais le MÊME
	//     AWOTOLGreyboxEnvironment que la bataille, reconfiguré à la volée par
	//     AWOTOLDemoDirector::HandleScreenChanged (RebuildAsCity) au même emplacement -> la
	//     caméra part donc de l'origine, comme le reste de la scène.
	FActorSpawnParameters CityCamParams;
	CityCamParams.Owner = this;
	CityCam = W->SpawnActor<AWOTOLCityCamera>(
		AWOTOLCityCamera::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, CityCamParams);
	if (CityCam) CityCam->ResetToHub(FVector::ZeroVector);

	// 3) Branche le PlayerController : faction + caméra (possession)
	if (AWOTOLPlayerController_Battle* PC =
			Cast<AWOTOLPlayerController_Battle>(W->GetFirstPlayerController()))
	{
		PC->SetPlayerFaction(PlayerFaction);
		if (Camera)
		{
			PC->SetBattleCamera(Camera);
		}
		PC->bShowMouseCursor = true;
	}
}
