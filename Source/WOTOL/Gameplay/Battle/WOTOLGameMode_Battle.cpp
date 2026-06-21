#include "WOTOLGameMode_Battle.h"
#include "WOTOLGameState_Battle.h"
#include "BattleStateObserver.h"
#include "TacticalPhaseManager.h"
#include "TerritoryStateManager.h"
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
	GameStateClass      = AWOTOLGameState_Battle::StaticClass();
	PlayerControllerClass = AWOTOLPlayerController_Battle::StaticClass();
}

void AWOTOLGameMode_Battle::BeginPlay()
{
	Super::BeginPlay();

	// Managers
	PhaseManager     = NewObject<UTacticalPhaseManager>(this);
	TerritoryManager = NewObject<UTerritoryStateManager>(this);
	PhaseManager->Initialize(GetWorld());
	TerritoryManager->Initialize(GetWorld());

	// Appliquer la config si disponible
	if (BattleConfig)
	{
		TerritoryManager->CaptureRatePerSecond = BattleConfig->CaptureRatePerSecond;
		TerritoryManager->TargetGrade.Grade    = BattleConfig->TargetTerritoryGrade;
		TerritoryManager->TargetGrade.CaptureProgressRequired =
			BattleConfig->CaptureProgressRequired;
	}

	// BattleStateObserver
	FActorSpawnParameters Params;
	Params.Owner  = this;
	StateObserver = GetWorld()->SpawnActor<ABattleStateObserver>(
		ABattleStateObserver::StaticClass(), FTransform::Identity, Params);

	// Brancher délégués
	if (StateObserver)
	{
		StateObserver->OnBattleStateChanged.AddDynamic(
			this, &AWOTOLGameMode_Battle::OnBattlePhaseChanged);
		StateObserver->OnVictoryConditionMet.AddDynamic(
			this, &AWOTOLGameMode_Battle::OnVictoryConditionMet);
	}

	PhaseManager->OnTacticalWindowOpened.AddDynamic(
		this, &AWOTOLGameMode_Battle::OnTacticalWindowOpened);
	PhaseManager->OnTacticalWindowClosed.AddDynamic(
		this, &AWOTOLGameMode_Battle::OnTacticalWindowClosed);

	TerritoryManager->OnTerritoryCaptured.AddDynamic(
		this, &AWOTOLGameMode_Battle::OnTerritoryCaptured);
	TerritoryManager->OnCaptureProgressChanged.AddDynamic(
		this, &AWOTOLGameMode_Battle::OnCaptureProgress);

	SpawnBattleCamera();
	SetupBattleFromGameInstance();
}

void AWOTOLGameMode_Battle::EndPlay(const EEndPlayReason::Type Reason)
{
	if (PhaseManager) PhaseManager->Shutdown();
	Super::EndPlay(Reason);
}

void AWOTOLGameMode_Battle::SpawnBattleCamera()
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	BattleCamera = GetWorld()->SpawnActor<AWOTOLBattleCamera>(
		AWOTOLBattleCamera::StaticClass(),
		FVector(0.f, 0.f, 100.f), FRotator::ZeroRotator, Params);
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

	// Configurer le PlayerController avec la faction du joueur
	if (AWOTOLPlayerController_Battle* PC =
			Cast<AWOTOLPlayerController_Battle>(GetWorld()->GetFirstPlayerController()))
	{
		PC->SetPlayerFaction(GS->PlayerFaction);
	}

	TriggerUnitSpawners();
	BroadcastPlayerProfileToAI();
	StartBattle();
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

void AWOTOLGameMode_Battle::BroadcastPlayerProfileToAI()
{
	UPlayerProfileSubsystem* ProfileSys =
		GetGameInstance()->GetSubsystem<UPlayerProfileSubsystem>();
	if (!ProfileSys) return;

	const FPlayerBehaviorProfile& Profile = ProfileSys->GetProfile();

	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (!GS) return;

	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	if (!Registry) return;

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

void AWOTOLGameMode_Battle::StartBattle()
{
	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (!GS || !StateObserver) return;

	StateObserver->SetBattlePhase(EBattlePhase::Tactical);

	const float Duration = BattleConfig
		? BattleConfig->TacticalWindowDuration : 30.f;

	PhaseManager->StartPhase(
		{ GS->PlayerFaction, GS->EnemyFaction }, Duration);
}

void AWOTOLGameMode_Battle::OnBattlePhaseChanged(EBattlePhase NewPhase)
{
	if (AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>())
	{
		GS->CurrentPhase = NewPhase;
	}
}

void AWOTOLGameMode_Battle::OnVictoryConditionMet(EFactionID Winner, EBattleResult Result)
{
	PhaseManager->StopPhase();

	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (GS) GS->BattleResult = Result;

	// Mettre à jour le profil joueur
	if (UPlayerProfileSubsystem* ProfileSys =
			GetGameInstance()->GetSubsystem<UPlayerProfileSubsystem>())
	{
		ProfileSys->RecordBattleEnd(Result);
	}

	// Sauvegarde automatique
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

void AWOTOLGameMode_Battle::OnTacticalWindowOpened(EFactionID Faction)
{
	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (GS) GS->ActiveTurnFaction = Faction;

	// Activer les AI controllers ennemis pendant leur fenêtre
	UFactionRegistrySubsystem* Registry =
		GetWorld()->GetSubsystem<UFactionRegistrySubsystem>();
	AWOTOLGameState_Battle* State = GetGameState<AWOTOLGameState_Battle>();

	if (Registry && State && Faction == State->EnemyFaction)
	{
		for (AUnitBase* Unit : Registry->GetUnitsForFaction(Faction))
		{
			if (Unit)
			{
				if (UAIAdaptiveController* AIC =
						Cast<UAIAdaptiveController>(Unit->GetController()))
				{
					// Le controller active la state machine via OnTacticalWindowOpened
					// (branché dans l'AIController lui-même après avoir souscrit au PhaseManager)
				}
			}
		}
	}
}

void AWOTOLGameMode_Battle::OnTacticalWindowClosed(EFactionID /*Faction*/) {}

void AWOTOLGameMode_Battle::OnTerritoryCaptured(EFactionID Faction, FTerritoryGrade Grade)
{
	if (StateObserver)
	{
		StateObserver->SetBattlePhase(EBattlePhase::Resolution);
	}

	if (USaveGameSubsystem* SaveSys =
			GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
	{
		if (UWOTOLSaveGame* Save = SaveSys->GetSaveGame())
		{
			Save->TerritoryGradeCaptured = Grade.Grade;
		}
		SaveSys->SaveGame();
	}
}

void AWOTOLGameMode_Battle::OnCaptureProgress(EFactionID Faction, float Progress)
{
	if (AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>())
	{
		GS->CaptureProgress = Progress;
	}
}
