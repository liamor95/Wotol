#include "MenuFlowSubsystem.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

const TArray<EMenuStep> UMenuFlowSubsystem::NewGameFlow = {
	EMenuStep::MainMenu,
	EMenuStep::GameModeSelection,
	EMenuStep::FactionSelection,
	EMenuStep::ChefPresentation,
	EMenuStep::DifficultyChoice,
	EMenuStep::CaptainCreation,
	EMenuStep::Introduction,
	EMenuStep::Kingdom,
	EMenuStep::Loading
};

void UMenuFlowSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentStep = EMenuStep::MainMenu;
}

void UMenuFlowSubsystem::GoToStep(EMenuStep Step)
{
	const EMenuStep Previous = CurrentStep;
	CurrentStep = Step;
	OnMenuStepChanged.Broadcast(CurrentStep, Previous);
}

void UMenuFlowSubsystem::GoNext()
{
	const int32 Idx = GetFlowIndex(CurrentStep);
	if (Idx >= 0 && Idx < NewGameFlow.Num() - 1)
	{
		GoToStep(NewGameFlow[Idx + 1]);
	}
}

void UMenuFlowSubsystem::GoBack()
{
	const int32 Idx = GetFlowIndex(CurrentStep);
	if (Idx > 0)
	{
		GoToStep(NewGameFlow[Idx - 1]);
	}
}

bool UMenuFlowSubsystem::CanGoBack() const
{
	return GetFlowIndex(CurrentStep) > 0;
}

void UMenuFlowSubsystem::StartGame(TSoftObjectPtr<UWorld> ExplorationLevel)
{
	GoToStep(EMenuStep::Loading);
	const FString LevelName = ExplorationLevel.GetAssetName();
	UGameplayStatics::OpenLevel(GetGameInstance(), FName(*LevelName));
}

void UMenuFlowSubsystem::LoadSavedGame(TSoftObjectPtr<UWorld> ExplorationLevel)
{
	StartGame(ExplorationLevel);
}

int32 UMenuFlowSubsystem::GetFlowIndex(EMenuStep Step) const
{
	return NewGameFlow.IndexOfByKey(Step);
}
