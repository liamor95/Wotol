#include "WOTOLGameMode_Demo.h"
#include "DemoFlowSubsystem.h"
#include "WOTOLDemoDirector.h"
#include "WOTOLGreyboxEnvironment.h"
#include "WOTOLDemoHUD.h"
#include "WOTOLCityEnvironment.h"
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
	// est propagé à GreyboxEnvironment, Director->DefaultPlayerFaction, CityEnv->PlayerFaction
	// ET PC->SetPlayerFaction() (qui ÉCRASE la résolution propre que le PlayerController fait
	// lui-même dans son propre BeginPlay) -> un seul repli périmé ici pouvait fausser la
	// faction dans TOUTE la scène de bataille/cité, pas seulement le fond de cité.
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

	// 2b) Décor + caméra de la vue CITÉ (isométrique fixe) — posés loin de l'arène (même
	//     niveau réutilisé pour tout ce qui précède : aucun risque de chevauchement).
	const FVector CityOrigin(0.f, 30000.f, 0.f);
	const FTransform CityTM(FRotator::ZeroRotator, CityOrigin);
	CityEnv = W->SpawnActorDeferred<AWOTOLCityEnvironment>(
		AWOTOLCityEnvironment::StaticClass(), CityTM, this);
	if (CityEnv)
	{
		CityEnv->PlayerFaction = PlayerFaction;
		UGameplayStatics::FinishSpawningActor(CityEnv, CityTM);
	}
	FActorSpawnParameters CityCamParams;
	CityCamParams.Owner = this;
	CityCam = W->SpawnActor<AWOTOLCityCamera>(
		AWOTOLCityCamera::StaticClass(), CityOrigin, FRotator::ZeroRotator, CityCamParams);
	if (CityCam) CityCam->ResetToHub(CityOrigin);

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
