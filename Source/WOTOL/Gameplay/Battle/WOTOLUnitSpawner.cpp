#include "WOTOLUnitSpawner.h"
#include "Gameplay/Units/UnitBase.h"
#include "Gameplay/Units/UnitDataAsset.h"
#include "Gameplay/AI/AIAdaptiveController.h"

AWOTOLUnitSpawner::AWOTOLUnitSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWOTOLUnitSpawner::BeginPlay()
{
	Super::BeginPlay();
	// Le GameMode appelle SpawnUnits() après avoir configuré la bataille
}

void AWOTOLUnitSpawner::SpawnUnits()
{
	const int32 Total = UnitsToSpawn.Num();

	for (int32 i = 0; i < Total; ++i)
	{
		UUnitDataAsset* Data = UnitsToSpawn[i];
		if (!Data || Data->UnitClass.IsNull()) continue;

		TSubclassOf<AUnitBase> UnitClass = Data->UnitClass.LoadSynchronous();
		if (!UnitClass) continue;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		const FVector SpawnLoc = GetFormationSlot(i, Total);
		AUnitBase* Unit = GetWorld()->SpawnActor<AUnitBase>(
			UnitClass, SpawnLoc, GetActorRotation(), Params);

		if (Unit)
		{
			Unit->UnitData = Data;

			// L'AIController est auto-spawné par ACharacter si AIControllerClass est set
			// On l'informe de la faction
			if (UAIAdaptiveController* AIC =
					Cast<UAIAdaptiveController>(Unit->GetController()))
			{
				AIC->SetControlledFaction(Faction);
			}

			OnUnitSpawned(Unit);
		}
	}
}

FVector AWOTOLUnitSpawner::GetFormationSlot(int32 Index, int32 Total) const
{
	// Formation en ligne (extensible à d'autres formations)
	const FVector Right = GetActorRightVector();
	const float   Offset = (Index - (Total - 1) * 0.5f) * FormationSpacing;
	return GetActorLocation() + Right * Offset;
}
