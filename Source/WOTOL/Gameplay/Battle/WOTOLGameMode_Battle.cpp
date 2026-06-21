#include "WOTOLGameMode_Battle.h"
#include "WOTOLGameState_Battle.h"
#include "BattleStateObserver.h"
#include "TacticalPhaseManager.h"
#include "TerritoryStateManager.h"
#include "Core/WOTOLGameInstance.h"
#include "Core/SaveGameSubsystem.h"
#include "Kismet/GameplayStatics.h"

AWOTOLGameMode_Battle::AWOTOLGameMode_Battle()
{
	GameStateClass = AWOTOLGameState_Battle::StaticClass();
}

void AWOTOLGameMode_Battle::BeginPlay()
{
	Super::BeginPlay();

	// Instancier les managers (UObject, pas des Actors)
	PhaseManager     = NewObject<UTacticalPhaseManager>(this);
	TerritoryManager = NewObject<UTerritoryStateManager>(this);

	PhaseManager->Initialize(GetWorld());
	TerritoryManager->Initialize(GetWorld());

	// Spawner l'observer dans le monde
	FActorSpawnParameters Params;
	Params.Owner = this;
	StateObserver = GetWorld()->SpawnActor<ABattleStateObserver>(
		ABattleStateObserver::StaticClass(), FTransform::Identity, Params);

	// Brancher les délégués
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

	SetupBattleFromGameInstance();
}

void AWOTOLGameMode_Battle::EndPlay(const EEndPlayReason::Type Reason)
{
	if (PhaseManager) PhaseManager->Shutdown();
	Super::EndPlay(Reason);
}

void AWOTOLGameMode_Battle::SetupBattleFromGameInstance()
{
	UWOTOLGameInstance* GI = Cast<UWOTOLGameInstance>(GetGameInstance());
	if (!GI) return;

	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (!GS) return;

	GS->PlayerFaction = GI->SelectedFaction;
	// En démo : l'ennemi est toujours Noxéens si le joueur joue Aquiloris, et vice versa
	GS->EnemyFaction = (GI->SelectedFaction == EFactionID::Aquiloris)
		? EFactionID::Noxeens : EFactionID::Aquiloris;

	if (StateObserver)
	{
		StateObserver->ActiveFactions = { GS->PlayerFaction, GS->EnemyFaction };
		StateObserver->BeginObserving();
	}

	StartBattle();
}

void AWOTOLGameMode_Battle::StartBattle()
{
	AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>();
	if (!GS || !StateObserver) return;

	StateObserver->SetBattlePhase(EBattlePhase::Tactical);

	const TArray<EFactionID> TurnOrder = { GS->PlayerFaction, GS->EnemyFaction };
	PhaseManager->StartPhase(TurnOrder, TacticalWindowDuration);
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
	if (GS)
	{
		GS->BattleResult = Result;
	}

	// Sauvegarde automatique en fin de bataille
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
	if (AWOTOLGameState_Battle* GS = GetGameState<AWOTOLGameState_Battle>())
	{
		GS->ActiveTurnFaction = Faction;
	}
}

void AWOTOLGameMode_Battle::OnTacticalWindowClosed(EFactionID /*Faction*/)
{
	// Le HUD Blueprint peut s'abonner directement à PhaseManager
}

void AWOTOLGameMode_Battle::OnTerritoryCaptured(EFactionID Faction, FTerritoryGrade Grade)
{
	if (StateObserver)
	{
		StateObserver->SetBattlePhase(EBattlePhase::Resolution);
	}

	// Sauvegarde du grade capturé
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
