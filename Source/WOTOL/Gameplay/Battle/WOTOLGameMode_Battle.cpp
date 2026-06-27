#include "WOTOLGameMode_Battle.h"
#include "WOTOLGameState_Battle.h"
#include "BattleStateObserver.h"
#include "RTSBattleManager.h"
#include "WOTOLBattleCamera.h"
#include "WOTOLUnitSpawner.h"
#include "WOTOLPlayerController_Battle.h"
#include "Core/WOTOLGameInstance.h"
#include "Core/SaveGameSubsystem.h"
#include "Core/PlayerProfileSubsystem.h"
#include "Core/FactionRegistrySubsystem.h"
#include "Data/BattleConfigDataAsset.h"
#include "Gameplay/AI/AIAdaptiveController.h"
#include "Gameplay/Units/UnitBase.h"
#include "Kismet/GameplayStatics.h"

AWOTOLGameMode_Battle::AWOTOLGameMode_Battle()
{
	GameStateClass        = AWOTOLGameState_Battle::StaticClass();
	PlayerControllerClass = AWOTOLPlayerController_Battle::StaticClass();
}

void AWOTOLGameMode_Battle::BeginPlay()
{
	Super::BeginPlay();

	// BattleStateObserver — vérifie les conditions victoire/défaite en temps réel
	FActorSpawnParameters Params;
	Params.Owner  = this;
	StateObserver = GetWorld()->SpawnActor<ABattleStateObserver>(
		ABattleStateObserver::StaticClass(), FTransform::Identity, Params);

	if (StateObserver)
	{
		StateObserver->OnBattleStateChanged.AddDynamic(
			this, &AWOTOLGameMode_Battle::OnBattlePhaseChanged);
		StateObserver->OnVictoryConditionMet.AddDynamic(
			this, &AWOTOLGameMode_Battle::OnVictoryConditionMet);
	}

	// S'abonner au gestionnaire de bataille RTS
	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->OnBattlePhaseChanged.AddDynamic(
			this, &AWOTOLGameMode_Battle::OnBattlePhaseChanged);
		RTS->OnBattleEnded.AddDynamic(
			this, &AWOTOLGameMode_Battle::OnBattleEnded);
	}

	SpawnBattleCamera();
	SetupBattleFromGameInstance();
}

void AWOTOLGameMode_Battle::SetupBattleFromGameInstance()
{
	UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance());
	if (!GI) return;

	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (!GS) return;

	GS->PlayerFaction = GI->SelectedFaction;
	GS->EnemyFaction  = (GI->SelectedFaction == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;

	if (StateObserver)
	{
		StateObserver->ActiveFactions = { GS->PlayerFaction, GS->EnemyFaction };
		StateObserver->BeginObserving();
	}

	if (AWOTOLPlayerController_Battle* PC =
			Cast<AWOTOLPlayerController_Battle>(GetWorld()->GetFirstPlayerController()))
	{
		PC->SetPlayerFaction(GS->PlayerFaction);
	}

	TriggerUnitSpawners();
	BroadcastPlayerProfileToAI();
	StartDeploymentPhase();
}

void AWOTOLGameMode_Battle::TriggerUnitSpawners()
{
	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), AWOTOLUnitSpawner::StaticClass(), Spawners);

	for (AActor* Actor : Spawners)
	{
		if (AWOTOLUnitSpawner* Spawner = Cast<AWOTOLUnitSpawner>(Actor))
		{
			Spawner->SpawnUnits();
		}
	}
}

void AWOTOLGameMode_Battle::SpawnBattleCamera()
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	BattleCamera = GetWorld()->SpawnActor<AWOTOLBattleCamera>(
		AWOTOLBattleCamera::StaticClass(),
		FVector(0.f, 0.f, 2000.f), FRotator(-60.f, 0.f, 0.f), Params);
}

void AWOTOLGameMode_Battle::StartDeploymentPhase()
{
	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (GS) GS->bIsDeploymentPhase = true;

	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		RTS->StartDeploymentPhase();
	}
}

void AWOTOLGameMode_Battle::OnDeploymentConfirmed()
{
	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (GS) GS->bIsDeploymentPhase = false;

	const float Duration = BattleConfig ? BattleConfig->BattleDurationSeconds : 3600.f;

	if (URTSBattleManager* RTS = GetWorld()->GetSubsystem<URTSBattleManager>())
	{
		// Lance le combat RTS — active toutes les IA ennemies simultanément
		RTS->StartBattlePhase(Duration);
	}
}

void AWOTOLGameMode_Battle::BroadcastPlayerProfileToAI()
{
	UPlayerProfileSubsystem* ProfileSys =
		GetGameInstance()->GetSubsystem<UPlayerProfileSubsystem>();
	if (!ProfileSys) return;

	const FPlayerBehaviorProfile& Profile = ProfileSys->GetProfile();

	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!GS || !Registry) return;

	for (AUnitBase* Unit : Registry->GetUnitsForFaction(GS->EnemyFaction))
	{
		if (!Unit) continue;
		if (UAIAdaptiveController* AIC =
				Cast<UAIAdaptiveController>(Unit->GetController()))
		{
			AIC->UpdatePlayerProfile(Profile);
		}
	}
}

void AWOTOLGameMode_Battle::OnBattlePhaseChanged(EBattlePhase NewPhase)
{
	if (AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>())
	{
		GS->CurrentPhase = NewPhase;
	}
}

void AWOTOLGameMode_Battle::OnBattleEnded(EFactionID Winner, EBattleResult Result)
{
	OnVictoryConditionMet(Winner, Result);
}

void AWOTOLGameMode_Battle::OnVictoryConditionMet(EFactionID Winner, EBattleResult Result)
{
	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (GS) GS->BattleResult = Result;

	if (StateObserver)
	{
		StateObserver->SetBattlePhase(EBattlePhase::Resolution);
	}

	if (UPlayerProfileSubsystem* ProfileSys =
			GetGameInstance()->GetSubsystem<UPlayerProfileSubsystem>())
	{
		ProfileSys->RecordBattleEnd(Result);
	}

	if (USaveGameSubsystem* SaveSys =
			GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
	{
		if (UWOTOLSaveGame* Save = SaveSys->GetSaveGame())
		{
			if (Result == EBattleResult::Victory) Save->TotalVictories++;
			else if (Result == EBattleResult::Defeat) Save->TotalDefeats++;
		}
		SaveSys->SaveGame();
	}
}
